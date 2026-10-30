#include "core/ResourceManager.hpp"
#include "recovery/RecoveryEngine.hpp"
#include "detection/DetectionEngine.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>

using namespace deadlock::core;
using namespace deadlock::recovery;
using namespace deadlock::detection;

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
        if (i + 1 < vec.size()) os << ", ";
    }
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, RecoveryStatus status) {
    return os << recoveryStatusToString(status);
}

inline std::ostream& operator<<(std::ostream& os, ProcessState state) {
    return os << processStateToString(state);
}

#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion FAILED: " << message << "\n  at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_FALSE(condition, message) ASSERT_TRUE(!(condition), message)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            std::cerr << "Assertion FAILED: " << message << "\n  Expected: " << (expected) \
                      << "\n  Actual:   " << (actual) << "\n  at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

// TC 1: Recovery when no deadlock exists
void testRecoveryWhenNoDeadlockExists() {
    std::cout << "[RUNNING] testRecoveryWhenNoDeadlockExists..." << std::flush;

    ResourceManager mgr;
    mgr.addResourceType("CPU", 4);
    mgr.addResourceType("MEM", 4);

    mgr.addProcess("P0", 5);
    mgr.addProcess("P1", 5);

    mgr.allocateDirect(0, {1, 0});
    mgr.allocateDirect(1, {0, 1});

    // P0 and P1 request resources that can easily be satisfied
    mgr.submitRequest(0, {0, 1}); // Available has MEM=3, so P0 is satisfied immediately

    auto det = mgr.detectDeadlock();
    ASSERT_FALSE(det.isDeadlocked, "System must not be deadlocked initially.");

    // Run recovery auto
    auto resAuto = mgr.recoverDeadlockAuto();
    ASSERT_EQ(resAuto.status, RecoveryStatus::RECOVERY_NOT_NEEDED, "Status must be RECOVERY_NOT_NEEDED.");
    ASSERT_TRUE(resAuto.success, "Success flag should be true for not-needed.");
    ASSERT_TRUE(resAuto.steps.empty(), "No recovery steps should be executed.");
    ASSERT_TRUE(resAuto.terminatedProcessIds.empty(), "No processes should be terminated.");
    ASSERT_EQ(resAuto.totalResourcesReclaimed, std::vector<int>({0, 0}), "Zero resources should be reclaimed.");

    // Run recovery manual on P0
    auto resManual = mgr.recoverDeadlockManual(0);
    ASSERT_EQ(resManual.status, RecoveryStatus::RECOVERY_NOT_NEEDED, "Manual recovery must also be RECOVERY_NOT_NEEDED.");
    ASSERT_TRUE(resManual.steps.empty(), "No steps should be executed.");

    // Invariants must hold
    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Invariants must hold after no-op recovery.");

    std::cout << " PASSED" << std::endl;
}

// TC 2: Recovery from a two-process circular wait
void testTwoProcessCircularWaitRecovery() {
    std::cout << "[RUNNING] testTwoProcessCircularWaitRecovery..." << std::flush;

    ResourceManager mgr;
    mgr.addResourceType("R0", 1);
    mgr.addResourceType("R1", 1);

    int p0 = mgr.addProcess("P0", 5);
    int p1 = mgr.addProcess("P1", 3); // Lower priority

    // P0 holds R0, P1 holds R1
    mgr.allocateDirect(p0, {1, 0});
    mgr.allocateDirect(p1, {0, 1});

    // P0 requests R1, P1 requests R0 -> Circular wait
    mgr.submitRequest(p0, {0, 1});
    mgr.submitRequest(p1, {1, 0});

    auto det = mgr.detectDeadlock();
    ASSERT_TRUE(det.isDeadlocked, "Two-process cycle must be deadlocked.");
    ASSERT_EQ(det.deadlockedProcessIds.size(), 2u, "Both P0 and P1 must be deadlocked.");

    // Manually select P1 as victim to terminate
    auto recRes = mgr.recoverDeadlockManual(p1);
    ASSERT_EQ(recRes.status, RecoveryStatus::RECOVERY_SUCCESSFUL, "Recovery should succeed.");
    ASSERT_TRUE(recRes.success, "Success should be true.");
    ASSERT_EQ(recRes.steps.size(), 1u, "Exactly 1 recovery step executed.");
    ASSERT_EQ(recRes.terminatedProcessIds.size(), 1u, "1 process terminated.");
    ASSERT_EQ(recRes.terminatedProcessIds[0], p1, "P1 should be terminated.");
    ASSERT_EQ(recRes.totalResourcesReclaimed, std::vector<int>({0, 1}), "R1 should be reclaimed.");

    // Verify P1 state in authoritative engine
    const auto& state = mgr.getState();
    ASSERT_EQ(state.processes[p1].state, ProcessState::TERMINATED, "P1 state must be TERMINATED.");
    ASSERT_EQ(state.allocation[p1], std::vector<int>({0, 0}), "P1 allocation must be zeroed.");
    ASSERT_EQ(state.request[p1], std::vector<int>({0, 0}), "P1 request must be cleared.");

    // Verify Available vector updated
    ASSERT_EQ(state.available, std::vector<int>({0, 1}), "Available must now contain R1.");

    // Verify post-recovery detection
    ASSERT_FALSE(recRes.finalDetection.isDeadlocked, "Final state must not be deadlocked.");
    ASSERT_TRUE(recRes.finalDetection.deadlockedProcessIds.empty(), "Deadlocked set must be empty.");

    // Verify P0 can now have its pending request granted
    int granted = mgr.processPendingRequests();
    ASSERT_EQ(granted, 1, "P0 pending request should now be granted.");
    ASSERT_EQ(state.processes[p0].state, ProcessState::RUNNING, "P0 should now be RUNNING.");
    ASSERT_EQ(state.allocation[p0], std::vector<int>({1, 1}), "P0 should hold both R0 and R1.");

    // Verify invariants
    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Invariants must hold after recovery and grant.");

    std::cout << " PASSED" << std::endl;
}

// TC 3: Multiple deadlocked processes (Canonical Silberschatz benchmark)
void testMultipleDeadlockedProcesses() {
    std::cout << "[RUNNING] testMultipleDeadlockedProcesses..." << std::flush;

    ResourceManager mgr;
    mgr.addResourceType("A", 7);
    mgr.addResourceType("B", 2);
    mgr.addResourceType("C", 6);

    for (int i = 0; i < 5; ++i) {
        mgr.addProcess("P" + std::to_string(i), (i == 2) ? 2 : 5); // P2 has low priority
    }

    // Allocation Matrix
    mgr.allocateDirect(0, {0, 1, 0});
    mgr.allocateDirect(1, {2, 0, 0});
    mgr.allocateDirect(2, {3, 0, 3});
    mgr.allocateDirect(3, {2, 1, 1});
    mgr.allocateDirect(4, {0, 0, 2});

    // Requests with injected deadlock (P2 requests [0, 0, 1])
    mgr.submitRequest(0, {0, 0, 0});
    mgr.submitRequest(1, {2, 0, 2});
    mgr.submitRequest(2, {0, 0, 1});
    mgr.submitRequest(3, {1, 0, 0});
    mgr.submitRequest(4, {0, 0, 2});

    auto det = mgr.detectDeadlock();
    ASSERT_TRUE(det.isDeadlocked, "Injected benchmark must be deadlocked.");
    ASSERT_EQ(det.deadlockedProcessIds.size(), 4u, "P1, P2, P3, P4 must be deadlocked.");

    // Use HIGHEST_ALLOCATION heuristic (P2 holds [3, 0, 3] = 6 units, highest in system)
    auto recRes = mgr.recoverDeadlockAuto(VictimSelectionPolicy::HIGHEST_ALLOCATION);
    ASSERT_EQ(recRes.status, RecoveryStatus::RECOVERY_SUCCESSFUL, "Recovery must succeed.");
    ASSERT_TRUE(recRes.success, "Success must be true.");
    ASSERT_EQ(recRes.steps.size(), 1u, "Deadlock must be completely resolved in 1 optimal step.");
    ASSERT_EQ(recRes.terminatedProcessIds[0], 2, "P2 must be chosen as the victim.");
    ASSERT_EQ(recRes.totalResourcesReclaimed, std::vector<int>({3, 0, 3}), "P2 held resources reclaimed.");

    ASSERT_FALSE(recRes.finalDetection.isDeadlocked, "Post-recovery state must be completely free of deadlock.");

    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Invariants must be preserved.");

    std::cout << " PASSED" << std::endl;
}

// TC 4: Invalid victim selection
void testInvalidVictimSelection() {
    std::cout << "[RUNNING] testInvalidVictimSelection..." << std::flush;

    ResourceManager mgr;
    mgr.addResourceType("R0", 1);
    mgr.addResourceType("R1", 1);

    mgr.addProcess("P0", 5);
    mgr.addProcess("P1", 5);
    mgr.addProcess("P2", 5); // Non-deadlocked process

    // P0 and P1 deadlock
    mgr.allocateDirect(0, {1, 0});
    mgr.allocateDirect(1, {0, 1});
    mgr.submitRequest(0, {0, 1});
    mgr.submitRequest(1, {1, 0});

    // P2 holds nothing, requests nothing
    auto det = mgr.detectDeadlock();
    ASSERT_TRUE(det.isDeadlocked, "P0 and P1 are deadlocked.");
    ASSERT_EQ(det.deadlockedProcessIds.size(), 2u, "Only P0 and P1 in set D.");

    // 4a. Out of bounds negative ID
    auto resNeg = mgr.recoverDeadlockManual(-1);
    ASSERT_EQ(resNeg.status, RecoveryStatus::RECOVERY_FAILED, "Negative ID must fail.");
    ASSERT_FALSE(resNeg.success, "Must not report success.");
    ASSERT_TRUE(resNeg.message.find("Invalid victim process ID") != std::string::npos, "Descriptive error message.");

    // 4b. Out of bounds high ID
    auto resHigh = mgr.recoverDeadlockManual(99);
    ASSERT_EQ(resHigh.status, RecoveryStatus::RECOVERY_FAILED, "Out-of-range ID must fail.");

    // 4c. Process P2 exists and is valid, but NOT in deadlocked set D
    auto resNonDeadlocked = mgr.recoverDeadlockManual(2);
    ASSERT_EQ(resNonDeadlocked.status, RecoveryStatus::RECOVERY_FAILED, "Selecting non-deadlocked process must fail.");
    ASSERT_TRUE(resNonDeadlocked.message.find("not a member of the active deadlocked set D") != std::string::npos, "Explains why.");

    // 4d. Terminate P0, then try to select P0 again
    auto resValid = mgr.recoverDeadlockManual(0);
    ASSERT_EQ(resValid.status, RecoveryStatus::RECOVERY_SUCCESSFUL, "First termination succeeds.");

    // Re-create a deadlock to test terminated process rejection
    mgr.allocateDirect(1, {1, 0});
    mgr.submitRequest(1, {0, 1});
    // P0 is already TERMINATED
    auto resAlreadyTerm = mgr.recoverDeadlockManual(0);
    ASSERT_EQ(resAlreadyTerm.status, RecoveryStatus::RECOVERY_FAILED, "Terminated process cannot be selected.");
    ASSERT_TRUE(resAlreadyTerm.message.find("already TERMINATED") != std::string::npos, "Explains already terminated.");

    // Invariants must hold
    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Invariants must hold after invalid selections.");

    std::cout << " PASSED" << std::endl;
}

// TC 5: Repeated detection after termination (Multi-cycle deadlock)
void testRepeatedDetectionAfterTermination() {
    std::cout << "[RUNNING] testRepeatedDetectionAfterTermination..." << std::flush;

    ResourceManager mgr;
    // Two independent cycles:
    // Cycle 1: P0 holds R0, requests R1; P1 holds R1, requests R0.
    // Cycle 2: P2 holds R2, requests R3; P3 holds R3, requests R2.
    mgr.addResourceType("R0", 1);
    mgr.addResourceType("R1", 1);
    mgr.addResourceType("R2", 1);
    mgr.addResourceType("R3", 1);

    for (int i = 0; i < 4; ++i) mgr.addProcess("P" + std::to_string(i), 5);

    mgr.allocateDirect(0, {1, 0, 0, 0});
    mgr.allocateDirect(1, {0, 1, 0, 0});
    mgr.allocateDirect(2, {0, 0, 1, 0});
    mgr.allocateDirect(3, {0, 0, 0, 1});

    mgr.submitRequest(0, {0, 1, 0, 0});
    mgr.submitRequest(1, {1, 0, 0, 0});
    mgr.submitRequest(2, {0, 0, 0, 1});
    mgr.submitRequest(3, {0, 0, 1, 0});

    auto detInit = mgr.detectDeadlock();
    ASSERT_TRUE(detInit.isDeadlocked, "Both cycles deadlocked.");
    ASSERT_EQ(detInit.deadlockedProcessIds.size(), 4u, "All 4 processes deadlocked.");

    // Step 1: Terminate P0 manually (breaks Cycle 1, but Cycle 2 persists)
    auto step1 = mgr.recoverDeadlockManual(0);
    ASSERT_EQ(step1.status, RecoveryStatus::RECOVERY_PARTIAL, "Status must be RECOVERY_PARTIAL.");
    ASSERT_FALSE(step1.success, "Success is false because deadlock still exists.");
    ASSERT_EQ(step1.steps.size(), 1u, "1 step recorded.");
    ASSERT_EQ(step1.finalDetection.deadlockedProcessIds.size(), 2u, "P2 and P3 must still be deadlocked.");
    ASSERT_EQ(step1.finalDetection.deadlockedProcessIds, std::vector<int>({2, 3}), "Remaining deadlocked processes.");

    // Step 2: Terminate P2 manually (breaks Cycle 2)
    auto step2 = mgr.recoverDeadlockManual(2);
    ASSERT_EQ(step2.status, RecoveryStatus::RECOVERY_SUCCESSFUL, "Status must now be RECOVERY_SUCCESSFUL.");
    ASSERT_TRUE(step2.success, "Success is true.");
    ASSERT_FALSE(step2.finalDetection.isDeadlocked, "No deadlocks remain.");

    // Verify invariants
    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Invariants preserved throughout multi-step recovery.");

    std::cout << " PASSED" << std::endl;
}

// TC 6: Resource conservation after recovery
void testResourceConservationAfterRecovery() {
    std::cout << "[RUNNING] testResourceConservationAfterRecovery..." << std::flush;

    ResourceManager mgr;
    mgr.addResourceType("R0", 5);
    mgr.addResourceType("R1", 7);
    mgr.addResourceType("R2", 4);

    mgr.addProcess("P0", 8);
    mgr.addProcess("P1", 2);
    mgr.addProcess("P2", 4);

    mgr.allocateDirect(0, {2, 3, 1});
    mgr.allocateDirect(1, {2, 2, 2});
    mgr.allocateDirect(2, {1, 2, 1});

    // Invariant check before deadlock
    std::string err;
    ASSERT_TRUE(mgr.verifyInvariants(err), "Conservation before deadlock.");

    // Inject deadlock
    mgr.submitRequest(0, {1, 1, 1});
    mgr.submitRequest(1, {1, 1, 1});
    mgr.submitRequest(2, {1, 1, 1});

    auto det = mgr.detectDeadlock();
    ASSERT_TRUE(det.isDeadlocked, "System is deadlocked.");

    // Check conservation during deadlock
    ASSERT_TRUE(mgr.verifyInvariants(err), "Conservation during deadlock.");

    // Run automated recovery
    auto rec = mgr.recoverDeadlockAuto(VictimSelectionPolicy::LOWEST_PRIORITY);
    ASSERT_TRUE(rec.success, "Recovery must succeed.");

    // Check conservation after recovery
    ASSERT_TRUE(mgr.verifyInvariants(err), "Conservation immediately after recovery.");

    // Verify terminated processes have 0 allocation and 0 request
    const auto& state = mgr.getState();
    for (int victimId : rec.terminatedProcessIds) {
        ASSERT_EQ(state.processes[victimId].state, ProcessState::TERMINATED, "Victim must be TERMINATED.");
        ASSERT_EQ(state.allocation[victimId], std::vector<int>({0, 0, 0}), "Victim allocation zeroed.");
        ASSERT_EQ(state.request[victimId], std::vector<int>({0, 0, 0}), "Victim request zeroed.");
    }

    // Complete all remaining alive processes
    for (int i = 0; i < state.numProcesses(); ++i) {
        if (state.processes[i].state != ProcessState::TERMINATED) {
            mgr.completeProcess(i);
        }
    }

    // Invariant check: all resources reclaimed to capacity
    ASSERT_TRUE(mgr.verifyInvariants(err), "Conservation after all processes completed.");
    ASSERT_EQ(mgr.getState().available, std::vector<int>({5, 7, 4}), "All capacity must be available.");

    std::cout << " PASSED" << std::endl;
}

// TC 7: Heuristic victim selection comparison
void testHeuristicVictimSelection() {
    std::cout << "[RUNNING] testHeuristicVictimSelection..." << std::flush;

    // Test LOWEST_PRIORITY vs HIGHEST_ALLOCATION vs MIN_TERMINATION_COST
    SystemState state;
    ResourceType r0; r0.id = 0; r0.name = "R0"; r0.totalCapacity = 10;
    state.resources.push_back(r0);

    Process p0; p0.id = 0; p0.name = "P0"; p0.priority = 2; p0.cpuTimeUsed = 100;
    Process p1; p1.id = 1; p1.name = "P1"; p1.priority = 9; p1.cpuTimeUsed = 1;
    state.processes.push_back(p0);
    state.processes.push_back(p1);

    state.available = {0};
    state.allocation = {{1}, {8}}; // P0 holds 1, P1 holds 8
    state.request = {{1}, {1}};
    state.maxDemand = {{2}, {9}};
    state.need = {{1}, {1}};

    std::vector<int> deadlocked = {0, 1};

    // Lowest priority should pick P0 (priority 2 < 9)
    int victimPri = RecoveryEngine::selectVictim(state, deadlocked, VictimSelectionPolicy::LOWEST_PRIORITY);
    ASSERT_EQ(victimPri, 0, "LOWEST_PRIORITY should select P0.");

    // Highest allocation should pick P1 (holds 8 > 1)
    int victimAlloc = RecoveryEngine::selectVictim(state, deadlocked, VictimSelectionPolicy::HIGHEST_ALLOCATION);
    ASSERT_EQ(victimAlloc, 1, "HIGHEST_ALLOCATION should select P1.");

    // Cost formula:
    // P0: (2 * 10) + (100 * 2) - (1 * 3) = 20 + 200 - 3 = 217
    // P1: (9 * 10) + (1 * 2) - (8 * 3) = 90 + 2 - 24 = 68
    // Min cost should pick P1!
    int victimCost = RecoveryEngine::selectVictim(state, deadlocked, VictimSelectionPolicy::MIN_TERMINATION_COST);
    ASSERT_EQ(victimCost, 1, "MIN_TERMINATION_COST should select P1.");

    std::cout << " PASSED" << std::endl;
}

int main() {
    std::cout << "========================================================\n"
              << " DEADLOCK SIMULATOR: PHASE 5 RECOVERY TEST HARNESS\n"
              << "========================================================\n" << std::endl;

    testRecoveryWhenNoDeadlockExists();
    testTwoProcessCircularWaitRecovery();
    testMultipleDeadlockedProcesses();
    testInvalidVictimSelection();
    testRepeatedDetectionAfterTermination();
    testResourceConservationAfterRecovery();
    testHeuristicVictimSelection();

    std::cout << "\n--------------------------------------------------------\n"
              << "RECOVERY TEST RESULTS: 7 Passed, 0 Failed.\n"
              << "========================================================\n" << std::endl;

    return 0;
}
