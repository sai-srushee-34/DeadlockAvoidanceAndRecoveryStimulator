#pragma once

#include "Process.hpp"
#include "ResourceType.hpp"
#include "SystemState.hpp"
#include "prevention/PreventionEngine.hpp"
#include "detection/DetectionEngine.hpp"
#include "recovery/RecoveryEngine.hpp"

#include <string>
#include <vector>
#include <memory>

namespace deadlock::core {

/**
 * @brief Return status structure for resource allocation operations.
 */
struct OperationResult {
    bool success{false};
    std::string message;

    static OperationResult Ok(const std::string& msg = "") {
        return {true, msg};
    }

    static OperationResult Fail(const std::string& msg) {
        return {false, msg};
    }
};

/**
 * @brief Record of a rejected resource request and the exact policy violation reason.
 */
struct RejectionRecord {
    int stepNumber{0};
    int processId{0};
    std::string processName;
    std::vector<int> requestVector;
    std::string policyName;
    std::string reason;
};

/**
 * @brief Authoritative resource manager and simulator engine core.
 * 
 * Manages processes, resources, matrices, invariant validation,
 * prevention policies, request queuing, allocation, release, and process completion.
 */
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager() = default;

    // --- Configuration & Entity Registration ---

    /**
     * @brief Define and register a new resource type.
     * @param name Descriptive label (e.g., "CPU", "Printer")
     * @param totalCapacity Total instances available in system (must be >= 1)
     * @param orderRank Global priority ranking for prevention protocols
     * @return Generated resource ID (0-indexed) or -1 on error.
     */
    int addResourceType(const std::string& name, int totalCapacity, int orderRank = 0);

    /**
     * @brief Register a new process in the system.
     * @param name Process name/tag (e.g., "P0", "Worker")
     * @param priority Priority rank (1 to 10)
     * @param maxDemand Optional maximum demand claim vector (length must match resources)
     * @return Generated process ID (0-indexed) or -1 on error.
     */
    int addProcess(const std::string& name, int priority = 5, const std::vector<int>& maxDemand = {});

    /**
     * @brief Enable or disable strict validation of requests against max demand claims.
     */
    void enableMaxDemandValidation(bool enable);

    /**
     * @brief Check whether max demand validation is active.
     */
    bool isMaxDemandValidationEnabled() const;

    /**
     * @brief Set the active Deadlock Prevention policy.
     */
    void setPreventionPolicy(prevention::PreventionPolicy policy);

    /**
     * @brief Get the currently active Deadlock Prevention policy.
     */
    prevention::PreventionPolicy getPreventionPolicy() const;

    /**
     * @brief Reset system to empty state.
     */
    void reset();

    /**
     * @brief Clear all processes and resources (alias for reset, initializes blank sandbox).
     */
    void clearAll();

    /**
     * @brief Delete a process entirely from the system.
     * Reclaims all held resources, cleans up matrix rows, and re-indexes remaining processes.
     * @param processId ID of the process to remove
     * @return true if process existed and was removed; false otherwise.
     */
    bool deleteProcess(int processId);

    /**
     * @brief Delete a resource class entirely from the system.
     * Removes capacity, updates process allocations/requests, and re-indexes remaining resources.
     * @param resourceId ID of the resource class to remove
     * @return true if resource existed and was removed; false otherwise.
     */
    bool deleteResourceType(int resourceId);

    /**
     * @brief Clear all processes from the system and reclaim all allocated resources.
     */
    void clearProcesses();

    /**
     * @brief Clear all resource types from the system.
     */
    void clearResources();

    // --- Core Lifecycle & Resource Mutators ---

    /**
     * @brief Submit a resource request on behalf of a process.
     * 
     * If prevention policy is active, validates request before allocation.
     * If resources are available and pass validation, they are allocated immediately.
     * If resources are unavailable, the request is recorded in the Request matrix
     * and the process transitions to WAITING.
     */
    OperationResult submitRequest(int processId, const std::vector<int>& requestVector);

    /**
     * @brief Directly allocate resources if available (bypasses request queue if valid).
     */
    OperationResult allocateDirect(int processId, const std::vector<int>& units);

    /**
     * @brief Release a specified subset of currently held resources back to Available.
     */
    OperationResult releaseResources(int processId, const std::vector<int>& releaseVector);

    /**
     * @brief Mark a process as completed (TERMINATED), reclaiming all held resources
     * and clearing outstanding requests.
     */
    OperationResult completeProcess(int processId);

    /**
     * @brief Terminate a process as part of deadlock recovery, reclaiming all held resources.
     * @param processId ID of the victim to terminate
     * @param outReclaimed Filled with vector of reclaimed resource units
     * @return OperationResult indicating success or failure
     */
    OperationResult terminateProcess(int processId, std::vector<int>& outReclaimed);

    /**
     * @brief Attempt to satisfy an existing queued request for a waiting process.
     */
    OperationResult grantPendingRequest(int processId);

    /**
     * @brief Scan all waiting processes and grant pending requests that can now be satisfied.
     * @return Number of pending requests successfully satisfied.
     */
    int processPendingRequests();

    /**
     * @brief Execute manual deadlock recovery by terminating the specified victim process.
     */
    recovery::RecoveryResult recoverDeadlockManual(int victimProcessId);

    /**
     * @brief Execute automated deadlock recovery using a specified victim selection heuristic.
     */
    recovery::RecoveryResult recoverDeadlockAuto(recovery::VictimSelectionPolicy policy = recovery::VictimSelectionPolicy::LOWEST_PRIORITY);

    // --- Inspection, Verification & Formatting ---

    /**
     * @brief Get immutable reference to current system state.
     */
    const SystemState& getState() const;

    /**
     * @brief Execute multi-instance deadlock detection algorithm on current state.
     */
    detection::DetectionResult detectDeadlock() const;

    /**
     * @brief Validate that all system invariants hold.
     */
    bool verifyInvariants(std::string& outError) const;

    /**
     * @brief Access historical log of all rejected requests.
     */
    const std::vector<RejectionRecord>& getRejectionLog() const;

    /**
     * @brief Produce human-readable summary of process states and available resources.
     */
    std::string formatState() const;

    /**
     * @brief Produce detailed tabular view of Allocation, Request, Max, and Need matrices.
     */
    std::string formatMatrices() const;

    /**
     * @brief Produce human-readable log of rejected requests.
     */
    std::string formatRejections() const;

private:
    SystemState state_;
    prevention::PreventionPolicy preventionPolicy_{prevention::PreventionPolicy::NONE};
    std::vector<RejectionRecord> rejectionLog_;

    bool isValidProcessId(int processId, std::string& outError) const;
    bool isValidResourceVector(const std::vector<int>& vec, std::string& outError) const;
    void recordRejection(int processId, const std::vector<int>& requestVector, const std::string& reason);
    static std::string vectorToString(const std::vector<int>& vec);
};

} // namespace deadlock::core
