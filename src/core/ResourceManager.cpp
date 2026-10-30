#include "core/ResourceManager.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace deadlock::core {

ResourceManager::ResourceManager() {
    reset();
}

void ResourceManager::reset() {
    state_.simulationStep = 0;
    state_.maxDemandModelEnabled = false;
    state_.resources.clear();
    state_.processes.clear();
    state_.available.clear();
    state_.allocation.clear();
    state_.request.clear();
    state_.maxDemand.clear();
    state_.need.clear();
    preventionPolicy_ = prevention::PreventionPolicy::NONE;
    rejectionLog_.clear();
}

void ResourceManager::clearAll() {
    reset();
}

bool ResourceManager::deleteProcess(int processId) {
    if (processId < 0 || processId >= static_cast<int>(state_.processes.size())) {
        return false;
    }

    // 1. Reclaim any allocated resources held by this process
    for (int j = 0; j < state_.numResources(); ++j) {
        state_.available[j] += state_.allocation[processId][j];
    }

    // 2. Erase row from matrices and process vector
    state_.processes.erase(state_.processes.begin() + processId);
    state_.allocation.erase(state_.allocation.begin() + processId);
    state_.request.erase(state_.request.begin() + processId);
    state_.maxDemand.erase(state_.maxDemand.begin() + processId);
    state_.need.erase(state_.need.begin() + processId);

    // 3. Re-index IDs of remaining processes
    for (size_t i = 0; i < state_.processes.size(); ++i) {
        state_.processes[i].id = static_cast<int>(i);
    }

    state_.simulationStep++;
    return true;
}

bool ResourceManager::deleteResourceType(int resourceId) {
    if (resourceId < 0 || resourceId >= static_cast<int>(state_.resources.size())) {
        return false;
    }

    // 1. Remove resource definition and available entry
    state_.resources.erase(state_.resources.begin() + resourceId);
    state_.available.erase(state_.available.begin() + resourceId);

    // 2. Erase column resourceId from every process matrix row
    for (auto& row : state_.allocation) {
        if (resourceId < static_cast<int>(row.size())) {
            row.erase(row.begin() + resourceId);
        }
    }
    for (auto& row : state_.request) {
        if (resourceId < static_cast<int>(row.size())) {
            row.erase(row.begin() + resourceId);
        }
    }
    for (auto& row : state_.maxDemand) {
        if (resourceId < static_cast<int>(row.size())) {
            row.erase(row.begin() + resourceId);
        }
    }
    for (auto& row : state_.need) {
        if (resourceId < static_cast<int>(row.size())) {
            row.erase(row.begin() + resourceId);
        }
    }

    // 3. Re-index IDs of remaining resources
    for (size_t j = 0; j < state_.resources.size(); ++j) {
        state_.resources[j].id = static_cast<int>(j);
    }

    state_.simulationStep++;
    return true;
}

void ResourceManager::clearProcesses() {
    const int m = state_.numResources();
    for (size_t i = 0; i < state_.processes.size(); ++i) {
        for (int j = 0; j < m; ++j) {
            state_.available[j] += state_.allocation[i][j];
        }
    }

    state_.processes.clear();
    state_.allocation.clear();
    state_.request.clear();
    state_.maxDemand.clear();
    state_.need.clear();

    state_.simulationStep++;
}

void ResourceManager::clearResources() {
    state_.resources.clear();
    state_.available.clear();

    for (auto& row : state_.allocation) row.clear();
    for (auto& row : state_.request) row.clear();
    for (auto& row : state_.maxDemand) row.clear();
    for (auto& row : state_.need) row.clear();

    state_.simulationStep++;
}


int ResourceManager::addResourceType(const std::string& name, int totalCapacity, int orderRank) {
    if (totalCapacity < 1) {
        return -1;
    }

    int resId = static_cast<int>(state_.resources.size());
    ResourceType res;
    res.id = resId;
    res.name = name;
    res.totalCapacity = totalCapacity;
    res.orderRank = (orderRank > 0) ? orderRank : (resId + 1);

    state_.resources.push_back(res);
    state_.available.push_back(totalCapacity);

    // Expand columns in all existing process rows
    for (auto& row : state_.allocation) row.push_back(0);
    for (auto& row : state_.request)    row.push_back(0);
    for (auto& row : state_.maxDemand)  row.push_back(0);
    for (auto& row : state_.need)       row.push_back(0);

    return resId;
}

int ResourceManager::addProcess(const std::string& name, int priority, const std::vector<int>& maxDemand) {
    const int m = state_.numResources();

    if (!maxDemand.empty() && static_cast<int>(maxDemand.size()) != m) {
        return -1;
    }

    std::vector<int> sanitizedDemand = maxDemand;
    if (!sanitizedDemand.empty()) {
        for (int j = 0; j < static_cast<int>(sanitizedDemand.size()); ++j) {
            if (sanitizedDemand[j] < 0) sanitizedDemand[j] = 0;
            if (sanitizedDemand[j] > state_.resources[j].totalCapacity) {
                sanitizedDemand[j] = state_.resources[j].totalCapacity;
            }
        }
    }

    int procId = static_cast<int>(state_.processes.size());
    Process proc;
    proc.id = procId;
    proc.name = (name.empty()) ? ("P" + std::to_string(procId)) : name;
    proc.priority = std::clamp(priority, 1, 10);
    proc.state = ProcessState::READY;
    proc.cpuTimeUsed = 0;
    proc.remainingTime = 10;
    proc.preemptionCount = 0;

    state_.processes.push_back(proc);

    state_.allocation.push_back(std::vector<int>(m, 0));
    state_.request.push_back(std::vector<int>(m, 0));

    if (!sanitizedDemand.empty()) {
        state_.maxDemand.push_back(sanitizedDemand);
        state_.need.push_back(sanitizedDemand); // Allocation is 0, so Need = Max
    } else {
        state_.maxDemand.push_back(std::vector<int>(m, 0));
        state_.need.push_back(std::vector<int>(m, 0));
    }

    return procId;
}

void ResourceManager::enableMaxDemandValidation(bool enable) {
    state_.maxDemandModelEnabled = enable;
}

bool ResourceManager::isMaxDemandValidationEnabled() const {
    return state_.maxDemandModelEnabled;
}

void ResourceManager::setPreventionPolicy(prevention::PreventionPolicy policy) {
    preventionPolicy_ = policy;
}

prevention::PreventionPolicy ResourceManager::getPreventionPolicy() const {
    return preventionPolicy_;
}

bool ResourceManager::isValidProcessId(int processId, std::string& outError) const {
    if (processId < 0 || processId >= state_.numProcesses()) {
        outError = "Invalid process ID: " + std::to_string(processId) + ". Total processes: " + std::to_string(state_.numProcesses()) + ".";
        return false;
    }
    return true;
}

bool ResourceManager::isValidResourceVector(const std::vector<int>& vec, std::string& outError) const {
    const int m = state_.numResources();
    if (static_cast<int>(vec.size()) != m) {
        outError = "Vector dimension mismatch: got " + std::to_string(vec.size()) + ", expected " + std::to_string(m) + ".";
        return false;
    }
    for (int j = 0; j < m; ++j) {
        if (vec[j] < 0) {
            outError = "Invalid vector: element at index " + std::to_string(j) + " is negative (" + std::to_string(vec[j]) + ").";
            return false;
        }
    }
    return true;
}

std::string ResourceManager::vectorToString(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i + 1 < vec.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

void ResourceManager::recordRejection(int processId, const std::vector<int>& requestVector, const std::string& reason) {
    RejectionRecord rec;
    rec.stepNumber = state_.simulationStep;
    rec.processId = processId;
    rec.processName = (processId >= 0 && processId < state_.numProcesses()) ? state_.processes[processId].name : "UNKNOWN";
    rec.requestVector = requestVector;
    rec.policyName = prevention::preventionPolicyToString(preventionPolicy_);
    rec.reason = reason;
    rejectionLog_.push_back(rec);
}

OperationResult ResourceManager::submitRequest(int processId, const std::vector<int>& requestVector) {
    std::string err;
    if (!isValidProcessId(processId, err)) {
        recordRejection(processId, requestVector, err);
        return OperationResult::Fail(err);
    }
    if (!isValidResourceVector(requestVector, err)) {
        recordRejection(processId, requestVector, err);
        return OperationResult::Fail(err);
    }

    Process& proc = state_.processes[processId];
    if (proc.state == ProcessState::TERMINATED) {
        std::string termErr = "Cannot submit request: Process P" + std::to_string(processId) + " is TERMINATED.";
        recordRejection(processId, requestVector, termErr);
        return OperationResult::Fail(termErr);
    }

    const int m = state_.numResources();

    // Check for trivial all-zero request
    bool allZero = true;
    for (int val : requestVector) {
        if (val > 0) { allZero = false; break; }
    }
    if (allZero) {
        return OperationResult::Ok("Process P" + std::to_string(processId) + " requested zero resources (no-op).");
    }

    // --- PHASE 3: Deadlock Prevention Policy Check ---
    if (preventionPolicy_ != prevention::PreventionPolicy::NONE) {
        auto prevCheck = prevention::PreventionEngine::validateRequest(state_, processId, requestVector, preventionPolicy_);
        if (!prevCheck.allowed) {
            recordRejection(processId, requestVector, prevCheck.reason);
            return OperationResult::Fail("Prevention Policy Violation: " + prevCheck.reason);
        }
    }

    // Validate against Max Demand / Need if model is enabled
    if (state_.maxDemandModelEnabled) {
        for (int j = 0; j < m; ++j) {
            if (requestVector[j] > state_.need[processId][j]) {
                std::string claimErr = "Request rejected: Process P" + std::to_string(processId) +
                                       " requested " + std::to_string(requestVector[j]) + " of '" + state_.resources[j].name +
                                       "', which exceeds its remaining Need (" + std::to_string(state_.need[processId][j]) + ").";
                recordRejection(processId, requestVector, claimErr);
                return OperationResult::Fail(claimErr);
            }
        }
    }

    // Check whether requested resources are currently available
    bool canAllocateImmediately = true;
    for (int j = 0; j < m; ++j) {
        if (requestVector[j] > state_.available[j]) {
            canAllocateImmediately = false;
            break;
        }
    }

    if (canAllocateImmediately) {
        // Allocate directly and transition to RUNNING
        for (int j = 0; j < m; ++j) {
            state_.available[j] -= requestVector[j];
            state_.allocation[processId][j] += requestVector[j];
            if (state_.maxDemandModelEnabled) {
                state_.need[processId][j] -= requestVector[j];
            }
            state_.request[processId][j] = 0;
        }
        proc.state = ProcessState::RUNNING;
        state_.simulationStep++;

        return OperationResult::Ok(
            "Granted: Allocated " + vectorToString(requestVector) + " to Process P" +
            std::to_string(processId) + " (" + proc.name + "). State is RUNNING."
        );
    } else {
        // Resources are unavailable: DO NOT allocate silently.
        // Record pending request in Request matrix and transition process to WAITING.
        state_.request[processId] = requestVector;
        proc.state = ProcessState::WAITING;
        state_.simulationStep++;

        return OperationResult::Fail(
            "Resources unavailable: Process P" + std::to_string(processId) +
            " requested " + vectorToString(requestVector) + ", but Available is " +
            vectorToString(state_.available) + ". Request queued; process transitioned to WAITING."
        );
    }
}

OperationResult ResourceManager::allocateDirect(int processId, const std::vector<int>& units) {
    std::string err;
    if (!isValidProcessId(processId, err)) return OperationResult::Fail(err);
    if (!isValidResourceVector(units, err)) return OperationResult::Fail(err);

    Process& proc = state_.processes[processId];
    if (proc.state == ProcessState::TERMINATED) {
        return OperationResult::Fail("Cannot allocate to TERMINATED process.");
    }

    const int m = state_.numResources();
    for (int j = 0; j < m; ++j) {
        if (units[j] > state_.available[j]) {
            return OperationResult::Fail("Cannot allocate directly: Insufficient available units for '" + state_.resources[j].name + "'.");
        }
    }

    for (int j = 0; j < m; ++j) {
        state_.available[j] -= units[j];
        state_.allocation[processId][j] += units[j];
        if (state_.maxDemandModelEnabled) {
            state_.need[processId][j] -= units[j];
        }
    }
    proc.state = ProcessState::RUNNING;
    state_.simulationStep++;

    return OperationResult::Ok("Directly allocated " + vectorToString(units) + " to Process P" + std::to_string(processId));
}

OperationResult ResourceManager::releaseResources(int processId, const std::vector<int>& releaseVector) {
    std::string err;
    if (!isValidProcessId(processId, err)) return OperationResult::Fail(err);
    if (!isValidResourceVector(releaseVector, err)) return OperationResult::Fail(err);

    Process& proc = state_.processes[processId];
    if (proc.state == ProcessState::TERMINATED) {
        return OperationResult::Fail("Cannot release resources: Process P" + std::to_string(processId) + " is already TERMINATED.");
    }

    const int m = state_.numResources();

    // Verify process currently holds the resources it attempts to release
    for (int j = 0; j < m; ++j) {
        if (releaseVector[j] > state_.allocation[processId][j]) {
            std::string relErr = "Invalid release: Process P" + std::to_string(processId) +
                                 " attempted to release " + std::to_string(releaseVector[j]) + " of '" +
                                 state_.resources[j].name + "', but currently holds only " +
                                 std::to_string(state_.allocation[processId][j]) + ".";
            recordRejection(processId, releaseVector, relErr);
            return OperationResult::Fail(relErr);
        }
    }

    // Return resources to Available pool
    for (int j = 0; j < m; ++j) {
        state_.available[j] += releaseVector[j];
        state_.allocation[processId][j] -= releaseVector[j];
        if (state_.maxDemandModelEnabled) {
            state_.need[processId][j] += releaseVector[j];
        }
    }

    // Update state based on remaining allocation & pending requests
    int totalRemainingHeld = 0;
    int totalPendingRequests = 0;
    for (int j = 0; j < m; ++j) {
        totalRemainingHeld += state_.allocation[processId][j];
        totalPendingRequests += state_.request[processId][j];
    }

    if (totalPendingRequests > 0) {
        proc.state = ProcessState::WAITING;
    } else if (totalRemainingHeld > 0) {
        proc.state = ProcessState::RUNNING;
    } else {
        proc.state = ProcessState::READY;
    }

    state_.simulationStep++;

    return OperationResult::Ok(
        "Released " + vectorToString(releaseVector) + " from Process P" +
        std::to_string(processId) + ". Process state is now " + processStateToString(proc.state) + "."
    );
}

OperationResult ResourceManager::completeProcess(int processId) {
    std::string err;
    if (!isValidProcessId(processId, err)) return OperationResult::Fail(err);

    Process& proc = state_.processes[processId];
    if (proc.state == ProcessState::TERMINATED) {
        return OperationResult::Fail("Process P" + std::to_string(processId) + " is already completed/TERMINATED.");
    }

    const int m = state_.numResources();
    std::vector<int> reclaimed = state_.allocation[processId];

    // Atomically return all held resources to Available pool
    for (int j = 0; j < m; ++j) {
        state_.available[j] += state_.allocation[processId][j];
        state_.allocation[processId][j] = 0;
        state_.request[processId][j] = 0;
        state_.need[processId][j] = 0;
    }

    proc.state = ProcessState::TERMINATED;
    state_.simulationStep++;

    return OperationResult::Ok(
        "Completed Process P" + std::to_string(processId) + " (" + proc.name +
        "). Reclaimed resources: " + vectorToString(reclaimed) + ". State set to TERMINATED."
    );
}

OperationResult ResourceManager::terminateProcess(int processId, std::vector<int>& outReclaimed) {
    std::string err;
    if (!isValidProcessId(processId, err)) return OperationResult::Fail(err);

    Process& proc = state_.processes[processId];
    if (proc.state == ProcessState::TERMINATED) {
        return OperationResult::Fail("Cannot terminate: Process P" + std::to_string(processId) + " is already TERMINATED.");
    }

    const int m = state_.numResources();
    outReclaimed = state_.allocation[processId];

    // Atomically return all held resources to Available pool (exactly once)
    for (int j = 0; j < m; ++j) {
        state_.available[j] += state_.allocation[processId][j];
        state_.allocation[processId][j] = 0;
        state_.request[processId][j] = 0;
        state_.need[processId][j] = 0;
    }

    proc.state = ProcessState::TERMINATED;
    proc.preemptionCount++;
    state_.simulationStep++;

    return OperationResult::Ok(
        "Terminated Process P" + std::to_string(processId) + " (" + proc.name +
        "). Released resources: " + vectorToString(outReclaimed) + ". State set to TERMINATED."
    );
}

recovery::RecoveryResult ResourceManager::recoverDeadlockManual(int victimProcessId) {
    return recovery::RecoveryEngine::recoverManual(*this, victimProcessId);
}

recovery::RecoveryResult ResourceManager::recoverDeadlockAuto(recovery::VictimSelectionPolicy policy) {
    return recovery::RecoveryEngine::recoverAuto(*this, policy);
}

OperationResult ResourceManager::grantPendingRequest(int processId) {
    std::string err;
    if (!isValidProcessId(processId, err)) return OperationResult::Fail(err);

    Process& proc = state_.processes[processId];
    if (proc.state != ProcessState::WAITING) {
        return OperationResult::Fail("Process P" + std::to_string(processId) + " is not WAITING.");
    }

    const int m = state_.numResources();
    const auto& pendingReq = state_.request[processId];

    for (int j = 0; j < m; ++j) {
        if (pendingReq[j] > state_.available[j]) {
            return OperationResult::Fail("Cannot grant: Resource '" + state_.resources[j].name + "' has " +
                                         std::to_string(state_.available[j]) + " available, but " +
                                         std::to_string(pendingReq[j]) + " requested.");
        }
    }

    // Allocate
    for (int j = 0; j < m; ++j) {
        state_.available[j] -= pendingReq[j];
        state_.allocation[processId][j] += pendingReq[j];
        if (state_.maxDemandModelEnabled) {
            state_.need[processId][j] -= pendingReq[j];
        }
        state_.request[processId][j] = 0;
    }

    proc.state = ProcessState::RUNNING;
    state_.simulationStep++;

    return OperationResult::Ok("Granted pending request " + vectorToString(pendingReq) + " for Process P" + std::to_string(processId));
}

int ResourceManager::processPendingRequests() {
    int grantedCount = 0;
    for (int i = 0; i < state_.numProcesses(); ++i) {
        if (state_.processes[i].state == ProcessState::WAITING) {
            auto res = grantPendingRequest(i);
            if (res.success) {
                grantedCount++;
            }
        }
    }
    return grantedCount;
}

const SystemState& ResourceManager::getState() const {
    return state_;
}

detection::DetectionResult ResourceManager::detectDeadlock() const {
    return detection::DetectionEngine::detectDeadlock(state_);
}

bool ResourceManager::verifyInvariants(std::string& outError) const {
    return state_.verifyInvariants(outError);
}

const std::vector<RejectionRecord>& ResourceManager::getRejectionLog() const {
    return rejectionLog_;
}

std::string ResourceManager::formatState() const {
    std::ostringstream oss;
    oss << "========================================================\n";
    oss << " SYSTEM STATE SNAPSHOT (Step " << state_.simulationStep << ")\n";
    oss << " Active Policy: " << prevention::preventionPolicyToString(preventionPolicy_) << "\n";
    oss << "========================================================\n";
    oss << "Resources (" << state_.numResources() << "):\n";
    for (int j = 0; j < state_.numResources(); ++j) {
        const auto& r = state_.resources[j];
        oss << "  [" << r.id << "] " << std::left << std::setw(12) << r.name
            << " | Capacity: " << std::setw(3) << r.totalCapacity
            << " | Available: " << std::setw(3) << state_.available[j]
            << " | OrderRank: " << r.orderRank << "\n";
    }

    oss << "\nProcesses (" << state_.numProcesses() << "):\n";
    for (int i = 0; i < state_.numProcesses(); ++i) {
        const auto& p = state_.processes[i];
        oss << "  [" << p.id << "] " << std::left << std::setw(10) << p.name
            << " | State: " << std::setw(11) << processStateToString(p.state)
            << " | Priority: " << std::setw(2) << p.priority
            << " | Held: " << vectorToString(state_.allocation[i])
            << " | Waiting: " << vectorToString(state_.request[i]) << "\n";
    }
    oss << "========================================================\n";
    return oss.str();
}

std::string ResourceManager::formatMatrices() const {
    std::ostringstream oss;
    const int n = state_.numProcesses();
    const int m = state_.numResources();

    auto printMatrix = [&](const std::string& title, const std::vector<std::vector<int>>& mat) {
        oss << "--- " << title << " Matrix (" << n << "x" << m << ") ---\n    ";
        for (int j = 0; j < m; ++j) {
            oss << std::right << std::setw(8) << state_.resources[j].name;
        }
        oss << "\n";
        for (int i = 0; i < n; ++i) {
            oss << "P" << std::left << std::setw(3) << i;
            for (int j = 0; j < m; ++j) {
                oss << std::right << std::setw(8) << mat[i][j];
            }
            oss << "  (" << processStateToString(state_.processes[i].state) << ")\n";
        }
        oss << "\n";
    };

    oss << "\nAvailable Vector: " << vectorToString(state_.available) << "\n\n";
    printMatrix("Allocation", state_.allocation);
    printMatrix("Request", state_.request);
    if (state_.maxDemandModelEnabled) {
        printMatrix("Max Demand", state_.maxDemand);
        printMatrix("Remaining Need", state_.need);
    }

    return oss.str();
}

std::string ResourceManager::formatRejections() const {
    std::ostringstream oss;
    oss << "========================================================\n";
    oss << " REJECTION & VIOLATION LOG (" << rejectionLog_.size() << " events)\n";
    oss << "========================================================\n";
    if (rejectionLog_.empty()) {
        oss << "  No requests have been rejected.\n";
    } else {
        for (size_t i = 0; i < rejectionLog_.size(); ++i) {
            const auto& rec = rejectionLog_[i];
            oss << "[" << (i + 1) << "] Step " << rec.stepNumber << " | Process P"
                << rec.processId << " (" << rec.processName << ")\n"
                << "    Requested: " << vectorToString(rec.requestVector) << "\n"
                << "    Policy:    " << rec.policyName << "\n"
                << "    Reason:    " << rec.reason << "\n\n";
        }
    }
    oss << "========================================================\n";
    return oss.str();
}

} // namespace deadlock::core
