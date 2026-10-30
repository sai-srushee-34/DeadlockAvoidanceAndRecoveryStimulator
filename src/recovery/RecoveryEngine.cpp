#include "recovery/RecoveryEngine.hpp"
#include "core/ResourceManager.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <climits>

namespace deadlock::recovery {

RecoveryResult RecoveryEngine::recoverManual(core::ResourceManager& manager, int victimProcessId) {
    RecoveryResult result;
    result.policyUsed = VictimSelectionPolicy::MANUAL;
    const auto& state = manager.getState();
    const int m = state.numResources();
    result.totalResourcesReclaimed.assign(m, 0);

    // Step 1: Run detection on current state
    result.initialDetection = manager.detectDeadlock();

    // Step 2: Validate state consistency
    if (!result.initialDetection.validState) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed: Initial system state is invalid: " + result.initialDetection.stateError;
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    // Step 3: Check if deadlock actually exists
    if (!result.initialDetection.isDeadlocked) {
        result.status = RecoveryStatus::RECOVERY_NOT_NEEDED;
        result.success = true;
        result.message = "No deadlock detected in the system. Recovery operation is unnecessary.";
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    // Step 4: Validate victim selection
    const auto& deadlockedSet = result.initialDetection.deadlockedProcessIds;
    if (victimProcessId < 0 || victimProcessId >= state.numProcesses()) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed: Invalid victim process ID (" + std::to_string(victimProcessId) +
                         "). Process ID is out of range [0, " + std::to_string(state.numProcesses() - 1) + "].";
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    if (state.processes[victimProcessId].state == core::ProcessState::TERMINATED) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed: Process P" + std::to_string(victimProcessId) +
                         " is already TERMINATED and cannot be selected as a victim.";
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    auto it = std::find(deadlockedSet.begin(), deadlockedSet.end(), victimProcessId);
    if (it == deadlockedSet.end()) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed: Process P" + std::to_string(victimProcessId) +
                         " is not a member of the active deadlocked set D = " +
                         vectorToString(deadlockedSet) + ". Only deadlocked processes can be selected for recovery.";
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    // Step 5: Terminate the victim and reclaim resources
    const auto& victimProc = state.processes[victimProcessId];
    std::string victimName = victimProc.name;
    int victimPriority = victimProc.priority;

    std::vector<int> reclaimed;
    auto termRes = manager.terminateProcess(victimProcessId, reclaimed);
    if (!termRes.success) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed during process termination: " + termRes.message;
        result.finalDetection = manager.detectDeadlock();
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    for (int j = 0; j < m; ++j) {
        result.totalResourcesReclaimed[j] += reclaimed[j];
    }
    result.terminatedProcessIds.push_back(victimProcessId);

    // Step 6: Re-run detection on the updated state
    result.finalDetection = manager.detectDeadlock();

    // Record recovery step
    RecoveryStep step;
    step.stepNumber = 1;
    step.victimProcessId = victimProcessId;
    step.victimProcessName = victimName;
    step.victimPriority = victimPriority;
    step.releasedResources = reclaimed;
    step.deadlockedSetBefore = deadlockedSet;
    step.deadlockedSetAfter = result.finalDetection.deadlockedProcessIds;
    step.detectionAfter = result.finalDetection;
    step.details = "Terminated victim Process P" + std::to_string(victimProcessId) +
                   " (" + victimName + "), releasing " + vectorToString(reclaimed) + ".";
    result.steps.push_back(step);

    // Step 7: Assess post-recovery status
    if (!result.finalDetection.isDeadlocked) {
        result.status = RecoveryStatus::RECOVERY_SUCCESSFUL;
        result.success = true;
        result.message = "Deadlock successfully resolved! Terminated victim P" + std::to_string(victimProcessId) +
                         " (" + victimName + ") and reclaimed resources " + vectorToString(reclaimed) + ".";
    } else {
        result.status = RecoveryStatus::RECOVERY_PARTIAL;
        result.success = false;
        result.message = "Terminated victim P" + std::to_string(victimProcessId) +
                         " (" + victimName + "), but deadlock still persists in processes " +
                         vectorToString(result.finalDetection.deadlockedProcessIds) + ".";
    }

    result.diagnosticReport = formatRecoveryReport(result);
    return result;
}

RecoveryResult RecoveryEngine::recoverAuto(core::ResourceManager& manager, 
                                          VictimSelectionPolicy policy,
                                          int maxIterations) {
    RecoveryResult result;
    result.policyUsed = policy;
    const auto& state = manager.getState();
    const int m = state.numResources();
    result.totalResourcesReclaimed.assign(m, 0);

    // Step 1: Run initial detection
    result.initialDetection = manager.detectDeadlock();

    if (!result.initialDetection.validState) {
        result.status = RecoveryStatus::RECOVERY_FAILED;
        result.success = false;
        result.message = "Recovery failed: Initial system state is invalid: " + result.initialDetection.stateError;
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    if (!result.initialDetection.isDeadlocked) {
        result.status = RecoveryStatus::RECOVERY_NOT_NEEDED;
        result.success = true;
        result.message = "No deadlock detected in the system. Recovery operation is unnecessary.";
        result.finalDetection = result.initialDetection;
        result.diagnosticReport = formatRecoveryReport(result);
        return result;
    }

    // Step 2: Automated iterative recovery loop
    int iterations = 0;
    while (iterations < maxIterations) {
        auto currentDetection = manager.detectDeadlock();
        if (!currentDetection.isDeadlocked) {
            // Deadlock resolved!
            result.status = RecoveryStatus::RECOVERY_SUCCESSFUL;
            result.success = true;
            result.finalDetection = currentDetection;
            result.message = "Deadlock successfully resolved after " + std::to_string(result.steps.size()) +
                             " victim termination(s) using " + victimPolicyToString(policy) + " policy.";
            result.diagnosticReport = formatRecoveryReport(result);
            return result;
        }

        const auto& deadlockedSet = currentDetection.deadlockedProcessIds;
        int victimId = selectVictim(manager.getState(), deadlockedSet, policy);
        if (victimId < 0) {
            result.status = RecoveryStatus::RECOVERY_FAILED;
            result.success = false;
            result.message = "Recovery halted: Unable to select an eligible victim process from set " +
                             vectorToString(deadlockedSet) + ".";
            result.finalDetection = currentDetection;
            result.diagnosticReport = formatRecoveryReport(result);
            return result;
        }

        const auto& victimProc = manager.getState().processes[victimId];
        std::string victimName = victimProc.name;
        int victimPriority = victimProc.priority;

        std::vector<int> reclaimed;
        auto termRes = manager.terminateProcess(victimId, reclaimed);
        if (!termRes.success) {
            result.status = RecoveryStatus::RECOVERY_FAILED;
            result.success = false;
            result.message = "Recovery failed during process termination: " + termRes.message;
            result.finalDetection = manager.detectDeadlock();
            result.diagnosticReport = formatRecoveryReport(result);
            return result;
        }

        for (int j = 0; j < m; ++j) {
            result.totalResourcesReclaimed[j] += reclaimed[j];
        }
        result.terminatedProcessIds.push_back(victimId);

        iterations++;
        auto postDetection = manager.detectDeadlock();

        RecoveryStep step;
        step.stepNumber = iterations;
        step.victimProcessId = victimId;
        step.victimProcessName = victimName;
        step.victimPriority = victimPriority;
        step.releasedResources = reclaimed;
        step.deadlockedSetBefore = deadlockedSet;
        step.deadlockedSetAfter = postDetection.deadlockedProcessIds;
        step.detectionAfter = postDetection;
        step.details = "Step " + std::to_string(iterations) + ": Terminated victim P" +
                       std::to_string(victimId) + " (" + victimName + "), reclaimed " +
                       vectorToString(reclaimed) + ". Remaining deadlocked: " +
                       vectorToString(postDetection.deadlockedProcessIds) + ".";
        result.steps.push_back(step);

        if (!postDetection.isDeadlocked) {
            result.status = RecoveryStatus::RECOVERY_SUCCESSFUL;
            result.success = true;
            result.finalDetection = postDetection;
            result.message = "Deadlock successfully resolved after " + std::to_string(result.steps.size()) +
                             " victim termination(s) using " + victimPolicyToString(policy) + " policy.";
            result.diagnosticReport = formatRecoveryReport(result);
            return result;
        }
    }

    // If limit reached without resolving deadlock
    result.finalDetection = manager.detectDeadlock();
    result.status = RecoveryStatus::RECOVERY_PARTIAL;
    result.success = false;
    result.message = "Maximum recovery iterations (" + std::to_string(maxIterations) +
                     ") reached, but deadlock persists in processes " +
                     vectorToString(result.finalDetection.deadlockedProcessIds) + ".";
    result.diagnosticReport = formatRecoveryReport(result);
    return result;
}

int RecoveryEngine::selectVictim(const core::SystemState& state, 
                                 const std::vector<int>& deadlockedSet, 
                                 VictimSelectionPolicy policy) {
    if (deadlockedSet.empty()) return -1;

    switch (policy) {
        case VictimSelectionPolicy::LOWEST_PRIORITY: {
            int bestVictim = -1;
            int lowestPriority = INT_MAX;
            int maxHeldResources = -1;

            for (int pid : deadlockedSet) {
                if (pid < 0 || pid >= state.numProcesses()) continue;
                const auto& proc = state.processes[pid];
                if (proc.state == core::ProcessState::TERMINATED) continue;

                int held = 0;
                for (int units : state.allocation[pid]) held += units;

                if (proc.priority < lowestPriority) {
                    lowestPriority = proc.priority;
                    maxHeldResources = held;
                    bestVictim = pid;
                } else if (proc.priority == lowestPriority) {
                    // Tie breaker: process holding more resources
                    if (held > maxHeldResources) {
                        maxHeldResources = held;
                        bestVictim = pid;
                    }
                }
            }
            return bestVictim;
        }

        case VictimSelectionPolicy::HIGHEST_ALLOCATION: {
            int bestVictim = -1;
            int maxHeldResources = -1;
            int lowestPriority = INT_MAX;

            for (int pid : deadlockedSet) {
                if (pid < 0 || pid >= state.numProcesses()) continue;
                const auto& proc = state.processes[pid];
                if (proc.state == core::ProcessState::TERMINATED) continue;

                int held = 0;
                for (int units : state.allocation[pid]) held += units;

                if (held > maxHeldResources) {
                    maxHeldResources = held;
                    lowestPriority = proc.priority;
                    bestVictim = pid;
                } else if (held == maxHeldResources) {
                    // Tie breaker: lower priority
                    if (proc.priority < lowestPriority) {
                        lowestPriority = proc.priority;
                        bestVictim = pid;
                    }
                }
            }
            return bestVictim;
        }

        case VictimSelectionPolicy::MIN_TERMINATION_COST: {
            int bestVictim = -1;
            double minCost = 1e9;

            for (int pid : deadlockedSet) {
                if (pid < 0 || pid >= state.numProcesses()) continue;
                if (state.processes[pid].state == core::ProcessState::TERMINATED) continue;

                double cost = calculateTerminationCost(state, pid);
                if (cost < minCost) {
                    minCost = cost;
                    bestVictim = pid;
                }
            }
            return bestVictim;
        }

        case VictimSelectionPolicy::MANUAL:
        default:
            return deadlockedSet.front();
    }
}

double RecoveryEngine::calculateTerminationCost(const core::SystemState& state, int processId) {
    if (processId < 0 || processId >= state.numProcesses()) return 1e9;
    const auto& proc = state.processes[processId];

    int totalHeld = 0;
    for (int units : state.allocation[processId]) {
        totalHeld += units;
    }

    // Composite cost formula:
    // Higher priority = higher cost to abort (+10 * priority)
    // More CPU time used = more sunk computation lost (+2 * cpuTimeUsed)
    // More resources held = liberates more capacity (-3 * totalHeld)
    // Previously preempted = starvation protection (-5 * preemptionCount)
    double cost = (proc.priority * 10.0) +
                  (proc.cpuTimeUsed * 2.0) -
                  (totalHeld * 3.0) -
                  (proc.preemptionCount * 5.0);

    return cost;
}

std::string RecoveryEngine::explainPreemptionRequirements() {
    std::ostringstream oss;
    oss << "========================================================\n"
        << "  RESOURCE PREEMPTION & ROLLBACK ARCHITECTURAL ANALYSIS\n"
        << "========================================================\n"
        << "In classical Operating Systems design (Silberschatz et al.),\n"
        << "resource preemption is an alternative recovery strategy to process\n"
        << "termination. However, realistic resource preemption requires:\n\n"
        << "1. PREEMPTIBLE RESOURCE CLASSIFICATION:\n"
        << "   Only state-agnostic or virtualization-backed resources (e.g., CPU\n"
        << "   registers, memory pages) can be safely preempted. Physical I/O\n"
        << "   devices (printers, optical drives) and software mutexes guarding\n"
        << "   critical sections are strictly non-preemptible.\n\n"
        << "2. TRANSACTIONAL CHECKPOINT & ROLLBACK:\n"
        << "   To preempt resources from a process without corrupting state, the OS\n"
        << "   must roll back the victim process to an earlier consistent checkpoint\n"
        << "   before the resource was requested or modified. Simulating preemption\n"
        << "   without transactional checkpoints produces unrealistic and corrupted\n"
        << "   system semantics.\n\n"
        << "3. STARVATION PREVENTION & AGING:\n"
        << "   If a process is repeatedly selected as a preemption victim, it will\n"
        << "   starve. The OS must track preemption counts and apply an aging factor\n"
        << "   to increase victim cost with each preemption.\n\n"
        << "CONCLUSION:\n"
        << "In accordance with engineering specifications, process termination is\n"
        << "implemented as the primary authoritative recovery strategy, while full\n"
        << "checkpoint/rollback preemption is scheduled for future work.\n"
        << "========================================================\n";
    return oss.str();
}

std::string RecoveryEngine::vectorToString(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i + 1 < vec.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

std::string RecoveryEngine::formatRecoveryReport(const RecoveryResult& res) {
    std::ostringstream oss;
    oss << "========================================================\n"
        << "             DEADLOCK RECOVERY REPORT\n"
        << "========================================================\n"
        << "Status:      " << recoveryStatusToString(res.status) << "\n"
        << "Policy Used: " << victimPolicyToString(res.policyUsed) << "\n"
        << "Message:     " << res.message << "\n";

    oss << "\nInitial Detection:\n"
        << "  Deadlocked: " << (res.initialDetection.isDeadlocked ? "YES" : "NO") << "\n";
    if (res.initialDetection.isDeadlocked) {
        oss << "  Deadlocked Set D: " << vectorToString(res.initialDetection.deadlockedProcessIds) << "\n";
    }

    if (!res.steps.empty()) {
        oss << "\nRecovery Sequence (" << res.steps.size() << " step(s)):\n";
        for (const auto& step : res.steps) {
            oss << "  Step " << step.stepNumber << ": Victim P" << step.victimProcessId
                << " (" << step.victimProcessName << ", Priority=" << step.victimPriority << ")\n"
                << "    Resources Reclaimed: " << vectorToString(step.releasedResources) << "\n"
                << "    Deadlocked Set Before: " << vectorToString(step.deadlockedSetBefore) << "\n"
                << "    Deadlocked Set After:  " << vectorToString(step.deadlockedSetAfter) << "\n";
        }
        oss << "\nTotal Resources Reclaimed: " << vectorToString(res.totalResourcesReclaimed) << "\n"
            << "Terminated Victims:        " << vectorToString(res.terminatedProcessIds) << "\n";
    }

    oss << "\nFinal Detection:\n"
        << "  Deadlocked: " << (res.finalDetection.isDeadlocked ? "YES" : "NO") << "\n";
    if (res.finalDetection.isDeadlocked) {
        oss << "  Remaining Deadlocked Set D: " << vectorToString(res.finalDetection.deadlockedProcessIds) << "\n";
    } else {
        oss << "  All processes can now complete or have terminated.\n";
    }
    oss << "========================================================\n";

    return oss.str();
}

} // namespace deadlock::recovery
