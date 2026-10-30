#pragma once

#include <string>
#include <vector>

namespace deadlock::core {

/**
 * @brief Lifecycle states for a simulated process.
 */
enum class ProcessState {
    NEW,          ///< Initialized, not yet registered/admitted
    READY,        // Eligible to run or waiting for admission
    RUNNING,      ///< Actively holding allocated resources and executing
    WAITING,      ///< Blocked waiting for requested resources to become available
    TERMINATED    ///< Completed execution and released all held resources
};

/**
 * @brief Convert ProcessState enum to human-readable string.
 */
inline std::string processStateToString(ProcessState state) {
    switch (state) {
        case ProcessState::NEW:        return "NEW";
        case ProcessState::READY:      return "READY";
        case ProcessState::RUNNING:    return "RUNNING";
        case ProcessState::WAITING:    return "WAITING";
        case ProcessState::TERMINATED: return "TERMINATED";
        default:                       return "UNKNOWN";
    }
}

/**
 * @brief Historical checkpoint for state rollback and preemption recovery.
 */
struct ProcessCheckpoint {
    int stepNumber{0};
    std::vector<int> heldAllocation;
    int cpuTimeSnapshot{0};
};

/**
 * @brief Simulated Process entity.
 */
struct Process {
    int id{0};                                  ///< Unique identifier (0 to n-1)
    std::string name;                           ///< Process label (e.g. "P0", "WorkerThread")
    int priority{5};                            ///< Priority level (1 = lowest, 10 = highest)
    ProcessState state{ProcessState::NEW};      ///< Current lifecycle state
    int cpuTimeUsed{0};                         ///< Simulated execution time spent
    int remainingTime{10};                      ///< Estimated remaining execution time
    int preemptionCount{0};                     ///< Times preempted (starvation avoidance metric)
    std::vector<ProcessCheckpoint> checkpoints; ///< Historical rollback checkpoints
};

} // namespace deadlock::core
