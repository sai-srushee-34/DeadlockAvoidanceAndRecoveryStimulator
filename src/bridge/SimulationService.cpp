#include "bridge/SimulationService.hpp"
#include "detection/DetectionEngine.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace deadlock::bridge {

SimulationService::SimulationService() {
    setupScenario("standard_safe");
    addEvent("SYSTEM", "C++ Authoritative Simulation Engine initialized. Native bridge active.", "INFO");
}

std::string SimulationService::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&bt, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void SimulationService::addEvent(const std::string& category, const std::string& message, const std::string& severity, JsonValue metadata) {
    SimulationEvent evt;
    evt.id = "evt_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    evt.timestamp = currentTimestamp();
    evt.category = category;
    evt.severity = severity;
    evt.message = message;
    evt.metadata = metadata;

    eventLog_.insert(eventLog_.begin(), evt);
    if (eventLog_.size() > 250) {
        eventLog_.pop_back();
    }
}

JsonValue SimulationService::getEvents() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return StateSerializer::serializeEvents(eventLog_);
}

void SimulationService::clearEvents() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    eventLog_.clear();
    addEvent("SYSTEM", "Kernel audit event log cleared by operator.", "INFO");
}

JsonValue SimulationService::getAvailableScenarios() {
    JsonValue arr;

    auto makeScen = [](const std::string& id, const std::string& name, const std::string& desc) {
        JsonValue o;
        o["id"] = id;
        o["name"] = name;
        o["description"] = desc;
        return o;
    };

    arr.push_back(makeScen("standard_safe", "Standard Safe State (3P, 3R)", "System operating normally with sufficient available headroom. Banker algorithm safe sequence exists: P1 -> P0 -> P2."));
    arr.push_back(makeScen("silberschatz_deadlock", "Silberschatz Injected Deadlock (5P, 3R)", "Classic multi-instance deadlock configuration from Silberschatz OS Concept. P0 can finish, but P1, P2, P3, and P4 enter mutual cyclic wait."));
    arr.push_back(makeScen("circular_wait", "Circular Wait Conflict (2P, 2R)", "Strict cycle violating Havender Condition 4: P0 holds R0 and requests R1; P1 holds R1 and requests R0. Both wait perpetually."));
    arr.push_back(makeScen("hold_wait_starvation", "Hold & Wait Resource Starvation (4P, 3R)", "Partial allocations lock out arriving processes, inducing artificial resource exhaustion under Havender Condition 2 violation."));
    arr.push_back(makeScen("custom_manual", "Manual / Custom Sandbox (Blank Canvas)", "User-defined blank state. Create custom processes, configure custom resources, allocate/request instances, or delete/terminate at will."));

    return arr;
}

void SimulationService::setupScenario(const std::string& scenarioId) {
    engine_.reset();

    if (scenarioId == "custom_manual") {
        activeScenarioId_ = "custom_manual";
        activeScenarioName_ = "Manual / Custom Sandbox (Blank Canvas)";
        activeScenarioDesc_ = "Blank interactive canvas. Register your own processes, define custom resource classes, allocate instances, submit requests, or delete components.";
        // Zero processes, zero resources initially in sandbox
    } else if (scenarioId == "silberschatz_deadlock") {
        activeScenarioId_ = "silberschatz_deadlock";
        activeScenarioName_ = "Silberschatz Injected Deadlock (5P, 3R)";
        activeScenarioDesc_ = "Classic multi-instance deadlock configuration from Silberschatz OS Concept. P0 can finish, but P1, P2, P3, and P4 enter mutual cyclic wait.";

        engine_.addResourceType("Compute Units", 7, 1);
        engine_.addResourceType("Tape Buffers", 2, 2);
        engine_.addResourceType("Print Spoolers", 6, 3);

        int p0 = engine_.addProcess("Init-Daemon", 9);
        int p1 = engine_.addProcess("Compiler-Backend", 4);
        int p2 = engine_.addProcess("Renderer-Thread", 3);
        int p3 = engine_.addProcess("Telemetry-Sink", 2);
        int p4 = engine_.addProcess("Storage-Manager", 6);

        // Initial Allocations
        engine_.allocateDirect(p0, {0, 1, 0});
        engine_.allocateDirect(p1, {2, 0, 0});
        engine_.allocateDirect(p2, {3, 0, 3});
        engine_.allocateDirect(p3, {2, 1, 1});
        engine_.allocateDirect(p4, {0, 0, 2});

        // Contention Requests
        engine_.submitRequest(p1, {2, 0, 2});
        engine_.submitRequest(p2, {0, 0, 1});
        engine_.submitRequest(p3, {1, 0, 0});
        engine_.submitRequest(p4, {0, 0, 2});

    } else if (scenarioId == "circular_wait") {
        activeScenarioId_ = "circular_wait";
        activeScenarioName_ = "Circular Wait Conflict (2P, 2R)";
        activeScenarioDesc_ = "Strict cycle violating Havender Condition 4: P0 holds R0 and requests R1; P1 holds R1 and requests R0. Both wait perpetually.";

        engine_.addResourceType("HSM Vault Lock", 1, 1);
        engine_.addResourceType("Entropy Pool Bus", 1, 2);

        int p0 = engine_.addProcess("Crypto-Signer", 4);
        int p1 = engine_.addProcess("Key-Custodian", 7);

        engine_.allocateDirect(p0, {1, 0});
        engine_.allocateDirect(p1, {0, 1});

        engine_.submitRequest(p0, {0, 1});
        engine_.submitRequest(p1, {1, 0});

    } else if (scenarioId == "hold_wait_starvation") {
        activeScenarioId_ = "hold_wait_starvation";
        activeScenarioName_ = "Hold & Wait Resource Starvation (4P, 3R)";
        activeScenarioDesc_ = "Partial allocations lock out arriving processes, inducing artificial resource exhaustion under Havender Condition 2 violation.";

        engine_.addResourceType("Shared Memory", 6, 1);
        engine_.addResourceType("GPU Acceleration", 2, 2);
        engine_.addResourceType("Network Sockets", 4, 3);

        int p0 = engine_.addProcess("Batch-Job-0", 2);
        int p1 = engine_.addProcess("Batch-Job-1", 5);
        int p2 = engine_.addProcess("Interactive-GUI", 8);
        int p3 = engine_.addProcess("Background-Sync", 1);

        engine_.allocateDirect(p0, {2, 1, 1});
        engine_.allocateDirect(p1, {2, 0, 2});
        engine_.allocateDirect(p2, {1, 1, 0});
        engine_.allocateDirect(p3, {1, 0, 1});

        engine_.submitRequest(p0, {1, 0, 0});
        engine_.submitRequest(p1, {0, 1, 1});
        engine_.submitRequest(p2, {2, 0, 1});
        engine_.submitRequest(p3, {1, 1, 0});

    } else { // standard_safe (default)
        activeScenarioId_ = "standard_safe";
        activeScenarioName_ = "Standard Safe State (3P, 3R)";
        activeScenarioDesc_ = "System operating normally with sufficient available headroom. Banker algorithm safe sequence exists: P1 -> P0 -> P2.";

        engine_.addResourceType("CPU Cores", 10, 1);
        engine_.addResourceType("Memory Pages", 5, 2);
        engine_.addResourceType("NVMe Channels", 7, 3);

        int p0 = engine_.addProcess("Worker-Alpha", 5, {7, 3, 4});
        int p1 = engine_.addProcess("IO-Dispatcher", 8, {3, 2, 2});
        int p2 = engine_.addProcess("DB-Sync-Worker", 3, {9, 4, 5});

        engine_.allocateDirect(p0, {3, 1, 2});
        engine_.allocateDirect(p1, {2, 0, 1});
        engine_.allocateDirect(p2, {3, 2, 2});

        engine_.submitRequest(p0, {0, 1, 0});
        engine_.submitRequest(p1, {1, 0, 1});
        engine_.submitRequest(p2, {0, 0, 1});
    }

    std::string err;
    if (!engine_.verifyInvariants(err)) {
        addEvent("SYSTEM", "Invariant warning during setup: " + err, "WARNING");
    }
}

JsonValue SimulationService::getState() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    JsonValue root = StateSerializer::serializeState(
        engine_.getState(),
        engine_.getPreventionPolicy(),
        activeScenarioId_,
        activeScenarioName_,
        activeScenarioDesc_
    );
    root["availableScenarios"] = getAvailableScenarios();
    return root;
}

JsonValue SimulationService::loadScenario(const std::string& scenarioId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    setupScenario(scenarioId);
    addEvent("SCENARIO", "Loaded scenario \"" + activeScenarioName_ + "\". Invariants verified by C++ engine.", "INFO");
    return getState();
}

JsonValue SimulationService::resetCurrentScenario() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    setupScenario(activeScenarioId_);
    addEvent("SYSTEM", "Reset scenario to initial baseline: " + activeScenarioName_, "WARNING");
    return getState();
}

JsonValue SimulationService::addProcess(const AddProcessParams& params, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (params.name.empty()) {
        outError = "Process name cannot be empty.";
        return JsonValue();
    }
    if (params.priority < 1 || params.priority > 10) {
        outError = "Priority must be between 1 and 10.";
        return JsonValue();
    }

    const auto& curState = engine_.getState();
    const int m = curState.numResources();

    std::vector<int> safeMaxDemand = params.maxDemand;
    if (safeMaxDemand.empty() && m > 0) {
        safeMaxDemand.assign(m, 0);
        for (int j = 0; j < m; ++j) {
            safeMaxDemand[j] = curState.resources[j].totalCapacity;
        }
    } else if (static_cast<int>(safeMaxDemand.size()) == m) {
        for (int j = 0; j < m; ++j) {
            if (safeMaxDemand[j] > curState.resources[j].totalCapacity) {
                safeMaxDemand[j] = curState.resources[j].totalCapacity;
            }
            if (safeMaxDemand[j] < 0) safeMaxDemand[j] = 0;
        }
    }

    // Ensure safeMaxDemand covers initial allocation
    if (static_cast<int>(params.initialAlloc.size()) == m) {
        for (int j = 0; j < m; ++j) {
            if (params.initialAlloc[j] > safeMaxDemand[j]) {
                safeMaxDemand[j] = params.initialAlloc[j];
            }
        }
    }

    int pId = engine_.addProcess(params.name, params.priority, safeMaxDemand);
    if (pId < 0) {
        outError = "Failed to register process in engine (resource dimension mismatch).";
        return JsonValue();
    }

    // Set simulated cpu time
    const_cast<core::Process&>(engine_.getState().processes[pId]).cpuTimeUsed = params.cost / 2;

    // Optional initial allocation
    if (!params.initialAlloc.empty()) {
        auto res = engine_.allocateDirect(pId, params.initialAlloc);
        if (!res.success) {
            engine_.deleteProcess(pId);
            outError = "Initial allocation failed: " + res.message;
            return JsonValue();
        }
    }

    addEvent("PROCESS", "Registered process P" + std::to_string(pId) + " (\"" + params.name + "\", Priority: " + std::to_string(params.priority) + ").", "INFO");
    return getState();
}

JsonValue SimulationService::addResource(const AddResourceParams& params, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (params.name.empty()) {
        outError = "Resource name cannot be empty.";
        return JsonValue();
    }
    if (params.capacity < 1) {
        outError = "Resource capacity must be at least 1.";
        return JsonValue();
    }

    int rId = engine_.addResourceType(params.name, params.capacity, params.orderRank);
    if (rId < 0) {
        outError = "Failed to add resource to engine.";
        return JsonValue();
    }

    addEvent("RESOURCE", "Defined resource class R" + std::to_string(rId) + " (\"" + params.name + "\", Capacity: " + std::to_string(params.capacity) + ", Rank: " + std::to_string(params.orderRank) + ").", "INFO");
    return getState();
}

JsonValue SimulationService::deleteProcess(int processId, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();
    
    int targetIndex = -1;
    for (size_t i = 0; i < state.processes.size(); ++i) {
        if (state.processes[i].id == processId) {
            targetIndex = static_cast<int>(i);
            break;
        }
    }
    if (targetIndex < 0 && processId >= 0 && processId < state.numProcesses()) {
        targetIndex = processId;
    }

    if (targetIndex < 0 || targetIndex >= state.numProcesses()) {
        outError = "Process not found with ID P" + std::to_string(processId);
        return JsonValue();
    }

    std::string pName = state.processes[targetIndex].name;
    int pRawId = state.processes[targetIndex].id;
    if (!engine_.deleteProcess(targetIndex)) {
        outError = "Failed to delete process P" + std::to_string(processId);
        return JsonValue();
    }
    addEvent("PROCESS", "Deleted process P" + std::to_string(pRawId) + " (\"" + pName + "\") from kernel table. Held resources reclaimed.", "WARNING");
    return getState();
}

JsonValue SimulationService::clearProcesses(std::string& /*outError*/) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    engine_.clearProcesses();
    addEvent("PROCESS", "Cleared all processes from kernel table. All allocated resources returned to physical headroom.", "WARNING");
    return getState();
}

JsonValue SimulationService::deleteResource(int resourceId, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();

    int targetIndex = -1;
    for (size_t j = 0; j < state.resources.size(); ++j) {
        if (state.resources[j].id == resourceId) {
            targetIndex = static_cast<int>(j);
            break;
        }
    }
    if (targetIndex < 0 && resourceId >= 0 && resourceId < state.numResources()) {
        targetIndex = resourceId;
    }

    if (targetIndex < 0 || targetIndex >= state.numResources()) {
        outError = "Resource class not found with ID R" + std::to_string(resourceId);
        return JsonValue();
    }

    std::string rName = state.resources[targetIndex].name;
    int rRawId = state.resources[targetIndex].id;
    if (!engine_.deleteResourceType(targetIndex)) {
        outError = "Failed to delete resource R" + std::to_string(resourceId);
        return JsonValue();
    }
    addEvent("RESOURCE", "Deleted resource class R" + std::to_string(rRawId) + " (\"" + rName + "\") from kernel catalog.", "WARNING");
    return getState();
}

JsonValue SimulationService::clearResources(std::string& /*outError*/) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    engine_.clearResources();
    addEvent("RESOURCE", "Cleared all resource classes from hardware catalog.", "WARNING");
    return getState();
}

JsonValue SimulationService::clearAll(std::string& /*outError*/) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    setupScenario("custom_manual");
    addEvent("SYSTEM", "Cleared all processes and resources. Blank custom sandbox initialized.", "WARNING");
    return getState();
}

JsonValue SimulationService::submitRequest(const SubmitRequestParams& params, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();

    if (params.processId < 0 || params.processId >= state.numProcesses()) {
        outError = "Invalid process ID: " + std::to_string(params.processId);
        return JsonValue();
    }

    if (state.processes[params.processId].state == core::ProcessState::TERMINATED) {
        outError = "Cannot submit request for terminated process P" + std::to_string(params.processId);
        return JsonValue();
    }

    auto opRes = engine_.submitRequest(params.processId, params.requestVector);
    if (!opRes.success) {
        if (opRes.message.find("Request queued") != std::string::npos) {
            addEvent("REQUEST", opRes.message, "WARNING");
            return getState();
        }
        outError = opRes.message;
        addEvent("PREVENTION", "Request rejected by C++ engine for P" + std::to_string(params.processId) + ": " + opRes.message, "WARNING");
        return JsonValue();
    }

    addEvent("REQUEST", opRes.message, "INFO");
    return getState();
}

JsonValue SimulationService::releaseResources(const ReleaseResourcesParams& params, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();

    if (params.processId < 0 || params.processId >= state.numProcesses()) {
        outError = "Invalid process ID: " + std::to_string(params.processId);
        return JsonValue();
    }

    auto opRes = engine_.releaseResources(params.processId, params.releaseVector);
    if (!opRes.success) {
        outError = opRes.message;
        return JsonValue();
    }

    int granted = engine_.processPendingRequests();
    std::string msg = opRes.message;
    if (granted > 0) {
        msg += " Auto-granted " + std::to_string(granted) + " pending request(s).";
    }

    addEvent("ALLOCATION", msg, "SUCCESS");
    return getState();
}

JsonValue SimulationService::setPreventionPolicy(const std::string& policyStr, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    prevention::PreventionPolicy pol = prevention::PreventionPolicy::NONE;

    if (policyStr == "HOLD_AND_WAIT_CONSERVATIVE" || policyStr == "HOLD_AND_WAIT") {
        pol = prevention::PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE;
    } else if (policyStr == "CIRCULAR_WAIT_TOTAL_ORDER" || policyStr == "CIRCULAR_WAIT") {
        pol = prevention::PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER;
    } else if (policyStr == "NONE" || policyStr == "DISABLED") {
        pol = prevention::PreventionPolicy::NONE;
    } else {
        outError = "Unknown policy: " + policyStr + ". Valid policies: NONE, HOLD_AND_WAIT, CIRCULAR_WAIT.";
        return JsonValue();
    }

    engine_.setPreventionPolicy(pol);
    addEvent("PREVENTION", "Active Deadlock Prevention policy set to: " + prevention::preventionPolicyToString(pol), "INFO");

    JsonValue res;
    res["success"] = true;
    res["policy"] = prevention::preventionPolicyToString(pol);
    return res;
}

JsonValue SimulationService::runDetection() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    detection::DetectionResult det = engine_.detectDeadlock();

    if (det.isDeadlocked) {
        std::string victimsStr;
        for (size_t i = 0; i < det.deadlockedProcessIds.size(); ++i) {
            victimsStr += "P" + std::to_string(det.deadlockedProcessIds[i]);
            if (i + 1 < det.deadlockedProcessIds.size()) victimsStr += ", ";
        }
        addEvent("DETECTION", "Shoshani-Coffman Detection: Deadlock found in candidate set D = { " + victimsStr + " }.", "ERROR");
    } else {
        addEvent("DETECTION", "Shoshani-Coffman Detection: Graph completely reducible. System is in SAFE STATE.", "SUCCESS");
    }

    return StateSerializer::serializeDetection(det);
}

JsonValue SimulationService::recoverVictimManual(int victimProcessId, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();

    if (victimProcessId < 0 || victimProcessId >= state.numProcesses()) {
        outError = "Invalid victim process ID: " + std::to_string(victimProcessId);
        return JsonValue();
    }

    auto rec = engine_.recoverDeadlockManual(victimProcessId);
    if (rec.status == recovery::RecoveryStatus::RECOVERY_NOT_NEEDED) {
        outError = rec.message;
        return JsonValue();
    }
    if (!rec.success && rec.status == recovery::RecoveryStatus::RECOVERY_FAILED) {
        outError = rec.message;
        return JsonValue();
    }

    std::string msg = rec.message;
    addEvent("RECOVERY", msg, rec.finalDetection.isDeadlocked ? "WARNING" : "SUCCESS");

    JsonValue res = StateSerializer::serializeRecovery(rec);
    res["updatedState"] = StateSerializer::serializeState(engine_.getState(), engine_.getPreventionPolicy(), activeScenarioId_, activeScenarioName_, activeScenarioDesc_);
    return res;
}

JsonValue SimulationService::recoverVictimAuto(const std::string& policyStr, std::string& outError) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    recovery::VictimSelectionPolicy pol = recovery::VictimSelectionPolicy::LOWEST_PRIORITY;

    if (policyStr == "LOWEST_PRIORITY" || policyStr == "LOW_PRIO") {
        pol = recovery::VictimSelectionPolicy::LOWEST_PRIORITY;
    } else if (policyStr == "HIGHEST_ALLOCATION" || policyStr == "HIGH_ALLOC") {
        pol = recovery::VictimSelectionPolicy::HIGHEST_ALLOCATION;
    } else if (policyStr == "MIN_TERMINATION_COST" || policyStr == "MIN_COST") {
        pol = recovery::VictimSelectionPolicy::MIN_TERMINATION_COST;
    } else {
        outError = "Unknown victim policy: " + policyStr;
        return JsonValue();
    }

    auto rec = engine_.recoverDeadlockAuto(pol);
    if (!rec.success && rec.status == recovery::RecoveryStatus::RECOVERY_FAILED) {
        outError = rec.message;
        return JsonValue();
    }

    addEvent("RECOVERY", "Automated recovery (" + recovery::victimPolicyToString(pol) + "): " + rec.message, rec.finalDetection.isDeadlocked ? "WARNING" : "SUCCESS");

    JsonValue res = StateSerializer::serializeRecovery(rec);
    res["updatedState"] = StateSerializer::serializeState(engine_.getState(), engine_.getPreventionPolicy(), activeScenarioId_, activeScenarioName_, activeScenarioDesc_);
    return res;
}

JsonValue SimulationService::getVictimCandidates() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto& state = engine_.getState();
    detection::DetectionResult det = engine_.detectDeadlock();

    JsonValue candidatesArr;
    for (int pId : det.deadlockedProcessIds) {
        if (pId >= 0 && pId < state.numProcesses()) {
            const auto& p = state.processes[pId];

            int heldTotal = 0;
            if (pId < static_cast<int>(state.allocation.size())) {
                for (int h : state.allocation[pId]) heldTotal += h;
            }

            int reqTotal = 0;
            if (pId < static_cast<int>(state.request.size())) {
                for (int r : state.request[pId]) reqTotal += r;
            }

            // Heuristic Cost from RecoveryEngine
            int calcCost = static_cast<int>(recovery::RecoveryEngine::calculateTerminationCost(state, pId));

            JsonValue cObj;
            cObj["id"] = "P" + std::to_string(p.id);
            cObj["rawId"] = p.id;
            cObj["name"] = p.name;
            cObj["pid"] = 1000 + p.id;
            cObj["priority"] = p.priority;
            cObj["baseCost"] = p.cpuTimeUsed * 2;
            cObj["heldResourcesTotal"] = heldTotal;
            cObj["requestedTotal"] = reqTotal;
            cObj["calculatedCost"] = calcCost;
            candidatesArr.push_back(cObj);
        }
    }

    return candidatesArr;
}

} // namespace deadlock::bridge
