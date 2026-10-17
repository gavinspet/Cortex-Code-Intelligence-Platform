/**
 * @file ProcessRunner.h
 * @brief Runs an external program without a shell, with a timeout, output cap and resource limits
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

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cortex::utils {

struct ProcessOptions {
    /// Wall-clock limit; the whole process group is SIGKILLed when exceeded.
    std::chrono::milliseconds timeout{std::chrono::seconds(120)};
    /// Maximum bytes of combined stdout/stderr retained; the rest is drained and discarded.
    std::size_t maxOutputBytes = 64 * 1024;
    /// Optional RLIMIT_FSIZE (max size of any single file the child may write).
    std::optional<unsigned long long> maxFileSizeBytes;
    /// Names of parent environment variables passed through (if set). Nothing else is inherited.
    std::vector<std::string> inheritEnv;
    /// Variables set explicitly for the child (override inherited values).
    std::vector<std::pair<std::string, std::string>> setEnv;
};

struct ProcessResult {
    bool started = false;
    bool timedOut = false;
    bool outputTruncated = false;
    int exitCode = -1;     ///< Exit status when the child exited normally, otherwise -1.
    int termSignal = 0;    ///< Signal number when the child was killed by a signal.
    std::string output;    ///< Combined stdout + stderr (capped).
    std::string error;     ///< Launch error description when started == false.

    bool succeeded() const noexcept {
        return started && !timedOut && termSignal == 0 && exitCode == 0;
    }
};

class ProcessRunner {
public:
    /**
     * Execute argv[0] with arguments argv[1..] via fork/execve. No shell is involved,
     * so arguments are passed verbatim. argv[0] is resolved against PATH in the parent
     * when it contains no '/'. stdin is /dev/null and all other descriptors are closed.
     */
    static ProcessResult run(const std::vector<std::string>& argv,
                             const ProcessOptions& options) noexcept;

    ProcessRunner() = delete;
};

} // namespace cortex::utils
