#pragma once

#include "core/SystemState.hpp"

#include <string>
#include <vector>

namespace deadlock::prevention {

/**
 * @brief Configurable deadlock prevention policies.
 * 
 * Unlike Avoidance (which requires a priori maximum claim matrices and evaluates
 * runtime state safety sequences), Prevention statically constrains the resource
 * allocation protocol so that at least one Coffman condition is mathematically
 * impossible to satisfy.
 */
enum class PreventionPolicy {
    NONE,                       ///< Standard allocation without prevention restrictions
    HOLD_AND_WAIT_CONSERVATIVE, ///< Breaks Hold & Wait: Process must request all resources at once; no incremental requests allowed
    CIRCULAR_WAIT_TOTAL_ORDER   ///< Breaks Circular Wait: Process can only request resources in strictly increasing order rank F(R)
};

/**
 * @brief Convert PreventionPolicy enum to human-readable string.
 */
inline std::string preventionPolicyToString(PreventionPolicy policy) {
    switch (policy) {
        case PreventionPolicy::NONE:
            return "NONE (Unconstrained)";
        case PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE:
            return "HOLD_AND_WAIT_CONSERVATIVE (All-or-Nothing Upfront Acquisition)";
        case PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER:
            return "CIRCULAR_WAIT_TOTAL_ORDER (Strict Hierarchical Ordering F(R))";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Outcome of a prevention policy validation check.
 */
struct PreventionCheckResult {
    bool allowed{true};
    std::string reason;

    static PreventionCheckResult Allow() {
        return {true, ""};
    }

    static PreventionCheckResult Reject(const std::string& reason) {
        return {false, reason};
    }
};

/**
 * @brief Static Deadlock Prevention verification engine.
 */
class PreventionEngine {
public:
    /**
     * @brief Dispatch request validation to the active prevention policy.
     * @param state Immutable system state snapshot
     * @param processId Identifier of the requesting process
     * @param requestVector Proposed resource request vector
     * @param policy Active prevention policy
     * @return PreventionCheckResult indicating whether the request satisfies the policy protocol
     */
    static PreventionCheckResult validateRequest(
        const core::SystemState& state,
        int processId,
        const std::vector<int>& requestVector,
        PreventionPolicy policy
    );

    /**
     * @brief Validate that the request satisfies the Hold-and-Wait prevention protocol.
     * 
     * Rule: A process may only request resources if it currently holds zero resources.
     * If it holds any resources, subsequent incremental requests are rejected.
     */
    static PreventionCheckResult validateHoldAndWait(
        const core::SystemState& state,
        int processId,
        const std::vector<int>& requestVector
    );

    /**
     * @brief Validate that the request satisfies the Circular-Wait prevention protocol.
     * 
     * Rule: All requested resource types must have an order rank strictly greater than
     * the highest order rank of any resource type the process currently holds:
     * F(R_requested) > max { F(R_held) }.
     */
    static PreventionCheckResult validateCircularWait(
        const core::SystemState& state,
        int processId,
        const std::vector<int>& requestVector
    );

    /**
     * @brief Verify that all configured resource types have unique, non-colliding order ranks.
     */
    static bool verifyUniqueResourceOrdering(
        const core::SystemState& state,
        std::string& outError
    );
};

} // namespace deadlock::prevention
