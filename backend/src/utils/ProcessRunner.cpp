/**
 * @file ProcessRunner.cpp
 * @brief fork/execve process runner: no shell, minimal environment, timeout, output cap, RLIMIT_FSIZE
 *
 * @project Cortex Code Intelligence Platform
 *
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#include "utils/ProcessRunner.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <map>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace cortex::utils {

namespace {

std::optional<std::string> resolveExecutable(const std::string& name) {
    if (name.empty()) {
        return std::nullopt;
    }
    if (name.find('/') != std::string::npos) {
        if (::access(name.c_str(), X_OK) == 0) {
            return name;
        }
        return std::nullopt;
    }

    const char* rawPath = std::getenv("PATH");
    const std::string pathList = (rawPath && *rawPath) ? rawPath : "/usr/local/bin:/usr/bin:/bin";
    std::size_t start = 0;
    while (true) {
        const auto end = pathList.find(':', start);
        const std::string dir = pathList.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        // Only absolute PATH entries; never resolve relative to the working directory.
        if (!dir.empty() && dir.front() == '/') {
            std::string candidate = dir + "/" + name;
            if (::access(candidate.c_str(), X_OK) == 0) {
                return candidate;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return std::nullopt;
}

int descriptorLimit() noexcept {
    struct rlimit rl {};
    if (::getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur != RLIM_INFINITY && rl.rlim_cur > 0) {
        return static_cast<int>(std::min<rlim_t>(rl.rlim_cur, 65536));
    }
    return 4096;
}

void closeQuietly(int& fd) noexcept {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void applyExitStatus(int status, ProcessResult& result) noexcept {
    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.termSignal = WTERMSIG(status);
    }
}

// Kill the child's process group (git spawns helpers such as git-remote-https) and reap the child.
void killAndReap(pid_t pid, ProcessResult& result) noexcept {
    ::kill(-pid, SIGKILL);
    ::kill(pid, SIGKILL);
    int status = 0;
    pid_t waited = -1;
    do {
        waited = ::waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited == pid) {
        applyExitStatus(status, result);
    }
}

} // namespace

ProcessResult ProcessRunner::run(const std::vector<std::string>& argv,
                                 const ProcessOptions& options) noexcept {
    ProcessResult result;
    int pipeFds[2] = {-1, -1};
    pid_t pid = -1;
    bool reaped = false;

    try {
        if (argv.empty()) {
            result.error = "empty argv";
            return result;
        }
        const auto executable = resolveExecutable(argv[0]);
        if (!executable) {
            result.error = "executable not found: " + argv[0];
            return result;
        }

        // Prepare everything the child needs before fork(): in a multi-threaded
        // process the child may only make async-signal-safe calls.
        std::vector<std::string> args(argv.begin(), argv.end());
        std::vector<char*> childArgv;
        childArgv.reserve(args.size() + 1);
        for (auto& arg : args) {
            childArgv.push_back(arg.data());
        }
        childArgv.push_back(nullptr);

        std::map<std::string, std::string> envMap;
        for (const auto& name : options.inheritEnv) {
            if (const char* value = std::getenv(name.c_str())) {
                envMap[name] = value;
            }
        }
        for (const auto& [name, value] : options.setEnv) {
            envMap[name] = value;
        }
        std::vector<std::string> envStrings;
        envStrings.reserve(envMap.size());
        for (const auto& [name, value] : envMap) {
            envStrings.push_back(name + "=" + value);
        }
        std::vector<char*> childEnv;
        childEnv.reserve(envStrings.size() + 1);
        for (auto& entry : envStrings) {
            childEnv.push_back(entry.data());
        }
        childEnv.push_back(nullptr);

        const char* exePath = executable->c_str();
        const bool limitFileSize = options.maxFileSizeBytes.has_value();
        struct rlimit fileSizeLimit {};
        if (limitFileSize) {
            fileSizeLimit.rlim_cur = static_cast<rlim_t>(*options.maxFileSizeBytes);
            fileSizeLimit.rlim_max = static_cast<rlim_t>(*options.maxFileSizeBytes);
        }
        const int fdLimit = descriptorLimit();

        // O_CLOEXEC so concurrently forked children (other worker threads) never inherit this pipe.
        if (::pipe2(pipeFds, O_CLOEXEC) != 0) {
            result.error = std::string("pipe2 failed: ") + std::strerror(errno);
            return result;
        }

        // Block every signal across fork() so the parent's handlers (e.g. the
        // server's SIGTERM handler) can never run in the child before execve().
        sigset_t allSignals;
        sigset_t previousMask;
        ::sigfillset(&allSignals);
        const bool masked = ::pthread_sigmask(SIG_SETMASK, &allSignals, &previousMask) == 0;

        pid = ::fork();
        if (pid != 0 && masked) {
            ::pthread_sigmask(SIG_SETMASK, &previousMask, nullptr);
        }
        if (pid < 0) {
            result.error = std::string("fork failed: ") + std::strerror(errno);
            closeQuietly(pipeFds[0]);
            closeQuietly(pipeFds[1]);
            return result;
        }

        if (pid == 0) {
            // ---- Child: async-signal-safe calls only ----
            ::setpgid(0, 0);
            // Default every disposition (inherited SIG_IGN survives execve; caught
            // handlers must not run here), then unblock. Errors for SIGKILL/SIGSTOP
            // and libc-reserved signals are expected and ignored.
            for (int sig = 1; sig < NSIG; ++sig) {
                ::signal(sig, SIG_DFL);
            }
            sigset_t noSignals;
            ::sigemptyset(&noSignals);
            ::sigprocmask(SIG_SETMASK, &noSignals, nullptr);

            const int devNull = ::open("/dev/null", O_RDONLY);
            if (devNull < 0 ||
                ::dup2(devNull, STDIN_FILENO) < 0 ||
                ::dup2(pipeFds[1], STDOUT_FILENO) < 0 ||
                ::dup2(pipeFds[1], STDERR_FILENO) < 0) {
                ::_exit(126);
            }

            // Do not leak server sockets / DB connections into the child.
            bool closedRange = false;
#ifdef SYS_close_range
            closedRange = ::syscall(SYS_close_range, 3U, ~0U, 0U) == 0;
#endif
            if (!closedRange) {
                for (int fd = 3; fd < fdLimit; ++fd) {
                    ::close(fd);
                }
            }

            if (limitFileSize && ::setrlimit(RLIMIT_FSIZE, &fileSizeLimit) != 0) {
                ::_exit(126);
            }

            ::execve(exePath, childArgv.data(), childEnv.data());
            ::_exit(127);
        }

        // ---- Parent ----
        ::setpgid(pid, pid);  // also set here to avoid racing the child before kill(-pid)
        closeQuietly(pipeFds[1]);
        result.started = true;

        const auto deadline = std::chrono::steady_clock::now() + options.timeout;
        const auto remainingMs = [&deadline]() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
        };

        std::array<char, 4096> buffer{};
        while (true) {
            const auto remaining = remainingMs();
            if (remaining <= 0) {
                result.timedOut = true;
                break;
            }
            struct pollfd pfd {pipeFds[0], POLLIN, 0};
            const int ready = ::poll(&pfd, 1, static_cast<int>(std::min<long long>(remaining, 1000)));
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            if (ready == 0) {
                continue;
            }
            const ssize_t n = ::read(pipeFds[0], buffer.data(), buffer.size());
            if (n > 0) {
                const std::size_t room = options.maxOutputBytes > result.output.size()
                    ? options.maxOutputBytes - result.output.size()
                    : 0;
                const std::size_t keep = std::min(room, static_cast<std::size_t>(n));
                result.output.append(buffer.data(), keep);
                if (keep < static_cast<std::size_t>(n)) {
                    result.outputTruncated = true;
                }
            } else if (n == 0) {
                break;  // EOF: every writer (child and its helpers) closed the pipe
            } else if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        }
        closeQuietly(pipeFds[0]);

        while (!result.timedOut) {
            int status = 0;
            const pid_t waited = ::waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                applyExitStatus(status, result);
                reaped = true;
                break;
            }
            if (waited < 0 && errno != EINTR) {
                // ECHILD: child already reaped elsewhere (e.g. SIGCHLD ignored); status unknown.
                result.error = std::string("waitpid failed: ") + std::strerror(errno);
                reaped = true;
                break;
            }
            if (remainingMs() <= 0) {
                result.timedOut = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!reaped) {
            killAndReap(pid, result);
            reaped = true;
        }
        return result;
    } catch (const std::exception& e) {
        result.error = std::string("process runner error: ") + e.what();
    } catch (...) {
        result.error = "process runner error";
    }

    if (pid > 0 && !reaped) {
        killAndReap(pid, result);
    }
    closeQuietly(pipeFds[0]);
    closeQuietly(pipeFds[1]);
    return result;
}

} // namespace cortex::utils
