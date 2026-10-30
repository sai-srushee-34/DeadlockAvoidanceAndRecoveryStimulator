#include "core/ResourceManager.hpp"
#include "detection/DetectionEngine.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace deadlock::core;
using namespace deadlock::detection;

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

// Test 1: No Deadlock (All processes can reduce and complete)
bool testNoDeadlock() {
    ResourceManager rm;
    rm.addResourceType("R0", 5);
    rm.addResourceType("R1", 5);

    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");

    // P0 holds [2, 1], P1 holds [1, 2]. Available is [2, 2].
    rm.allocateDirect(p0, {2, 1});
    rm.allocateDirect(p1, {1, 2});

    // P0 requests [1, 1], P1 requests [2, 2]
    rm.submitRequest(p0, {1, 1}); // Available has [2, 2] -> P0 can finish
    rm.submitRequest(p1, {2, 2}); // Currently waits, but P0's completion frees enough

    auto result = rm.detectDeadlock();
    TEST_ASSERT(result.validState, "State must be valid");
    TEST_ASSERT(!result.isDeadlocked, "System must NOT be deadlocked");
    TEST_ASSERT(result.deadlockedProcessIds.empty(), "Deadlocked process set must be empty");
    TEST_ASSERT(result.reducibleProcessIds.size() == 2, "Both processes must be reducible");

    return true;
}

// Test 2: Classic Two-Process Circular Deadlock
bool testTwoProcessCycleDeadlock() {
    ResourceManager rm;
    rm.addResourceType("MutexA", 1);
    rm.addResourceType("MutexB", 1);

    int p0 = rm.addProcess("Thread_0");
    int p1 = rm.addProcess("Thread_1");

    // P0 acquires MutexA, P1 acquires MutexB -> Available is [0, 0]
    rm.allocateDirect(p0, {1, 0});
    rm.allocateDirect(p1, {0, 1});

    // P0 requests MutexB, P1 requests MutexA
    rm.submitRequest(p0, {0, 1}); // Insufficient available -> Queued in Request matrix
    rm.submitRequest(p1, {1, 0}); // Insufficient available -> Queued in Request matrix

    auto result = rm.detectDeadlock();
    TEST_ASSERT(result.validState, "State must be valid");
    TEST_ASSERT(result.isDeadlocked, "System must be DEADLOCKED");
    TEST_ASSERT(result.deadlockedProcessIds.size() == 2, "Both processes must be in deadlocked set");
    TEST_ASSERT(result.deadlockedProcessIds[0] == p0 && result.deadlockedProcessIds[1] == p1,
                "Deadlocked set must be {0, 1}");

    return true;
}

// Test 3: Multiple Instances of Resource Types (Silberschatz Canonical Benchmark)
bool testMultipleInstancesBenchmark() {
    ResourceManager rm;
    // Capacity: A: 7, B: 2, C: 6
    rm.addResourceType("A", 7);
    rm.addResourceType("B", 2);
    rm.addResourceType("C", 6);

    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");
    int p2 = rm.addProcess("P2");
    int p3 = rm.addProcess("P3");
    int p4 = rm.addProcess("P4");

    // Allocations:
    // P0: [0, 1, 0], P1: [2, 0, 0], P2: [3, 0, 3], P3: [2, 1, 1], P4: [0, 0, 2]
    // Total Allocated: [7, 2, 6] -> Available: [0, 0, 0]
    rm.allocateDirect(p0, {0, 1, 0});
    rm.allocateDirect(p1, {2, 0, 0});
    rm.allocateDirect(p2, {3, 0, 3});
    rm.allocateDirect(p3, {2, 1, 1});
    rm.allocateDirect(p4, {0, 0, 2});

    // Sub-case 3A: Non-deadlocked request matrix
    // Requests: P0: [0, 0, 0], P1: [2, 0, 2], P2: [0, 0, 0], P3: [1, 0, 0], P4: [0, 0, 2]
    rm.submitRequest(p0, {0, 0, 0});
    rm.submitRequest(p1, {2, 0, 2});
    rm.submitRequest(p2, {0, 0, 0});
    rm.submitRequest(p3, {1, 0, 0});
    rm.submitRequest(p4, {0, 0, 2});

    auto res3A = rm.detectDeadlock();
    TEST_ASSERT(!res3A.isDeadlocked, "Sub-case 3A must NOT be deadlocked (P0 and P2 can finish first)");
    TEST_ASSERT(res3A.deadlockedProcessIds.empty(), "Deadlocked set must be empty in 3A");

    // Sub-case 3B: Deadlock Injection (P2 now requests [0, 0, 1])
    rm.submitRequest(p2, {0, 0, 1});

    auto res3B = rm.detectDeadlock();
    TEST_ASSERT(res3B.isDeadlocked, "Sub-case 3B must be DEADLOCKED");
    // P0 finishes (releases [0, 1, 0]), but that is insufficient for P1, P2, P3, P4
    std::vector<int> expectedDeadlock = {p1, p2, p3, p4};
    TEST_ASSERT(res3B.deadlockedProcessIds == expectedDeadlock,
                "Deadlocked set in 3B must precisely be {1, 2, 3, 4}");

    return true;
}

// Test 4: Processes with Zero Allocations
bool testZeroAllocationProcessHandling() {
    ResourceManager rm;
    rm.addResourceType("R0", 3);
    rm.addResourceType("R1", 3);

    int p0 = rm.addProcess("HoldingWorker");
    int pZero = rm.addProcess("ZeroAllocWorker");

    // P0 holds [2, 2], Available is [1, 1]
    rm.allocateDirect(p0, {2, 2});

    // P_zero holds 0 resources and requests [2, 2] (more than available)
    rm.submitRequest(pZero, {2, 2});

    // P0 requests [1, 0] <= Available [1, 1]
    rm.submitRequest(p0, {1, 0});

    auto result = rm.detectDeadlock();
    TEST_ASSERT(!result.isDeadlocked, "System must NOT be deadlocked");
    // P_zero holds 0 resources so it cannot block any other process.
    // Finish[P_zero] is initialized to true.
    // P0 can acquire its request [1, 0] and complete, freeing [3, 2].
    // Then P_zero can acquire its request and complete.
    TEST_ASSERT(result.deadlockedProcessIds.empty(), "No processes should be deadlocked");

    return true;
}

// Test 5: Several Unfinished Processes where Only Some are Deadlocked
bool testPartialDeadlockSubset() {
    ResourceManager rm;
    rm.addResourceType("R_Shared", 2);
    rm.addResourceType("R_CycleA", 1);
    rm.addResourceType("R_CycleB", 1);

    int pA = rm.addProcess("CycleA");
    int pB = rm.addProcess("CycleB");
    int pSafeHolding = rm.addProcess("SafeHolding");
    int pSafeWaiting = rm.addProcess("SafeWaiting");

    // Cycle between pA and pB on R_CycleA and R_CycleB
    rm.allocateDirect(pA, {0, 1, 0});
    rm.allocateDirect(pB, {0, 0, 1});
    rm.submitRequest(pA, {0, 0, 1}); // pA waits for R_CycleB
    rm.submitRequest(pB, {0, 1, 0}); // pB waits for R_CycleA

    // Independent safe pair on R_Shared:
    // pSafeHolding holds both units of R_Shared and needs nothing.
    // pSafeWaiting requests 1 unit of R_Shared.
    rm.allocateDirect(pSafeHolding, {2, 0, 0});
    rm.submitRequest(pSafeWaiting, {1, 0, 0}); // Blocked waiting for R_Shared

    auto result = rm.detectDeadlock();
    TEST_ASSERT(result.isDeadlocked, "System has a deadlock");

    // pSafeHolding finishes (Request is 0 <= Work), releases R_Shared [2, 0, 0].
    // Then pSafeWaiting receives R_Shared and finishes!
    // But pA and pB cannot finish!
    std::vector<int> expectedDeadlocked = {pA, pB};
    TEST_ASSERT(result.deadlockedProcessIds == expectedDeadlocked,
                "Only {pA, pB} must be deadlocked; safe processes must reduce successfully");

    return true;
}

// Test 6: No Outstanding Requests
bool testNoOutstandingRequests() {
    ResourceManager rm;
    rm.addResourceType("CPU", 10);
    rm.addResourceType("RAM", 10);

    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");

    rm.allocateDirect(p0, {4, 3});
    rm.allocateDirect(p1, {5, 5});

    // Zero requests submitted -> Request matrix is all zeros
    auto result = rm.detectDeadlock();
    TEST_ASSERT(!result.isDeadlocked, "State with 0 requests cannot be deadlocked");
    TEST_ASSERT(result.deadlockedProcessIds.empty(), "Deadlocked set must be empty");
    TEST_ASSERT(result.finalWork == std::vector<int>({10, 10}), "Final work must match total capacity");

    return true;
}

// Test 7: Invalid or Inconsistent System-State Input
bool testInvalidStateInputHandling() {
    // Manually construct a corrupted SystemState to test detection robustness
    SystemState corruptedState;
    corruptedState.resources.push_back({0, "R0", 5, 1});
    corruptedState.processes.push_back({0, "P0", 5, ProcessState::RUNNING, 0, 10, 0, {}});

    // Corrupt conservation law: Available (10) + Allocated (0) != Capacity (5)
    corruptedState.available = {10};
    corruptedState.allocation = {{0}};
    corruptedState.request = {{0}};
    corruptedState.maxDemand = {{0}};
    corruptedState.need = {{0}};

    auto result = DetectionEngine::detectDeadlock(corruptedState);
    TEST_ASSERT(!result.validState, "Corrupted state must be recognized as invalid");
    TEST_ASSERT(!result.isDeadlocked, "Invalid state must not falsely report a deadlock");
    TEST_ASSERT(!result.stateError.empty(), "State error description must be populated");
    TEST_ASSERT(result.diagnosticReport.find("INVALID SYSTEM STATE") != std::string::npos,
                "Diagnostic report must explain the invariant violation");

    return true;
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " DEADLOCK SIMULATOR: PHASE 4 DETECTION TEST HARNESS\n";
    std::cout << "========================================================\n\n";

    int passedCount = 0;
    int failedCount = 0;

    RUN_TEST(testNoDeadlock);
    RUN_TEST(testTwoProcessCycleDeadlock);
    RUN_TEST(testMultipleInstancesBenchmark);
    RUN_TEST(testZeroAllocationProcessHandling);
    RUN_TEST(testPartialDeadlockSubset);
    RUN_TEST(testNoOutstandingRequests);
    RUN_TEST(testInvalidStateInputHandling);

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "DETECTION TEST RESULTS: " << passedCount << " Passed, " << failedCount << " Failed.\n";
    std::cout << "========================================================\n";

    return (failedCount == 0) ? 0 : 1;
}
