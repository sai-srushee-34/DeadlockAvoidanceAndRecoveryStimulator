#include "core/ResourceManager.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace deadlock::core;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(testFunc) \
    do { \
        std::cout << "[RUNNING] " << #testFunc << "... "; \
        if (testFunc()) { \
            std::cout << "PASSED\n"; \
            passedCount++; \
        } else { \
            std::cout << "FAILED\n"; \
            failedCount++; \
        } \
    } while (0)

// Test 1: Resource and Process Initialization
bool testInitializationAndDimensions() {
    ResourceManager rm;
    int r0 = rm.addResourceType("CPU", 10, 1);
    int r1 = rm.addResourceType("Memory", 5, 2);
    int r2 = rm.addResourceType("Disk", 7, 3);

    TEST_ASSERT(r0 == 0 && r1 == 1 && r2 == 2, "Resource IDs must be 0, 1, 2");

    int p0 = rm.addProcess("P0", 5, {7, 5, 3});
    int p1 = rm.addProcess("P1", 8, {3, 2, 2});

    TEST_ASSERT(p0 == 0 && p1 == 1, "Process IDs must be 0, 1");

    const auto& state = rm.getState();
    TEST_ASSERT(state.numResources() == 3, "Resource count should be 3");
    TEST_ASSERT(state.numProcesses() == 2, "Process count should be 2");

    TEST_ASSERT(state.available == std::vector<int>({10, 5, 7}), "Available must match initial total capacity");
    TEST_ASSERT(state.allocation[0] == std::vector<int>({0, 0, 0}), "Initial allocation must be zero");
    TEST_ASSERT(state.processes[0].state == ProcessState::READY, "Initial process state must be READY");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must pass on init: " + err);
    return true;
}

// Test 2: Valid Allocation & Available Deduction
bool testValidAllocation() {
    ResourceManager rm;
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 5);
    rm.addProcess("P0", 5);

    auto result = rm.submitRequest(0, {3, 2});
    TEST_ASSERT(result.success, "Request should succeed when units are available: " + result.message);

    const auto& state = rm.getState();
    TEST_ASSERT(state.available == std::vector<int>({7, 3}), "Available must be deducted to [7, 3]");
    TEST_ASSERT(state.allocation[0] == std::vector<int>({3, 2}), "Allocation[0] must be [3, 2]");
    TEST_ASSERT(state.processes[0].state == ProcessState::RUNNING, "Process should be RUNNING");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Conservation invariant must hold: " + err);
    return true;
}

// Test 3: Resource Release and State Transition
bool testResourceRelease() {
    ResourceManager rm;
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 5);
    rm.addProcess("P0", 5);

    rm.submitRequest(0, {4, 3});
    auto relResult = rm.releaseResources(0, {2, 1});
    TEST_ASSERT(relResult.success, "Valid release must succeed: " + relResult.message);

    const auto& state = rm.getState();
    TEST_ASSERT(state.available == std::vector<int>({8, 3}), "Available must become [8, 3]");
    TEST_ASSERT(state.allocation[0] == std::vector<int>({2, 2}), "Allocation must become [2, 2]");
    TEST_ASSERT(state.processes[0].state == ProcessState::RUNNING, "Process should remain RUNNING with remaining units");

    // Release remaining units
    rm.releaseResources(0, {2, 2});
    TEST_ASSERT(state.available == std::vector<int>({10, 5}), "Available must be restored to [10, 5]");
    TEST_ASSERT(state.processes[0].state == ProcessState::READY, "Process holding 0 units should transition to READY");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Conservation invariant must hold: " + err);
    return true;
}

// Test 4: Resource Conservation Invariant across Multiple Operations
bool testResourceConservation() {
    ResourceManager rm;
    rm.addResourceType("A", 15);
    rm.addResourceType("B", 10);
    rm.addResourceType("C", 12);

    int p0 = rm.addProcess("Worker0");
    int p1 = rm.addProcess("Worker1");
    int p2 = rm.addProcess("Worker2");

    rm.submitRequest(p0, {4, 2, 3});
    rm.submitRequest(p1, {5, 3, 2});
    rm.submitRequest(p2, {2, 1, 4});

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariant must hold after multiple allocations: " + err);

    rm.releaseResources(p1, {2, 1, 0});
    TEST_ASSERT(rm.verifyInvariants(err), "Invariant must hold after partial release: " + err);

    rm.completeProcess(p0);
    TEST_ASSERT(rm.verifyInvariants(err), "Invariant must hold after process completion: " + err);

    return true;
}

// Test 5: Rejection of Invalid Negative and Dimension-Mismatched Inputs
bool testInvalidInputRejection() {
    ResourceManager rm;
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 10);
    rm.addProcess("P0");

    // Negative numbers
    auto resNeg = rm.submitRequest(0, {-1, 2});
    TEST_ASSERT(!resNeg.success, "Negative request should be rejected");

    // Dimension mismatch
    auto resDim = rm.submitRequest(0, {1, 2, 3});
    TEST_ASSERT(!resDim.success, "Vector with wrong dimension should be rejected");

    // Out-of-bounds process ID
    auto resProc = rm.submitRequest(99, {1, 1});
    TEST_ASSERT(!resProc.success, "Non-existent process ID should be rejected");

    auto resProcNeg = rm.submitRequest(-1, {1, 1});
    TEST_ASSERT(!resProcNeg.success, "Negative process ID should be rejected");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants should remain intact after failed inputs");
    return true;
}

// Test 6: Rejection of Invalid Release (Releasing More than Held)
bool testInvalidReleaseRejection() {
    ResourceManager rm;
    rm.addResourceType("R0", 10);
    rm.addProcess("P0");

    rm.submitRequest(0, {3});

    // Attempt to release 5 when only holding 3
    auto resRel = rm.releaseResources(0, {5});
    TEST_ASSERT(!resRel.success, "Releasing more than held must fail");

    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[0][0] == 3, "Allocation must not be corrupted");
    TEST_ASSERT(state.available[0] == 7, "Available must remain 7");

    return true;
}

// Test 7: No Silent Allocation When Resources Unavailable (Queuing & WAITING State)
bool testUnavailableRequestQueuing() {
    ResourceManager rm;
    rm.addResourceType("CPU", 5);
    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");

    // P0 allocates 4 of 5 CPUs
    rm.submitRequest(p0, {4});
    TEST_ASSERT(rm.getState().available[0] == 1, "Available must be 1");

    // P1 requests 3 CPUs (only 1 available)
    auto resP1 = rm.submitRequest(p1, {3});
    TEST_ASSERT(!resP1.success, "Request should not be granted immediately when unavailable");

    const auto& state = rm.getState();
    // Verify NO SILENT ALLOCATION OCCURRED
    TEST_ASSERT(state.allocation[p1][0] == 0, "P1 must not receive silent allocation");
    TEST_ASSERT(state.available[0] == 1, "Available must remain 1");
    // Verify Request matrix recorded the demand
    TEST_ASSERT(state.request[p1][0] == 3, "Request[P1] must record 3 requested units");
    // Verify process transitioned to WAITING
    TEST_ASSERT(state.processes[p1].state == ProcessState::WAITING, "P1 must transition to WAITING");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold with pending request: " + err);
    return true;
}

// Test 8: Max Demand Model and Need Validation
bool testMaxDemandValidation() {
    ResourceManager rm;
    rm.addResourceType("A", 10);
    rm.addResourceType("B", 10);

    // Max claim for P0 is [5, 4]
    int p0 = rm.addProcess("P0", 5, {5, 4});
    rm.enableMaxDemandValidation(true);

    const auto& state = rm.getState();
    TEST_ASSERT(state.need[p0] == std::vector<int>({5, 4}), "Initial Need must equal Max Demand");

    // Valid request within Need
    auto resValid = rm.submitRequest(p0, {2, 2});
    TEST_ASSERT(resValid.success, "Request within Need must succeed: " + resValid.message);
    TEST_ASSERT(state.need[p0] == std::vector<int>({3, 2}), "Need must decrease to [3, 2]");

    // Request exceeding remaining Need
    auto resExceed = rm.submitRequest(p0, {4, 1}); // Requests 4 of A, Need is only 3
    TEST_ASSERT(!resExceed.success, "Request exceeding Need must be rejected");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold under max demand model: " + err);
    return true;
}

// Test 9: Process Completion and Resource Reclamation
bool testProcessCompletion() {
    ResourceManager rm;
    rm.addResourceType("A", 10);
    rm.addResourceType("B", 8);
    int p0 = rm.addProcess("P0");

    rm.submitRequest(p0, {6, 4});
    TEST_ASSERT(rm.getState().available == std::vector<int>({4, 4}), "Available should be [4, 4]");

    auto compResult = rm.completeProcess(p0);
    TEST_ASSERT(compResult.success, "Process completion should succeed: " + compResult.message);

    const auto& state = rm.getState();
    TEST_ASSERT(state.processes[p0].state == ProcessState::TERMINATED, "Process state must be TERMINATED");
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({0, 0}), "Allocation must be zeroed out");
    TEST_ASSERT(state.available == std::vector<int>({10, 8}), "All resources must be returned to Available");

    // Attempting to request from TERMINATED process must be rejected
    auto resAfterTerm = rm.submitRequest(p0, {1, 1});
    TEST_ASSERT(!resAfterTerm.success, "Terminated process cannot make requests");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold after process termination: " + err);
    return true;
}

// Test 10: Pending Request Resolution on Resource Release
bool testPendingRequestResolution() {
    ResourceManager rm;
    rm.addResourceType("GPU", 2);
    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");

    rm.submitRequest(p0, {2}); // P0 takes both GPUs
    rm.submitRequest(p1, {1}); // P1 waits for 1 GPU

    TEST_ASSERT(rm.getState().processes[p1].state == ProcessState::WAITING, "P1 should be WAITING");

    // P0 releases 1 GPU
    rm.releaseResources(p0, {1});
    TEST_ASSERT(rm.getState().available[0] == 1, "Available GPU must be 1");

    // Sweep pending requests
    int granted = rm.processPendingRequests();
    TEST_ASSERT(granted == 1, "Should grant 1 pending request");

    const auto& state = rm.getState();
    TEST_ASSERT(state.processes[p1].state == ProcessState::RUNNING, "P1 should now be RUNNING");
    TEST_ASSERT(state.allocation[p1][0] == 1, "P1 should now hold 1 GPU");
    TEST_ASSERT(state.request[p1][0] == 0, "P1 request should be cleared");
    TEST_ASSERT(state.available[0] == 0, "Available GPU must now be 0");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold after granting pending request: " + err);
    return true;
}

// Test 11: Process Deletion & Immediate Resource Reclamation
bool testProcessDeletionAndConservation() {
    ResourceManager rm;
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 5);
    rm.addResourceType("R2", 7);

    int p0 = rm.addProcess("P0", 5);
    int p1 = rm.addProcess("P1", 8);

    rm.allocateDirect(p0, {3, 2, 2});
    rm.allocateDirect(p1, {2, 1, 1});

    // Before deletion, available should be: {10-5=5, 5-3=2, 7-3=4}
    TEST_ASSERT(rm.getState().available == std::vector<int>({5, 2, 4}), "Available before deletion must be [5, 2, 4]");

    // Delete process P0
    bool deleted = rm.deleteProcess(p0);
    TEST_ASSERT(deleted, "deleteProcess should succeed for valid ID");

    // Invariants must hold
    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold after process deletion: " + err);

    const auto& state = rm.getState();
    TEST_ASSERT(state.numProcesses() == 1, "Process count should now be 1");
    // P0's allocations {3, 2, 2} must be returned to available -> {5+3=8, 2+2=4, 4+2=6}
    TEST_ASSERT(state.available == std::vector<int>({8, 4, 6}), "P0 resources must be returned to Available");
    // P1 was re-indexed to ID 0 and retains its allocation
    TEST_ASSERT(state.allocation[0] == std::vector<int>({2, 1, 1}), "P1 allocation must be preserved");

    return true;
}

// Test 12: Resource Type Deletion & Matrix Dimension Updating
bool testResourceTypeDeletion() {
    ResourceManager rm;
    rm.addResourceType("R0", 5);
    rm.addResourceType("R1", 4);

    int p0 = rm.addProcess("P0", 5);
    rm.allocateDirect(p0, {2, 1});

    TEST_ASSERT(rm.getState().numResources() == 2, "Initial resources should be 2");

    bool deleted = rm.deleteResourceType(1); // delete R1
    TEST_ASSERT(deleted, "deleteResourceType should succeed for valid ID");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold after resource deletion: " + err);

    const auto& state = rm.getState();
    TEST_ASSERT(state.numResources() == 1, "Resource count should now be 1");
    TEST_ASSERT(state.resources[0].name == "R0", "Remaining resource must be R0");
    TEST_ASSERT(state.allocation[0] == std::vector<int>({2}), "P0 allocation row must now be 1 column [2]");
    TEST_ASSERT(state.available == std::vector<int>({3}), "Available vector must now be [3]");

    return true;
}

// Test 13: Clear All & Blank Canvas Initialization
bool testClearAllBlankCanvas() {
    ResourceManager rm;
    rm.addResourceType("R0", 5);
    rm.addProcess("P0", 5);

    rm.clearAll();

    const auto& state = rm.getState();
    TEST_ASSERT(state.numProcesses() == 0, "Processes must be empty after clearAll");
    TEST_ASSERT(state.numResources() == 0, "Resources must be empty after clearAll");
    TEST_ASSERT(state.available.empty(), "Available vector must be empty");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold for empty state: " + err);

    return true;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " DEADLOCK SIMULATOR: CORE ENGINE UNIT TEST HARNESS\n";
    std::cout << "========================================================\n\n";

    int passedCount = 0;
    int failedCount = 0;

    RUN_TEST(testInitializationAndDimensions);
    RUN_TEST(testValidAllocation);
    RUN_TEST(testResourceRelease);
    RUN_TEST(testResourceConservation);
    RUN_TEST(testInvalidInputRejection);
    RUN_TEST(testInvalidReleaseRejection);
    RUN_TEST(testUnavailableRequestQueuing);
    RUN_TEST(testMaxDemandValidation);
    RUN_TEST(testProcessCompletion);
    RUN_TEST(testPendingRequestResolution);
    RUN_TEST(testProcessDeletionAndConservation);
    RUN_TEST(testResourceTypeDeletion);
    RUN_TEST(testClearAllBlankCanvas);

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "TEST RESULTS: " << passedCount << " Passed, " << failedCount << " Failed.\n";
    std::cout << "========================================================\n";

    return (failedCount == 0) ? 0 : 1;
}
