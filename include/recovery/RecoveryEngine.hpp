#pragma once

#include "core/SystemState.hpp"
#include "detection/DetectionEngine.hpp"

#include <vector>
#include <string>

namespace deadlock::core {
class ResourceManager;
}

namespace deadlock::recovery {

/**
 * @brief Strategy used to select a victim process among the deadlocked set D.
 */
enum class VictimSelectionPolicy {
    MANUAL,              ///< Caller explicitly specifies the victim process ID
    LOWEST_PRIORITY,     ///< Select process in D with the lowest priority rank (1 = lowest, 10 = highest)
    HIGHEST_ALLOCATION,  ///< Select process in D holding the greatest total number of resource units
    MIN_TERMINATION_COST ///< Multi-factor cost heuristic balancing priority, CPU time, held resources, preemption
};

/**
 * @brief Human-readable string representation of VictimSelectionPolicy.
 */
inline std::string victimPolicyToString(VictimSelectionPolicy policy) {
    switch (policy) {
        case VictimSelectionPolicy::MANUAL:              return "MANUAL";
        case VictimSelectionPolicy::LOWEST_PRIORITY:     return "LOWEST_PRIORITY";
        case VictimSelectionPolicy::HIGHEST_ALLOCATION:  return "HIGHEST_ALLOCATION";
        case VictimSelectionPolicy::MIN_TERMINATION_COST: return "MIN_TERMINATION_COST";
        default:                                         return "UNKNOWN";
    }
}

/**
 * @brief Overall status of the deadlock recovery operation.
 */
enum class RecoveryStatus {
    RECOVERY_NOT_NEEDED,  ///< Initial detection found no deadlock
    RECOVERY_SUCCESSFUL,  ///< Deadlock was completely resolved; final state is deadlock-free
    RECOVERY_PARTIAL,     ///< Termination performed, but deadlock still persists (single-step mode)
    RECOVERY_FAILED       ///< Recovery could not proceed due to invalid victim, invalid state, or error
};

/**
 * @brief Human-readable string representation of RecoveryStatus.
 */
inline std::string recoveryStatusToString(RecoveryStatus status) {
    switch (status) {
        case RecoveryStatus::RECOVERY_NOT_NEEDED: return "RECOVERY_NOT_NEEDED";
        case RecoveryStatus::RECOVERY_SUCCESSFUL: return "RECOVERY_SUCCESSFUL";
        case RecoveryStatus::RECOVERY_PARTIAL:    return "RECOVERY_PARTIAL";
        case RecoveryStatus::RECOVERY_FAILED:     return "RECOVERY_FAILED";
        default:                                  return "UNKNOWN";
    }
}

/**
 * @brief Diagnostic record of a single victim termination step.
 */
struct RecoveryStep {
    int stepNumber{0};
    int victimProcessId{-1};
    std::string victimProcessName;
    int victimPriority{0};
    std::vector<int> releasedResources;          ///< Resources reclaimed and returned to Available
    std::vector<int> deadlockedSetBefore;        ///< Set D prior to termination
    std::vector<int> deadlockedSetAfter;         ///< Set D immediately following termination
    detection::DetectionResult detectionAfter;   ///< Detection result after this step
    std::string details;
};

/**
 * @brief Structured outcome and diagnostic report returned after a recovery operation.
 */
struct RecoveryResult {
    RecoveryStatus status{RecoveryStatus::RECOVERY_NOT_NEEDED};
    bool success{false};                         ///< True if recovery achieved a deadlock-free state or wasn't needed
    std::string message;
    VictimSelectionPolicy policyUsed{VictimSelectionPolicy::MANUAL};
    detection::DetectionResult initialDetection;
    detection::DetectionResult finalDetection;
    std::vector<RecoveryStep> steps;
    std::vector<int> totalResourcesReclaimed;    ///< Component-wise sum of all resources released
    std::vector<int> terminatedProcessIds;       ///< List of all victims terminated during recovery
    std::string diagnosticReport;                ///< Formatted human-readable report
};

/**
 * @brief Deadlock Recovery Engine.
 * 
 * Implements victim selection heuristics, process termination, resource reclamation,
 * repeated multi-instance detection, and authoritative state consistency enforcement.
 */
class RecoveryEngine {
public:
    /**
     * @brief Execute a single manual recovery step by terminating a specific victim.
     * 
     * Validates that:
     * - System is currently deadlocked.
     * - Specified victimProcessId exists, is not already terminated, and is an active member
     *   of the deadlocked candidate set D.
     * If valid:
     * - Terminates the process, reclaims held resources to Available, clears requests.
     * - Re-runs detection to evaluate new state.
     * 
     * @param manager Reference to authoritative ResourceManager
     * @param victimProcessId ID of the victim to terminate
     * @return Structured RecoveryResult containing step diagnostics and updated state
     */
    static RecoveryResult recoverManual(core::ResourceManager& manager, int victimProcessId);

    /**
     * @brief Automatically select victims and iterate termination until deadlock is resolved.
     * 
     * @param manager Reference to authoritative ResourceManager
     * @param policy Heuristic used to select victims (LOWEST_PRIORITY, HIGHEST_ALLOCATION, MIN_TERMINATION_COST)
     * @param maxIterations Safety limit to avoid infinite loops
     * @return Structured RecoveryResult with complete sequence of steps and final state
     */
    static RecoveryResult recoverAuto(core::ResourceManager& manager, 
                                      VictimSelectionPolicy policy = VictimSelectionPolicy::LOWEST_PRIORITY,
                                      int maxIterations = 50);

    /**
     * @brief Select a victim process from a deadlocked set according to the specified policy.
     * @param state Current system state snapshot
     * @param deadlockedSet Active deadlocked process IDs (D)
     * @param policy Selection strategy
     * @return Chosen process ID, or -1 if no eligible candidate found
     */
    static int selectVictim(const core::SystemState& state, 
                            const std::vector<int>& deadlockedSet, 
                            VictimSelectionPolicy policy);

    /**
     * @brief Calculate the heuristic termination cost for a process.
     * 
     * Formula: Cost(P) = (Priority * 10) + (CpuTime * 2) - (TotalHeldResources * 3) - (PreemptionCount * 5)
     * Lower cost indicates a more desirable candidate for termination.
     */
    static double calculateTerminationCost(const core::SystemState& state, int processId);

    /**
     * @brief Document why resource preemption requires transactional rollback and is scheduled as future work.
     */
    static std::string explainPreemptionRequirements();

private:
    static std::string vectorToString(const std::vector<int>& vec);
    static std::string formatRecoveryReport(const RecoveryResult& res);
};

} // namespace deadlock::recovery
