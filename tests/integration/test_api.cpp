#include "bridge/SimulationService.hpp"
#include "bridge/Json.hpp"

#include <iostream>
#include <cassert>
#include <string>

using namespace deadlock::bridge;

void assertCondition(bool condition, const std::string& testName) {
    if (!condition) {
        std::cerr << "[FAILED] " << testName << std::endl;
        std::exit(1);
    }
    std::cout << "[RUNNING] " << testName << "... PASSED" << std::endl;
}

void testInitialStateAndScenarios() {
    SimulationService service;
    JsonValue state = service.getState();

    assertCondition(state.get("scenarioId").asString() == "standard_safe", "testInitialState_ScenarioName");
    assertCondition(state.get("resources").asArray().size() == 3, "testInitialState_ResourceCount");
    assertCondition(state.get("processes").asArray().size() == 3, "testInitialState_ProcessCount");
    assertCondition(state.get("isDeadlocked").asBool() == false, "testInitialState_NotDeadlocked");

    JsonValue scenarios = service.getAvailableScenarios();
    assertCondition(scenarios.asArray().size() == 5, "testAvailableScenarios_Count");
}

void testAddProcessValidAndInvalid() {
    SimulationService service;
    std::string err;

    // Invalid: Empty name
    SimulationService::AddProcessParams badParams;
    badParams.name = "";
    service.addProcess(badParams, err);
    assertCondition(!err.empty(), "testAddProcess_RejectEmptyName");

    // Invalid: Priority out of range
    err.clear();
    SimulationService::AddProcessParams badPrio;
    badPrio.name = "Worker-Test";
    badPrio.priority = 15;
    service.addProcess(badPrio, err);
    assertCondition(!err.empty(), "testAddProcess_RejectInvalidPriority");

    // Valid
    err.clear();
    SimulationService::AddProcessParams validParams;
    validParams.name = "Worker-Test";
    validParams.priority = 7;
    validParams.cost = 40;
    validParams.maxDemand = {5, 2, 2};
    JsonValue state = service.addProcess(validParams, err);
    assertCondition(err.empty(), "testAddProcess_Success");
    assertCondition(state.get("processes").asArray().size() == 4, "testAddProcess_ProcessCountIncremented");
}

void testAddResourceValidAndInvalid() {
    SimulationService service;
    std::string err;

    // Invalid: Capacity <= 0
    SimulationService::AddResourceParams badRes;
    badRes.name = "GPU";
    badRes.capacity = 0;
    service.addResource(badRes, err);
    assertCondition(!err.empty(), "testAddResource_RejectZeroCapacity");

    // Valid
    err.clear();
    SimulationService::AddResourceParams validRes;
    validRes.name = "GPU";
    validRes.capacity = 4;
    validRes.orderRank = 4;
    JsonValue state = service.addResource(validRes, err);
    assertCondition(err.empty(), "testAddResource_Success");
    assertCondition(state.get("resources").asArray().size() == 4, "testAddResource_ResourceCountIncremented");
}

void testRequestSubmissionAndQueuing() {
    SimulationService service;
    std::string err;

    // P0 requests 1 unit of R0 (available is 2) -> Granted immediately
    SimulationService::SubmitRequestParams req1;
    req1.processId = 0;
    req1.requestVector = {1, 0, 0};
    JsonValue s1 = service.submitRequest(req1, err);
    assertCondition(err.empty(), "testRequest_ImmediateGrantSuccess");

    // P0 requests 2 units of R0 (within Need 3, but exceeds Available 1) -> Must wait
    SimulationService::SubmitRequestParams req2;
    req2.processId = 0;
    req2.requestVector = {2, 0, 0};
    JsonValue s2 = service.submitRequest(req2, err);
    assertCondition(err.empty(), "testRequest_QueuedWaitingSuccess");
    assertCondition(s2.get("processes").asArray()[0].get("state").asString() == "WAITING", "testRequest_ProcessStateWaiting");
}

void testPreventionRejectionHoldAndWait() {
    SimulationService service;
    std::string err;

    // Set Hold and Wait Conservative policy
    service.setPreventionPolicy("HOLD_AND_WAIT_CONSERVATIVE", err);
    assertCondition(err.empty(), "testPrevention_SetPolicySuccess");

    // P0 currently holds [3, 1, 2]. Incremental request must be rejected!
    SimulationService::SubmitRequestParams req;
    req.processId = 0;
    req.requestVector = {1, 0, 0};
    service.submitRequest(req, err);
    assertCondition(!err.empty(), "testPrevention_RejectHoldAndWaitIncremental");
}

void testPreventionRejectionCircularWait() {
    SimulationService service;
    std::string err;

    // Set Circular Wait Total Order policy
    service.setPreventionPolicy("CIRCULAR_WAIT_TOTAL_ORDER", err);
    assertCondition(err.empty(), "testPrevention_SetCircularWaitPolicySuccess");

    // P0 holds R2 (order rank 3). Attempt to request R0 (order rank 1) -> Must be rejected!
    SimulationService::SubmitRequestParams req;
    req.processId = 0;
    req.requestVector = {1, 0, 0};
    service.submitRequest(req, err);
    assertCondition(!err.empty(), "testPrevention_RejectCircularWaitDecreasingOrder");
}

void testDeadlockScenarioDetection() {
    SimulationService service;
    service.loadScenario("silberschatz_deadlock");

    JsonValue det = service.runDetection();
    assertCondition(det.get("isDeadlocked").asBool() == true, "testDeadlockDetection_IsDeadlocked");

    JsonValue deadlockedJson = det.get("deadlockedProcesses");
    const auto& deadlockedArr = deadlockedJson.asArray();
    assertCondition(deadlockedArr.size() == 4, "testDeadlockDetection_CandidateCount");

    // Candidate set must be { P1, P2, P3, P4 }
    bool foundP1 = false, foundP2 = false, foundP3 = false, foundP4 = false;
    for (const auto& item : deadlockedArr) {
        std::string tag = item.asString();
        if (tag == "P1") foundP1 = true;
        if (tag == "P2") foundP2 = true;
        if (tag == "P3") foundP3 = true;
        if (tag == "P4") foundP4 = true;
    }
    assertCondition(foundP1 && foundP2 && foundP3 && foundP4, "testDeadlockDetection_CandidateMembers");
}

void testSafeScenarioDetection() {
    SimulationService service;
    service.loadScenario("standard_safe");

    JsonValue det = service.runDetection();
    assertCondition(det.get("isDeadlocked").asBool() == false, "testSafeScenario_NotDeadlocked");
    assertCondition(det.get("deadlockedProcesses").asArray().empty(), "testSafeScenario_EmptyDeadlockSet");
}

void testRecoveryFollowedByDetection() {
    SimulationService service;
    service.loadScenario("silberschatz_deadlock");

    std::string err;
    // P1 is in deadlock set D. Terminate P1!
    JsonValue rec = service.recoverVictimManual(1, err);
    assertCondition(err.empty(), "testRecovery_TerminateVictimP1_Success");
    assertCondition(rec.get("status").asString() == "RECOVERY_SUCCESSFUL" || rec.get("status").asString() == "RECOVERY_PARTIAL", "testRecovery_ValidStatus");

    // Post-recovery state check
    JsonValue state = service.getState();
    assertCondition(state.get("processes").asArray()[1].get("state").asString() == "TERMINATED", "testRecovery_P1MarkedTerminated");

    // Verify resources were reclaimed into Available vector
    int availR0 = state.get("available").asArray()[0].asInt();
    assertCondition(availR0 >= 2, "testRecovery_AllocationsReclaimedToAvailable");
}

void testReleaseResourcesAndSweep() {
    SimulationService service;
    std::string err;

    // Release 1 unit of R0 from P0
    SimulationService::ReleaseResourcesParams rel;
    rel.processId = 0;
    rel.releaseVector = {1, 0, 0};
    JsonValue state = service.releaseResources(rel, err);
    assertCondition(err.empty(), "testReleaseResources_Success");

    // Invalid release: process doesn't hold 10 units of R0
    SimulationService::ReleaseResourcesParams badRel;
    badRel.processId = 0;
    badRel.releaseVector = {10, 0, 0};
    service.releaseResources(badRel, err);
    assertCondition(!err.empty(), "testReleaseResources_RejectExcessiveRelease");
}

void testInvalidVictimRecoveryRejection() {
    SimulationService service;
    service.loadScenario("standard_safe");

    std::string err;
    // In standard safe state, system is not deadlocked. Manual recovery should fail!
    service.recoverVictimManual(0, err);
    assertCondition(!err.empty(), "testRecovery_RejectWhenNoDeadlock");
}

void testCustomSandboxAndDeletion() {
    SimulationService service;
    JsonValue state = service.loadScenario("custom_manual");

    assertCondition(state.get("scenarioId").asString() == "custom_manual", "testCustomSandbox_ScenarioId");
    assertCondition(state.get("processes").asArray().empty(), "testCustomSandbox_ZeroProcessesInitially");
    assertCondition(state.get("resources").asArray().empty(), "testCustomSandbox_ZeroResourcesInitially");

    std::string err;
    // Add custom resource
    SimulationService::AddResourceParams rParams;
    rParams.name = "Custom-Core";
    rParams.capacity = 4;
    rParams.orderRank = 1;
    state = service.addResource(rParams, err);
    assertCondition(err.empty() && state.get("resources").asArray().size() == 1, "testCustomSandbox_AddCustomResource");

    // Add custom process
    SimulationService::AddProcessParams pParams;
    pParams.name = "Custom-Worker";
    pParams.priority = 6;
    pParams.initialAlloc = {2};
    state = service.addProcess(pParams, err);
    assertCondition(err.empty() && state.get("processes").asArray().size() == 1, "testCustomSandbox_AddCustomProcess");

    // Delete process
    state = service.deleteProcess(0, err);
    assertCondition(err.empty() && state.get("processes").asArray().empty(), "testCustomSandbox_DeleteProcess");
    assertCondition(state.get("available").asArray()[0].asInt() == 4, "testCustomSandbox_ResourcesReclaimedOnDelete");

    // Delete resource
    state = service.deleteResource(0, err);
    assertCondition(err.empty() && state.get("resources").asArray().empty(), "testCustomSandbox_DeleteResource");

    // Clear all
    state = service.clearAll(err);
    assertCondition(err.empty() && state.get("scenarioId").asString() == "custom_manual", "testCustomSandbox_ClearAll");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " DEADLOCK SIMULATOR: PHASE 7 API & SERVICE TEST HARNESS\n";
    std::cout << "========================================================\n\n";

    testInitialStateAndScenarios();
    testAddProcessValidAndInvalid();
    testAddResourceValidAndInvalid();
    testRequestSubmissionAndQueuing();
    testPreventionRejectionHoldAndWait();
    testPreventionRejectionCircularWait();
    testDeadlockScenarioDetection();
    testSafeScenarioDetection();
    testRecoveryFollowedByDetection();
    testReleaseResourcesAndSweep();
    testInvalidVictimRecoveryRejection();
    testCustomSandboxAndDeletion();

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "API TEST RESULTS: 12 Passed, 0 Failed.\n";
    std::cout << "========================================================\n";

    return 0;
}
