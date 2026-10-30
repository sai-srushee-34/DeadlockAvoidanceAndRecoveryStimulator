#include "core/ResourceManager.hpp"
#include "prevention/PreventionEngine.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace deadlock::core;
using namespace deadlock::prevention;

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

// Test 1: Hold-and-Wait Valid Upfront All-at-Once Acquisition
bool testHoldAndWaitValidUpfront() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 10);
    int p0 = rm.addProcess("P0");

    // Process starts with 0 held resources. Requests full requirement [3, 4]
    auto result = rm.submitRequest(p0, {3, 4});
    TEST_ASSERT(result.success, "Upfront all-at-once request must be granted: " + result.message);

    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({3, 4}), "Allocation must be [3, 4]");
    TEST_ASSERT(state.available == std::vector<int>({7, 6}), "Available must be [7, 6]");
    TEST_ASSERT(state.processes[p0].state == ProcessState::RUNNING, "Process must be RUNNING");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold: " + err);
    return true;
}

// Test 2: Hold-and-Wait Rejection of Incremental Requests while Holding Resources
bool testHoldAndWaitRejectionIncremental() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);
    rm.addResourceType("R0", 10);
    rm.addResourceType("R1", 10);
    int p0 = rm.addProcess("P0");

    // Initial grant: P0 obtains [2, 2]
    auto firstReq = rm.submitRequest(p0, {2, 2});
    TEST_ASSERT(firstReq.success, "Initial request must succeed");

    // P0 attempts to request additional resources [1, 0] while holding [2, 2]
    auto secondReq = rm.submitRequest(p0, {1, 0});
    TEST_ASSERT(!secondReq.success, "Incremental request while holding resources must be REJECTED");
    TEST_ASSERT(secondReq.message.find("Hold-and-Wait violation") != std::string::npos,
                "Error message must cite Hold-and-Wait violation");

    // Verify system state remained unmodified by rejected request
    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({2, 2}), "Allocation must remain [2, 2]");
    TEST_ASSERT(state.available == std::vector<int>({8, 8}), "Available must remain [8, 8]");
    TEST_ASSERT(state.request[p0] == std::vector<int>({0, 0}), "No pending request should be queued on protocol violation");

    // Verify rejection log captured the event
    TEST_ASSERT(rm.getRejectionLog().size() == 1, "Rejection log must record the violation");
    TEST_ASSERT(rm.getRejectionLog()[0].processId == p0, "Rejection log must reference process P0");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold after rejection: " + err);
    return true;
}

// Test 3: Hold-and-Wait Queuing with Zero Partial Allocation when Unavailable
bool testHoldAndWaitZeroPartialAllocation() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);
    rm.addResourceType("CPU", 4);
    rm.addResourceType("Memory", 4);

    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");

    // P0 allocates [3, 2], leaving [1, 2] available
    rm.submitRequest(p0, {3, 2});

    // P1 requests [2, 1] (CPU requested is 2, but only 1 available)
    auto resP1 = rm.submitRequest(p1, {2, 1});
    TEST_ASSERT(!resP1.success, "Request must not be granted when insufficient instances available");

    const auto& state = rm.getState();
    // CRITICAL: P1 must hold ZERO units (no partial allocation of Memory!)
    TEST_ASSERT(state.allocation[p1] == std::vector<int>({0, 0}),
                "Hold-and-Wait requires ZERO partial allocation when full request cannot be met");
    TEST_ASSERT(state.available == std::vector<int>({1, 2}), "Available units must remain untouched");
    TEST_ASSERT(state.processes[p1].state == ProcessState::WAITING, "P1 must transition to WAITING");
    TEST_ASSERT(state.request[p1] == std::vector<int>({2, 1}), "Pending request must be preserved in Request matrix");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold: " + err);
    return true;
}

// Test 4: Circular-Wait Valid Increasing Order Requests
bool testCircularWaitValidIncreasingOrder() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);

    // Assign explicit ordering ranks: TapeDrive = 1, Disk = 2, Printer = 3
    rm.addResourceType("TapeDrive", 5, 1);
    rm.addResourceType("Disk", 5, 2);
    rm.addResourceType("Printer", 5, 3);

    int p0 = rm.addProcess("P0");

    // Request TapeDrive (rank 1 > 0)
    auto req1 = rm.submitRequest(p0, {1, 0, 0});
    TEST_ASSERT(req1.success, "Requesting rank 1 when holding nothing must succeed: " + req1.message);

    // Request Disk (rank 2 > 1)
    auto req2 = rm.submitRequest(p0, {0, 1, 0});
    TEST_ASSERT(req2.success, "Requesting rank 2 when holding rank 1 must succeed: " + req2.message);

    // Request Printer (rank 3 > 2)
    auto req3 = rm.submitRequest(p0, {0, 0, 1});
    TEST_ASSERT(req3.success, "Requesting rank 3 when holding rank 2 must succeed: " + req3.message);

    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({1, 1, 1}), "All three ordered requests must be allocated");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold: " + err);
    return true;
}

// Test 5: Circular-Wait Rejection of Decreasing / Non-Increasing Rank Requests
bool testCircularWaitRejectionDecreasingRank() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);

    rm.addResourceType("R0", 5, 1); // Rank 1
    rm.addResourceType("R1", 5, 2); // Rank 2
    rm.addResourceType("R2", 5, 3); // Rank 3

    int p0 = rm.addProcess("P0");

    // P0 first acquires R1 (Rank 2)
    auto req1 = rm.submitRequest(p0, {0, 1, 0});
    TEST_ASSERT(req1.success, "Acquiring R1 must succeed");

    // P0 now attempts to request R0 (Rank 1 < Rank 2)
    auto reqViolate = rm.submitRequest(p0, {1, 0, 0});
    TEST_ASSERT(!reqViolate.success, "Requesting lower rank must be REJECTED by Circular-Wait policy");
    TEST_ASSERT(reqViolate.message.find("Circular-Wait violation") != std::string::npos,
                "Error message must specify Circular-Wait violation");

    // Verify state was NOT corrupted
    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({0, 1, 0}), "Allocation must remain [0, 1, 0]");
    TEST_ASSERT(state.available == std::vector<int>({5, 4, 5}), "Available must remain [5, 4, 5]");

    // Verify rejection log
    TEST_ASSERT(!rm.getRejectionLog().empty(), "Rejection log must record the event");
    TEST_ASSERT(rm.getRejectionLog().back().reason.find("strictly follow increasing order") != std::string::npos,
                "Log must record increasing order rationale");

    std::string err;
    TEST_ASSERT(rm.verifyInvariants(err), "Invariants must hold: " + err);
    return true;
}

// Test 6: Circular-Wait Rejection of Equal Rank Requests
bool testCircularWaitRejectionEqualRank() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);

    rm.addResourceType("R0", 5, 1);
    rm.addResourceType("R1", 5, 2);

    int p0 = rm.addProcess("P0");

    // Allocate 1 instance of R1 (Rank 2)
    rm.submitRequest(p0, {0, 1});

    // Attempt to request another instance of R1 (Rank 2 <= Rank 2)
    auto reqEqual = rm.submitRequest(p0, {0, 1});
    TEST_ASSERT(!reqEqual.success, "Requesting equal rank must be REJECTED under strict increasing order");

    return true;
}

// Test 7: Circular-Wait Multi-Resource Request containing an Out-of-Order Rank
bool testCircularWaitMultiResourceVectorRejection() {
    ResourceManager rm;
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);

    rm.addResourceType("A", 10, 1); // Rank 1
    rm.addResourceType("B", 10, 2); // Rank 2
    rm.addResourceType("C", 10, 3); // Rank 3

    int p0 = rm.addProcess("P0");

    // Hold resource B (rank 2)
    rm.submitRequest(p0, {0, 1, 0});

    // Request both A (rank 1 - violation) and C (rank 3 - valid) together
    auto reqMulti = rm.submitRequest(p0, {1, 0, 1});
    TEST_ASSERT(!reqMulti.success, "Multi-resource request with any violating rank must be rejected as a whole");

    const auto& state = rm.getState();
    TEST_ASSERT(state.allocation[p0] == std::vector<int>({0, 1, 0}), "No resources from rejected vector allocated");
    TEST_ASSERT(state.available == std::vector<int>({10, 9, 10}), "Available remains intact");

    return true;
}

// Test 8: Dynamic Policy Switching
bool testDynamicPolicySwitching() {
    ResourceManager rm;
    rm.addResourceType("R0", 5, 1);
    rm.addResourceType("R1", 5, 2);
    int p0 = rm.addProcess("P0");

    // Start with NONE (Unconstrained)
    rm.submitRequest(p0, {0, 1}); // Hold R1 (rank 2)
    auto unconstrained = rm.submitRequest(p0, {1, 0}); // Request R0 (rank 1)
    TEST_ASSERT(unconstrained.success, "Under NONE policy, out-of-order request is permitted");

    // Reset and switch to CIRCULAR_WAIT
    rm.reset();
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);
    rm.addResourceType("R0", 5, 1);
    rm.addResourceType("R1", 5, 2);
    p0 = rm.addProcess("P0");

    rm.submitRequest(p0, {0, 1});
    auto restricted = rm.submitRequest(p0, {1, 0});
    TEST_ASSERT(!restricted.success, "Under CIRCULAR_WAIT policy, out-of-order request is rejected");

    return true;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " DEADLOCK SIMULATOR: PHASE 3 PREVENTION TEST HARNESS\n";
    std::cout << "========================================================\n\n";

    int passedCount = 0;
    int failedCount = 0;

    RUN_TEST(testHoldAndWaitValidUpfront);
    RUN_TEST(testHoldAndWaitRejectionIncremental);
    RUN_TEST(testHoldAndWaitZeroPartialAllocation);
    RUN_TEST(testCircularWaitValidIncreasingOrder);
    RUN_TEST(testCircularWaitRejectionDecreasingRank);
    RUN_TEST(testCircularWaitRejectionEqualRank);
    RUN_TEST(testCircularWaitMultiResourceVectorRejection);
    RUN_TEST(testDynamicPolicySwitching);

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "PREVENTION TEST RESULTS: " << passedCount << " Passed, " << failedCount << " Failed.\n";
    std::cout << "========================================================\n";

    return (failedCount == 0) ? 0 : 1;
}
