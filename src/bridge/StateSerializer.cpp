#include "bridge/StateSerializer.hpp"
#include "detection/DetectionEngine.hpp"

#include <numeric>
#include <iomanip>
#include <sstream>

namespace deadlock::bridge {

JsonValue StateSerializer::serializeState(const core::SystemState& state,
                                          prevention::PreventionPolicy policy,
                                          const std::string& scenarioId,
                                          const std::string& scenarioName,
                                          const std::string& description) {
    JsonValue root;
    root["scenarioId"] = scenarioId;
    root["scenarioName"] = scenarioName;
    root["description"] = description;
    root["step"] = state.simulationStep;
    root["policy"] = prevention::preventionPolicyToString(policy);
    root["policyCode"] = (policy == prevention::PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE) ? "HOLD_AND_WAIT_CONSERVATIVE" :
                         (policy == prevention::PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER) ? "CIRCULAR_WAIT_TOTAL_ORDER" : "NONE";

    // Prevention settings flags
    JsonValue prevSettings;
    prevSettings["policy"] = (policy == prevention::PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE) ? "A" : "B";
    prevSettings["holdWaitActive"] = (policy == prevention::PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);
    prevSettings["circularWaitActive"] = (policy == prevention::PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);
    root["preventionSettings"] = prevSettings;

    // Resources
    JsonValue resArr(std::vector<JsonValue>{});
    int totalCapacity = 0;
    for (size_t j = 0; j < state.resources.size(); ++j) {
        const auto& r = state.resources[j];
        totalCapacity += r.totalCapacity;
        int avail = (j < state.available.size()) ? state.available[j] : 0;

        JsonValue rObj;
        rObj["id"] = "R" + std::to_string(r.id);
        rObj["rawId"] = r.id;
        rObj["name"] = r.name;
        rObj["capacity"] = r.totalCapacity;
        rObj["available"] = avail;
        rObj["orderRank"] = r.orderRank;
        rObj["unit"] = "Units";
        resArr.push_back(rObj);
    }
    root["resources"] = resArr;

    // Run live detection on current state
    detection::DetectionResult detResult = detection::DetectionEngine::detectDeadlock(state);

    // Processes
    JsonValue procArr(std::vector<JsonValue>{});
    int runningCount = 0;
    int waitingCount = 0;
    int deadlockedCount = 0;
    int terminatedCount = 0;

    for (size_t i = 0; i < state.processes.size(); ++i) {
        const auto& p = state.processes[i];
        std::string pStateStr = core::processStateToString(p.state);

        bool isDeadlocked = false;
        for (int dId : detResult.deadlockedProcessIds) {
            if (dId == p.id) {
                isDeadlocked = true;
                break;
            }
        }

        if (p.state == core::ProcessState::TERMINATED) {
            terminatedCount++;
        } else if (isDeadlocked) {
            pStateStr = "DEADLOCKED";
            deadlockedCount++;
        } else if (p.state == core::ProcessState::WAITING) {
            waitingCount++;
        } else {
            runningCount++;
        }

        JsonValue pObj;
        pObj["id"] = "P" + std::to_string(p.id);
        pObj["rawId"] = p.id;
        pObj["name"] = p.name;
        pObj["pid"] = 1000 + p.id;
        pObj["priority"] = p.priority;
        pObj["state"] = pStateStr;
        pObj["cost"] = static_cast<int>(recovery::RecoveryEngine::calculateTerminationCost(state, p.id));
        pObj["cpuTime"] = p.cpuTimeUsed;
        pObj["preemptions"] = p.preemptionCount;
        procArr.push_back(pObj);
    }
    root["processes"] = procArr;

    // Available vector
    JsonValue availArr(std::vector<JsonValue>{});
    for (int a : state.available) {
        availArr.push_back(a);
    }
    root["available"] = availArr;

    // Allocation Matrix
    int totalAllocated = 0;
    JsonValue allocMat(std::vector<JsonValue>{});
    for (const auto& row : state.allocation) {
        JsonValue rowArr(std::vector<JsonValue>{});
        for (int val : row) {
            totalAllocated += val;
            rowArr.push_back(val);
        }
        allocMat.push_back(rowArr);
    }
    root["allocation"] = allocMat;

    // Request Matrix
    int totalRequested = 0;
    JsonValue reqMat(std::vector<JsonValue>{});
    for (const auto& row : state.request) {
        JsonValue rowArr(std::vector<JsonValue>{});
        for (int val : row) {
            totalRequested += val;
            rowArr.push_back(val);
        }
        reqMat.push_back(rowArr);
    }
    root["request"] = reqMat;

    // Max Demand Matrix
    JsonValue maxMat(std::vector<JsonValue>{});
    for (const auto& row : state.maxDemand) {
        JsonValue rowArr(std::vector<JsonValue>{});
        for (int val : row) {
            rowArr.push_back(val);
        }
        maxMat.push_back(rowArr);
    }
    root["maxDemand"] = maxMat;

    // Need Matrix
    JsonValue needMat(std::vector<JsonValue>{});
    for (const auto& row : state.need) {
        JsonValue rowArr(std::vector<JsonValue>{});
        for (int val : row) {
            rowArr.push_back(val);
        }
        needMat.push_back(rowArr);
    }
    root["need"] = needMat;

    // Summary Metrics
    double utilPct = (totalCapacity > 0) ? (static_cast<double>(totalAllocated) / totalCapacity * 100.0) : 0.0;
    std::ostringstream ssUtil;
    ssUtil << std::fixed << std::setprecision(1) << utilPct;

    JsonValue metrics;
    metrics["totalProcesses"] = static_cast<int>(state.processes.size());
    metrics["runningProcesses"] = runningCount;
    metrics["waitingProcesses"] = waitingCount;
    metrics["deadlockedProcessesCount"] = deadlockedCount;
    metrics["terminatedProcesses"] = terminatedCount;
    metrics["totalResources"] = static_cast<int>(state.resources.size());
    metrics["totalCapacity"] = totalCapacity;
    metrics["totalAllocated"] = totalAllocated;
    metrics["totalRequested"] = totalRequested;
    metrics["utilizationPct"] = ssUtil.str();
    root["metrics"] = metrics;

    // Live Detection Summary
    root["isDeadlocked"] = detResult.isDeadlocked;
    JsonValue deadlockedArr(std::vector<JsonValue>{});
    for (int id : detResult.deadlockedProcessIds) {
        deadlockedArr.push_back("P" + std::to_string(id));
    }
    root["deadlockedProcesses"] = deadlockedArr;
    root["detectionReport"] = serializeDetection(detResult);

    return root;
}

JsonValue StateSerializer::serializeDetection(const detection::DetectionResult& result) {
    JsonValue obj;
    obj["validState"] = result.validState;
    obj["stateError"] = result.stateError;
    obj["isDeadlocked"] = result.isDeadlocked;

    JsonValue deadlockedArr(std::vector<JsonValue>{});
    for (int id : result.deadlockedProcessIds) {
        deadlockedArr.push_back("P" + std::to_string(id));
    }
    obj["deadlockedProcesses"] = deadlockedArr;

    JsonValue reducibleArr(std::vector<JsonValue>{});
    for (int id : result.reducibleProcessIds) {
        reducibleArr.push_back("P" + std::to_string(id));
    }
    obj["reducibleProcesses"] = reducibleArr;

    JsonValue finalWorkArr(std::vector<JsonValue>{});
    for (int w : result.finalWork) {
        finalWorkArr.push_back(w);
    }
    obj["finalWork"] = finalWorkArr;

    JsonValue traceArr(std::vector<JsonValue>{});
    for (size_t i = 0; i < result.reductionSteps.size(); ++i) {
        JsonValue stepObj;
        stepObj["step"] = static_cast<int>(i + 1);
        stepObj["description"] = result.reductionSteps[i];
        traceArr.push_back(stepObj);
    }
    obj["trace"] = traceArr;
    obj["diagnosticReport"] = result.diagnosticReport;

    return obj;
}

JsonValue StateSerializer::serializeRecovery(const recovery::RecoveryResult& result) {
    JsonValue obj;
    obj["status"] = recovery::recoveryStatusToString(result.status);
    obj["success"] = result.success;
    obj["message"] = result.message;
    obj["policyUsed"] = recovery::victimPolicyToString(result.policyUsed);
    obj["deadlockResolved"] = !result.finalDetection.isDeadlocked;

    JsonValue victimsArr(std::vector<JsonValue>{});
    for (int id : result.terminatedProcessIds) {
        victimsArr.push_back("P" + std::to_string(id));
    }
    obj["terminatedProcessIds"] = victimsArr;

    int totalReclaimedSum = 0;
    JsonValue reclaimedArr(std::vector<JsonValue>{});
    for (int r : result.totalResourcesReclaimed) {
        totalReclaimedSum += r;
        reclaimedArr.push_back(r);
    }
    obj["totalResourcesReclaimed"] = reclaimedArr;
    obj["totalUnitsReclaimed"] = totalReclaimedSum;

    JsonValue remainingDeadlocked(std::vector<JsonValue>{});
    for (int id : result.finalDetection.deadlockedProcessIds) {
        remainingDeadlocked.push_back("P" + std::to_string(id));
    }
    obj["remainingDeadlocked"] = remainingDeadlocked;

    JsonValue stepsArr(std::vector<JsonValue>{});
    for (const auto& step : result.steps) {
        JsonValue sObj;
        sObj["stepNumber"] = step.stepNumber;
        sObj["victimProcessId"] = "P" + std::to_string(step.victimProcessId);
        sObj["victimProcessName"] = step.victimProcessName;
        sObj["victimPriority"] = step.victimPriority;
        sObj["details"] = step.details;
        stepsArr.push_back(sObj);
    }
    obj["steps"] = stepsArr;
    obj["diagnosticReport"] = result.diagnosticReport;

    return obj;
}

JsonValue StateSerializer::serializeEvent(const SimulationEvent& evt) {
    JsonValue obj;
    obj["id"] = evt.id;
    obj["timestamp"] = evt.timestamp;
    obj["category"] = evt.category;
    obj["severity"] = evt.severity;
    obj["message"] = evt.message;
    obj["metadata"] = evt.metadata;
    return obj;
}

JsonValue StateSerializer::serializeEvents(const std::vector<SimulationEvent>& events) {
    JsonValue arr;
    for (const auto& evt : events) {
        arr.push_back(serializeEvent(evt));
    }
    return arr;
}

JsonValue StateSerializer::serializeRejection(const core::RejectionRecord& rec) {
    JsonValue obj;
    obj["stepNumber"] = rec.stepNumber;
    obj["processId"] = "P" + std::to_string(rec.processId);
    obj["processName"] = rec.processName;
    obj["policyName"] = rec.policyName;
    obj["reason"] = rec.reason;

    JsonValue reqVec;
    for (int v : rec.requestVector) {
        reqVec.push_back(v);
    }
    obj["requestVector"] = reqVec;
    return obj;
}

} // namespace deadlock::bridge
