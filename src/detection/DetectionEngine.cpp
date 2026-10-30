#include "detection/DetectionEngine.hpp"

#include <sstream>
#include <iomanip>

namespace deadlock::detection {

std::string DetectionEngine::vectorToString(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i + 1 < vec.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

DetectionResult DetectionEngine::detectDeadlock(const core::SystemState& state) {
    DetectionResult result;

    // Step 1: Formally verify system state invariants
    std::string invariantErr;
    if (!state.verifyInvariants(invariantErr)) {
        result.validState = false;
        result.stateError = invariantErr;
        result.isDeadlocked = false;
        result.diagnosticReport =
            "========================================================\n"
            " DEADLOCK DETECTION ERROR: INVALID SYSTEM STATE\n"
            "========================================================\n"
            "The system state violates formal Operating Systems invariants:\n"
            "  " + invariantErr + "\n"
            "Detection aborted to prevent false conclusions on corrupted data.\n"
            "========================================================\n";
        return result;
    }

    result.validState = true;
    const int n = state.numProcesses();
    const int m = state.numResources();

    if (n == 0) {
        result.isDeadlocked = false;
        result.finalWork = state.available;
        result.diagnosticReport = "System has 0 processes. No deadlock possible.";
        return result;
    }

    // Step 2: Initialize Work = Available
    std::vector<int> Work = state.available;

    // Step 3: Initialize Finish array
    // Finish[i] = true if Allocation[i] == 0 (cannot participate in deadlock); false otherwise
    std::vector<bool> Finish(n, false);
    for (int i = 0; i < n; ++i) {
        bool hasAllocation = false;
        for (int j = 0; j < m; ++j) {
            if (state.allocation[i][j] > 0) {
                hasAllocation = true;
                break;
            }
        }

        if (!hasAllocation) {
            Finish[i] = true;
            result.reducibleProcessIds.push_back(i);
            result.reductionSteps.push_back(
                "Process P" + std::to_string(i) + " (" + state.processes[i].name +
                ") holds 0 resources -> Initialized Finish[" + std::to_string(i) + "] = true."
            );
        }
    }

    // Step 4: Matrix Reduction Loop
    bool progress = true;
    while (progress) {
        progress = false;
        for (int i = 0; i < n; ++i) {
            if (!Finish[i]) {
                // Check if Request[i] <= Work component-wise
                bool canSatisfy = true;
                for (int j = 0; j < m; ++j) {
                    if (state.request[i][j] > Work[j]) {
                        canSatisfy = false;
                        break;
                    }
                }

                if (canSatisfy) {
                    std::ostringstream stepLog;
                    stepLog << "Process P" << i << " (" << state.processes[i].name << ") Request "
                            << vectorToString(state.request[i]) << " <= Work " << vectorToString(Work)
                            << ". Simulated completion; reclaimed " << vectorToString(state.allocation[i]);

                    // Simulate process completion: Work += Allocation[i]
                    for (int j = 0; j < m; ++j) {
                        Work[j] += state.allocation[i][j];
                    }

                    stepLog << " -> New Work: " << vectorToString(Work);
                    result.reductionSteps.push_back(stepLog.str());

                    Finish[i] = true;
                    result.reducibleProcessIds.push_back(i);
                    progress = true;
                    break; // Restart scan to ensure deterministic reduction order
                }
            }
        }
    }

    // Step 5: Identify processes that could not finish
    for (int i = 0; i < n; ++i) {
        if (!Finish[i]) {
            result.deadlockedProcessIds.push_back(i);
        }
    }

    result.isDeadlocked = !result.deadlockedProcessIds.empty();
    result.finalWork = Work;

    // Step 6: Generate comprehensive diagnostic report
    std::ostringstream report;
    report << "========================================================\n";
    report << "     DEADLOCK DETECTION REPORT (Multi-Instance Model)\n";
    report << "========================================================\n";
    report << "Initial Available Work: " << vectorToString(state.available) << "\n";
    report << "Final Unallocated Work: " << vectorToString(result.finalWork) << "\n\n";

    report << "Reduction Trace (" << result.reductionSteps.size() << " steps):\n";
    for (size_t s = 0; s < result.reductionSteps.size(); ++s) {
        report << "  [" << (s + 1) << "] " << result.reductionSteps[s] << "\n";
    }

    if (result.isDeadlocked) {
        report << "\n[RESULT] DEADLOCK DETECTED! Deadlocked Process Set D:\n";
        for (int pId : result.deadlockedProcessIds) {
            report << "  - Process P" << pId << " (" << state.processes[pId].name << ")\n"
                   << "      Holding:   " << vectorToString(state.allocation[pId]) << "\n"
                   << "      Requested: " << vectorToString(state.request[pId]) << "\n";
        }
        report << "\nDiagnostic Explanation:\n"
               << "  Blocked processes alone do not necessarily imply a deadlock.\n"
               << "  However, for the processes listed above in set D, their outstanding requests\n"
               << "  cannot be satisfied by current unallocated resources nor by any subsequent\n"
               << "  completion of other processes. A permanent circular wait dependency exists.\n";
    } else {
        report << "\n[RESULT] NO DEADLOCK DETECTED.\n"
               << "All processes holding resources can either run immediately or eventually\n"
               << "complete once preceding processes finish and release their allocations.\n";
    }
    report << "========================================================\n";

    result.diagnosticReport = report.str();
    return result;
}

} // namespace deadlock::detection
