#include "prevention/PreventionEngine.hpp"

#include <sstream>
#include <unordered_set>

namespace deadlock::prevention {

namespace {

std::string vectorToString(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i + 1 < vec.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

} // anonymous namespace

PreventionCheckResult PreventionEngine::validateRequest(
    const core::SystemState& state,
    int processId,
    const std::vector<int>& requestVector,
    PreventionPolicy policy
) {
    switch (policy) {
        case PreventionPolicy::NONE:
            return PreventionCheckResult::Allow();

        case PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE:
            return validateHoldAndWait(state, processId, requestVector);

        case PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER:
            return validateCircularWait(state, processId, requestVector);

        default:
            return PreventionCheckResult::Allow();
    }
}

PreventionCheckResult PreventionEngine::validateHoldAndWait(
    const core::SystemState& state,
    int processId,
    const std::vector<int>& requestVector
) {
    if (processId < 0 || processId >= state.numProcesses()) {
        return PreventionCheckResult::Reject("Invalid process ID: " + std::to_string(processId));
    }

    const int m = state.numResources();

    // 1. Check whether process currently holds any resource instances
    int totalHeld = 0;
    std::ostringstream heldDetails;
    for (int j = 0; j < m; ++j) {
        int heldCount = state.allocation[processId][j];
        if (heldCount > 0) {
            if (totalHeld > 0) heldDetails << ", ";
            heldDetails << heldCount << " of '" << state.resources[j].name << "'";
            totalHeld += heldCount;
        }
    }

    if (totalHeld > 0) {
        return PreventionCheckResult::Reject(
            "Hold-and-Wait violation: Process P" + std::to_string(processId) +
            " currently holds " + std::to_string(totalHeld) + " resource unit(s) (" +
            heldDetails.str() + "). Under the Conservative Hold-and-Wait prevention protocol, "
            "processes must acquire all needed resources upfront; incremental requests during "
            "execution are strictly prohibited."
        );
    }

    // 2. If maximum demand model is enabled, verify the process requests its complete claim
    if (state.maxDemandModelEnabled) {
        bool isFullClaim = true;
        for (int j = 0; j < m; ++j) {
            if (requestVector[j] != state.maxDemand[processId][j]) {
                isFullClaim = false;
                break;
            }
        }

        if (!isFullClaim) {
            return PreventionCheckResult::Reject(
                "Hold-and-Wait violation: Process P" + std::to_string(processId) +
                " submitted partial request " + vectorToString(requestVector) +
                " instead of full upfront claim " + vectorToString(state.maxDemand[processId]) +
                ". All resources must be acquired together in a single request."
            );
        }
    }

    return PreventionCheckResult::Allow();
}

PreventionCheckResult PreventionEngine::validateCircularWait(
    const core::SystemState& state,
    int processId,
    const std::vector<int>& requestVector
) {
    if (processId < 0 || processId >= state.numProcesses()) {
        return PreventionCheckResult::Reject("Invalid process ID: " + std::to_string(processId));
    }

    const int m = state.numResources();

    // 1. Identify the highest order rank among all resources currently held by the process
    int highestHeldRank = 0;
    std::string highestHeldResName = "";

    for (int j = 0; j < m; ++j) {
        if (state.allocation[processId][j] > 0) {
            int rank = state.resources[j].orderRank;
            if (rank > highestHeldRank) {
                highestHeldRank = rank;
                highestHeldResName = state.resources[j].name;
            }
        }
    }

    // 2. Validate that EVERY requested resource has an order rank strictly higher than highestHeldRank
    for (int j = 0; j < m; ++j) {
        if (requestVector[j] > 0) {
            int reqRank = state.resources[j].orderRank;
            if (reqRank <= highestHeldRank) {
                return PreventionCheckResult::Reject(
                    "Circular-Wait violation: Process P" + std::to_string(processId) +
                    " currently holds '" + highestHeldResName + "' (rank " +
                    std::to_string(highestHeldRank) + "), but requested '" +
                    state.resources[j].name + "' with rank " + std::to_string(reqRank) +
                    " <= " + std::to_string(highestHeldRank) +
                    ". All resource requests must strictly follow increasing order rank F(R)."
                );
            }
        }
    }

    return PreventionCheckResult::Allow();
}

bool PreventionEngine::verifyUniqueResourceOrdering(
    const core::SystemState& state,
    std::string& outError
) {
    std::unordered_set<int> seenRanks;
    for (const auto& res : state.resources) {
        if (seenRanks.find(res.orderRank) != seenRanks.end()) {
            outError = "Ordering error: Duplicate resource rank " + std::to_string(res.orderRank) +
                       " assigned to resource '" + res.name + "'. All ranks must be strictly unique.";
            return false;
        }
        seenRanks.insert(res.orderRank);
    }
    return true;
}

} // namespace deadlock::prevention
