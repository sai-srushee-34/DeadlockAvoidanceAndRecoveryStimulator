#pragma once

#include "core/SystemState.hpp"

#include <vector>
#include <string>

namespace deadlock::detection {

/**
 * @brief Structured outcome of the multi-instance deadlock detection algorithm.
 */
struct DetectionResult {
    bool validState{true};                 ///< True if system state satisfies all mathematical invariants
    std::string stateError;                ///< Invariant violation description if state is invalid
    bool isDeadlocked{false};              ///< True if at least one process cannot complete (deadlocked subset D is non-empty)
    std::vector<int> deadlockedProcessIds; ///< Process IDs belonging to the deadlocked set D
    std::vector<int> reducibleProcessIds;  ///< Process IDs that successfully completed in reduction sequence
    std::vector<int> finalWork;            ///< Unallocated Work vector at reduction termination
    std::vector<std::string> reductionSteps; ///< Detailed trace of each reduction step
    std::string diagnosticReport;          ///< Comprehensive human-readable report
};

/**
 * @brief Multi-Instance Deadlock Detection Engine implementing the Shoshani-Coffman algorithm.
 * 
 * Analyzes active Allocation and outstanding Request matrices against Available resources.
 * Supports multiple instances per resource type. Operates with zero side effects on the live engine.
 */
class DetectionEngine {
public:
    /**
     * @brief Run the Shoshani-Coffman matrix reduction deadlock detection algorithm.
     * 
     * Algorithm Steps:
     * 1. Validate state invariants; if invalid, return early with validState = false.
     * 2. Initialize Work = Available.
     * 3. Initialize Finish[i] = true for all i where Allocation[i] == 0; false otherwise.
     * 4. Find an index i such that Finish[i] == false and Request[i] <= Work.
     * 5. If found, simulate completion: Work += Allocation[i], Finish[i] = true, repeat step 4.
     * 6. If Finish[i] == false for any i, system is DEADLOCKED; deadlocked set D = { i | Finish[i] == false }.
     * 
     * Algorithmic Complexity:
     *   Time Complexity:  O(m * n^2) in worst case (n processes, m resource types).
     *   Space Complexity: O(m + n) for local Work vector and Finish boolean array.
     * 
     * @param state Immutable reference to current system state
     * @return DetectionResult containing status, deadlocked process IDs, and reduction steps
     */
    static DetectionResult detectDeadlock(const core::SystemState& state);

private:
    static std::string vectorToString(const std::vector<int>& vec);
};

} // namespace deadlock::detection
