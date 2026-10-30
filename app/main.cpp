#include "core/ResourceManager.hpp"
#include "prevention/PreventionEngine.hpp"
#include "detection/DetectionEngine.hpp"
#include "recovery/RecoveryEngine.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace deadlock::core;
using namespace deadlock::prevention;
using namespace deadlock::detection;
using namespace deadlock::recovery;

void printBanner() {
    std::cout << "========================================================\n";
    std::cout << "   DEADLOCK SIMULATION ENGINE - CONSOLE INTERFACE\n";
    std::cout << "   Course-Based Project (CBP) - Phase 5 Recovery\n";
    std::cout << "========================================================\n";
    std::cout << "Type 'help' for commands, 'demo' for core demo,\n";
    std::cout << "'demo_prevention' for prevention, 'demo_detection' for detection,\n";
    std::cout << "or 'demo_recovery' for deadlock recovery.\n\n";
}

void printHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  state                           - Display current process and resource states\n";
    std::cout << "  matrix                          - Display Allocation, Request, Max, and Need matrices\n";
    std::cout << "  detect                          - Execute multi-instance deadlock detection algorithm\n";
    std::cout << "  recover <victimId>              - Manually terminate victim process to recover from deadlock\n";
    std::cout << "  recover_auto [pri|alloc|cost]   - Automatically select victims and recover using heuristic\n";
    std::cout << "  preemption_info                 - Display architectural analysis of resource preemption\n";
    std::cout << "  policy <none|hold_wait|order>   - Configure active Deadlock Prevention policy\n";
    std::cout << "  rejections                      - Display log of rejected requests & policy violations\n";
    std::cout << "  add_res <name> <cap> [rank]     - Register a new resource type with capacity & order rank\n";
    std::cout << "  add_proc <name> [priority]      - Register a new process (1-10 priority)\n";
    std::cout << "  request <pId> <u1 u2 ...>       - Submit resource request for process\n";
    std::cout << "  release <pId> <u1 u2 ...>       - Release held resources from process\n";
    std::cout << "  complete <pId>                  - Complete process and reclaim all its resources\n";
    std::cout << "  sweep                           - Attempt to grant queued requests for WAITING processes\n";
    std::cout << "  verify                          - Check all system invariants and conservation laws\n";
    std::cout << "  demo                            - Run basic core resource simulation demo\n";
    std::cout << "  demo_prevention                 - Run Hold-and-Wait and Circular-Wait prevention demo\n";
    std::cout << "  demo_detection                  - Run Multi-Instance Deadlock Detection demo\n";
    std::cout << "  demo_recovery                   - Run Deadlock Recovery termination & heuristic demo\n";
    std::cout << "  reset                           - Clear all resources, processes, and policies\n";
    std::cout << "  help                            - Show this command reference\n";
    std::cout << "  exit                            - Quit the simulator\n\n";
}

void runCoreDemo(ResourceManager& rm) {
    std::cout << "\n>>> Starting Automated Core Simulation Demonstration <<<\n\n";
    rm.reset();

    std::cout << "[Step 1] Creating Resources: CPU (4 units), Memory (6 units), Disk (2 units)...\n";
    rm.addResourceType("CPU", 4, 1);
    rm.addResourceType("Memory", 6, 2);
    rm.addResourceType("Disk", 2, 3);

    std::cout << "[Step 2] Registering Processes: P0 (Worker), P1 (Database), P2 (Renderer)...\n";
    int p0 = rm.addProcess("P0_Worker", 5);
    int p1 = rm.addProcess("P1_Database", 8);
    int p2 = rm.addProcess("P2_Renderer", 4);

    std::cout << rm.formatState();

    std::cout << "[Step 3] Process P0 requests [2, 3, 1]...\n";
    auto r1 = rm.submitRequest(p0, {2, 3, 1});
    std::cout << "  -> Result: " << r1.message << "\n";

    std::cout << "[Step 4] Process P1 requests [1, 2, 1]...\n";
    auto r2 = rm.submitRequest(p1, {1, 2, 1});
    std::cout << "  -> Result: " << r2.message << "\n";

    std::cout << rm.formatMatrices();

    std::cout << "[Step 5] Process P2 requests [2, 2, 1] (Only 1 CPU and 0 Disks available)...\n";
    auto r3 = rm.submitRequest(p2, {2, 2, 1});
    std::cout << "  -> Result: " << r3.message << "\n";
    std::cout << "  -> Note: Request was NOT allocated silently. P2 transitioned to WAITING.\n";

    std::cout << rm.formatState();

    std::cout << "[Step 6] Process P0 completes execution...\n";
    auto r4 = rm.completeProcess(p0);
    std::cout << "  -> Result: " << r4.message << "\n";

    std::cout << "[Step 7] Sweeping queued requests to satisfy waiting processes...\n";
    int granted = rm.processPendingRequests();
    std::cout << "  -> Granted " << granted << " pending request(s). Process P2 is now RUNNING!\n";

    std::cout << rm.formatState();

    std::string err;
    if (rm.verifyInvariants(err)) {
        std::cout << "[Verification] All Operating Systems invariants and conservation laws HOLD!\n";
    } else {
        std::cout << "[Verification ERROR] " << err << "\n";
    }

    std::cout << "\n>>> Core Demonstration Completed Successfully <<<\n\n";
}

void runPreventionDemo(ResourceManager& rm) {
    std::cout << "\n========================================================\n";
    std::cout << "     DEADLOCK PREVENTION DEMONSTRATION (PHASE 3)\n";
    std::cout << "========================================================\n\n";

    std::cout << "--- SCENARIO 1: HOLD-AND-WAIT PREVENTION (Conservative All-or-Nothing) ---\n";
    rm.reset();
    rm.setPreventionPolicy(PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);

    rm.addResourceType("CPU", 4);
    rm.addResourceType("Memory", 6);
    int p0 = rm.addProcess("P0_Batch");
    int p1 = rm.addProcess("P1_Analytics");

    std::cout << "Active Policy: " << preventionPolicyToString(rm.getPreventionPolicy()) << "\n";

    std::cout << "\n[1.1] Process P0 requests all required resources upfront: [2, 3]...\n";
    auto h1 = rm.submitRequest(p0, {2, 3});
    std::cout << "  -> " << h1.message << "\n";

    std::cout << "\n[1.2] Process P0 attempts to request additional [1, 1] while holding [2, 3]...\n";
    auto h2 = rm.submitRequest(p0, {1, 1});
    std::cout << "  -> " << h2.message << "\n";
    std::cout << "  -> Rationale: Coffman Condition #2 invalidated. Incremental requests forbidden during execution.\n";

    std::cout << "\n[1.3] Process P1 requests [3, 2] (only 2 CPUs available)...\n";
    auto h3 = rm.submitRequest(p1, {3, 2});
    std::cout << "  -> " << h3.message << "\n";
    std::cout << "  -> Crucial Invariant: P1 holds ZERO partial allocations while waiting. Allocation[P1] = [0, 0].\n";

    std::cout << "\n--- SCENARIO 2: CIRCULAR-WAIT PREVENTION (Strict Resource Ordering F(R)) ---\n";
    rm.reset();
    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);

    rm.addResourceType("Disk", 5, 1);
    rm.addResourceType("Printer", 5, 2);
    rm.addResourceType("GPU", 5, 3);

    int pA = rm.addProcess("Process_A");
    int pB = rm.addProcess("Process_B");

    std::cout << "Active Policy: " << preventionPolicyToString(rm.getPreventionPolicy()) << "\n";
    std::cout << "Resource Ranks: Disk = 1, Printer = 2, GPU = 3\n\n";

    std::cout << "[2.1] Process_A requests Disk (rank 1)...\n";
    auto c1 = rm.submitRequest(pA, {1, 0, 0});
    std::cout << "  -> " << c1.message << "\n";

    std::cout << "[2.2] Process_A requests Printer (rank 2 > 1) -> Valid increasing order...\n";
    auto c2 = rm.submitRequest(pA, {0, 1, 0});
    std::cout << "  -> " << c2.message << "\n";

    std::cout << "[2.3] Process_B acquires GPU (rank 3)...\n";
    auto c3 = rm.submitRequest(pB, {0, 0, 1});
    std::cout << "  -> " << c3.message << "\n";

    std::cout << "[2.4] Process_B attempts to request Disk (rank 1 < 3) -> Out-of-order violation!\n";
    auto c4 = rm.submitRequest(pB, {1, 0, 0});
    std::cout << "  -> " << c4.message << "\n";
    std::cout << "  -> Mathematical Guarantee: Strict linear order F(R) breaks Circular Wait cycles.\n";

    std::cout << "\n" << rm.formatRejections();

    std::cout << "========================================================\n";
    std::cout << "   PREVENTION DEMONSTRATION COMPLETED SUCCESSFULLY\n";
    std::cout << "========================================================\n\n";
}

void runDetectionDemo(ResourceManager& rm) {
    std::cout << "\n========================================================\n";
    std::cout << "    DEADLOCK DETECTION DEMONSTRATION (PHASE 4)\n";
    std::cout << "    Shoshani-Coffman Multi-Instance Algorithm\n";
    std::cout << "========================================================\n\n";

    rm.reset();

    // Silberschatz Classical Benchmark: 5 processes, 3 resource types (A: 7, B: 2, C: 6)
    std::cout << "[Step 1] Initializing Resources: A (7 units), B (2 units), C (6 units)...\n";
    rm.addResourceType("A", 7);
    rm.addResourceType("B", 2);
    rm.addResourceType("C", 6);

    std::cout << "[Step 2] Initializing 5 Processes (P0 to P4)...\n";
    int p0 = rm.addProcess("P0");
    int p1 = rm.addProcess("P1");
    int p2 = rm.addProcess("P2");
    int p3 = rm.addProcess("P3");
    int p4 = rm.addProcess("P4");

    std::cout << "[Step 3] Allocating initial resource instances...\n";
    rm.allocateDirect(p0, {0, 1, 0});
    rm.allocateDirect(p1, {2, 0, 0});
    rm.allocateDirect(p2, {3, 0, 3});
    rm.allocateDirect(p3, {2, 1, 1});
    rm.allocateDirect(p4, {0, 0, 2});

    std::cout << "[Step 4] Submitting initial requests:\n"
              << "  P0: [0, 0, 0] | P1: [2, 0, 2] | P2: [0, 0, 0] | P3: [1, 0, 0] | P4: [0, 0, 2]\n";
    rm.submitRequest(p0, {0, 0, 0});
    rm.submitRequest(p1, {2, 0, 2});
    rm.submitRequest(p2, {0, 0, 0});
    rm.submitRequest(p3, {1, 0, 0});
    rm.submitRequest(p4, {0, 0, 2});

    std::cout << rm.formatMatrices();

    std::cout << ">>> Running Detection on Initial Benchmark State <<<\n";
    auto res1 = rm.detectDeadlock();
    std::cout << res1.diagnosticReport;

    std::cout << "\n>>> Step 5: Injecting Deadlock (Process P2 requests [0, 0, 1]) <<<\n";
    rm.submitRequest(p2, {0, 0, 1});
    std::cout << rm.formatMatrices();

    std::cout << ">>> Running Detection on Injected State <<<\n";
    auto res2 = rm.detectDeadlock();
    std::cout << res2.diagnosticReport;

    std::cout << "========================================================\n";
    std::cout << "   DETECTION DEMONSTRATION COMPLETED SUCCESSFULLY\n";
    std::cout << "========================================================\n\n";
}

void runRecoveryDemo(ResourceManager& rm) {
    std::cout << "\n========================================================\n";
    std::cout << "      DEADLOCK RECOVERY DEMONSTRATION (PHASE 5)\n";
    std::cout << "      Process Termination & Heuristic Victim Selection\n";
    std::cout << "========================================================\n\n";

    rm.reset();

    std::cout << "[Step 1] Initializing Resources: A (7 units), B (2 units), C (6 units)...\n";
    rm.addResourceType("A", 7);
    rm.addResourceType("B", 2);
    rm.addResourceType("C", 6);

    std::cout << "[Step 2] Initializing 5 Processes with distinct priorities and workloads:\n"
              << "  P0: Priority 5, CPU 10 | P1: Priority 8, CPU 40 | P2: Priority 3, CPU 5\n"
              << "  P3: Priority 7, CPU 20 | P4: Priority 4, CPU 15\n";
    int p0 = rm.addProcess("P0", 5);
    int p1 = rm.addProcess("P1", 8);
    int p2 = rm.addProcess("P2", 3);
    int p3 = rm.addProcess("P3", 7);
    int p4 = rm.addProcess("P4", 4);

    rm.allocateDirect(p0, {0, 1, 0});
    rm.allocateDirect(p1, {2, 0, 0});
    rm.allocateDirect(p2, {3, 0, 3});
    rm.allocateDirect(p3, {2, 1, 1});
    rm.allocateDirect(p4, {0, 0, 2});

    // Injected requests causing circular wait on P1, P2, P3, P4
    rm.submitRequest(p0, {0, 0, 0});
    rm.submitRequest(p1, {2, 0, 2});
    rm.submitRequest(p2, {0, 0, 1});
    rm.submitRequest(p3, {1, 0, 0});
    rm.submitRequest(p4, {0, 0, 2});

    std::cout << rm.formatMatrices();

    std::cout << ">>> Step 3: Running Pre-Recovery Deadlock Detection <<<\n";
    auto initDet = rm.detectDeadlock();
    std::cout << initDet.diagnosticReport;

    std::cout << ">>> Step 4: Evaluating Termination Costs of Deadlocked Candidates <<<\n";
    const auto& state = rm.getState();
    for (int pid : initDet.deadlockedProcessIds) {
        double cost = RecoveryEngine::calculateTerminationCost(state, pid);
        int held = 0;
        for (int u : state.allocation[pid]) held += u;
        std::cout << "  Process P" << pid << " (" << state.processes[pid].name
                  << "): Priority=" << state.processes[pid].priority
                  << ", HeldUnits=" << held
                  << " -> TerminationCost = " << cost << "\n";
    }

    std::cout << "\nNotice: Process P2 holds 6 total units (A:3, C:3) and has low priority (3).\n"
              << "Terminating lowest-ID process (P0 or P1) would be suboptimal. P2 is the optimal victim!\n";

    std::cout << "\n>>> Step 5: Executing Recovery with MIN_TERMINATION_COST Heuristic <<<\n";
    auto recRes = rm.recoverDeadlockAuto(VictimSelectionPolicy::MIN_TERMINATION_COST);
    std::cout << recRes.diagnosticReport;

    std::cout << ">>> Step 6: Granting Pending Requests for Remaining Processes <<<\n";
    int granted = rm.processPendingRequests();
    std::cout << "  Granted " << granted << " pending request(s) using newly liberated resources!\n";
    std::cout << rm.formatState();

    std::string err;
    if (rm.verifyInvariants(err)) {
        std::cout << "[Verification] All Operating Systems invariants and conservation laws HOLD!\n";
    } else {
        std::cout << "[Verification ERROR] " << err << "\n";
    }

    std::cout << "========================================================\n";
    std::cout << "   RECOVERY DEMONSTRATION COMPLETED SUCCESSFULLY\n";
    std::cout << "========================================================\n\n";
}

int main(int argc, char* argv[]) {
    ResourceManager rm;

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--demo") {
            printBanner();
            runCoreDemo(rm);
            return 0;
        } else if (arg == "--demo-prevention") {
            printBanner();
            runPreventionDemo(rm);
            return 0;
        } else if (arg == "--demo-detection") {
            printBanner();
            runDetectionDemo(rm);
            return 0;
        } else if (arg == "--demo-recovery") {
            printBanner();
            runRecoveryDemo(rm);
            return 0;
        }
    }

    printBanner();

    std::string line;
    while (true) {
        std::cout << "deadlock-sim> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) continue;

        if (cmd == "exit" || cmd == "quit") {
            std::cout << "Exiting Deadlock Simulation Engine. Goodbye!\n";
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "state") {
            std::cout << rm.formatState();
        } else if (cmd == "matrix") {
            std::cout << rm.formatMatrices();
        } else if (cmd == "detect") {
            auto res = rm.detectDeadlock();
            std::cout << res.diagnosticReport;
        } else if (cmd == "rejections") {
            std::cout << rm.formatRejections();
        } else if (cmd == "demo") {
            runCoreDemo(rm);
        } else if (cmd == "demo_prevention") {
            runPreventionDemo(rm);
        } else if (cmd == "demo_detection") {
            runDetectionDemo(rm);
        } else if (cmd == "reset") {
            rm.reset();
            std::cout << "Simulator state reset.\n";
        } else if (cmd == "policy") {
            std::string pol;
            if (iss >> pol) {
                if (pol == "none") {
                    rm.setPreventionPolicy(PreventionPolicy::NONE);
                    std::cout << "Prevention policy set to: NONE (Unconstrained)\n";
                } else if (pol == "hold_wait") {
                    rm.setPreventionPolicy(PreventionPolicy::HOLD_AND_WAIT_CONSERVATIVE);
                    std::cout << "Prevention policy set to: HOLD_AND_WAIT_CONSERVATIVE (All-or-Nothing)\n";
                } else if (pol == "order" || pol == "circular_wait") {
                    rm.setPreventionPolicy(PreventionPolicy::CIRCULAR_WAIT_TOTAL_ORDER);
                    std::cout << "Prevention policy set to: CIRCULAR_WAIT_TOTAL_ORDER (Strict Hierarchical Ordering)\n";
                } else {
                    std::cout << "Unknown policy: '" << pol << "'. Options: none, hold_wait, order\n";
                }
            } else {
                std::cout << "Current policy: " << preventionPolicyToString(rm.getPreventionPolicy()) << "\n";
                std::cout << "Usage: policy <none|hold_wait|order>\n";
            }
        } else if (cmd == "verify") {
            std::string err;
            if (rm.verifyInvariants(err)) {
                std::cout << "[OK] All invariants and conservation laws hold.\n";
            } else {
                std::cout << "[FAIL] Invariant violation: " << err << "\n";
            }
        } else if (cmd == "add_res") {
            std::string name;
            int cap = 0, rank = 0;
            if (iss >> name >> cap) {
                iss >> rank;
                int id = rm.addResourceType(name, cap, rank);
                if (id >= 0) {
                    std::cout << "Added resource type [" << id << "] '" << name << "' with capacity " << cap
                              << " (OrderRank: " << rm.getState().resources[id].orderRank << ")\n";
                } else {
                    std::cout << "Error: Capacity must be >= 1.\n";
                }
            } else {
                std::cout << "Usage: add_res <name> <capacity> [orderRank]\n";
            }
        } else if (cmd == "add_proc") {
            std::string name;
            int priority = 5;
            if (iss >> name) {
                iss >> priority;
                int id = rm.addProcess(name, priority);
                std::cout << "Added process [" << id << "] '" << name << "' with priority " << priority << "\n";
            } else {
                std::cout << "Usage: add_proc <name> [priority]\n";
            }
        } else if (cmd == "request") {
            int pId;
            if (iss >> pId) {
                std::vector<int> req;
                int val;
                while (iss >> val) req.push_back(val);
                auto res = rm.submitRequest(pId, req);
                std::cout << (res.success ? "[SUCCESS] " : "[DEFERRED/REJECTED] ") << res.message << "\n";
            } else {
                std::cout << "Usage: request <processId> <u1 u2 ...>\n";
            }
        } else if (cmd == "release") {
            int pId;
            if (iss >> pId) {
                std::vector<int> rel;
                int val;
                while (iss >> val) rel.push_back(val);
                auto res = rm.releaseResources(pId, rel);
                std::cout << (res.success ? "[SUCCESS] " : "[FAILED] ") << res.message << "\n";
            } else {
                std::cout << "Usage: release <processId> <u1 u2 ...>\n";
            }
        } else if (cmd == "complete") {
            int pId;
            if (iss >> pId) {
                auto res = rm.completeProcess(pId);
                std::cout << (res.success ? "[SUCCESS] " : "[FAILED] ") << res.message << "\n";
            } else {
                std::cout << "Usage: complete <processId>\n";
            }
        } else if (cmd == "sweep") {
            int count = rm.processPendingRequests();
            std::cout << "Granted " << count << " pending request(s).\n";
        } else if (cmd == "recover") {
            int victimId;
            if (iss >> victimId) {
                auto res = rm.recoverDeadlockManual(victimId);
                std::cout << res.diagnosticReport;
            } else {
                std::cout << "Usage: recover <victimProcessId>\n";
            }
        } else if (cmd == "recover_auto") {
            std::string polStr;
            VictimSelectionPolicy pol = VictimSelectionPolicy::LOWEST_PRIORITY;
            if (iss >> polStr) {
                if (polStr == "pri" || polStr == "priority") pol = VictimSelectionPolicy::LOWEST_PRIORITY;
                else if (polStr == "alloc" || polStr == "allocation") pol = VictimSelectionPolicy::HIGHEST_ALLOCATION;
                else if (polStr == "cost") pol = VictimSelectionPolicy::MIN_TERMINATION_COST;
                else {
                    std::cout << "Unknown policy: '" << polStr << "'. Options: pri, alloc, cost\n";
                    continue;
                }
            }
            auto res = rm.recoverDeadlockAuto(pol);
            std::cout << res.diagnosticReport;
        } else if (cmd == "preemption_info") {
            std::cout << RecoveryEngine::explainPreemptionRequirements();
        } else if (cmd == "demo_recovery") {
            runRecoveryDemo(rm);
        } else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for command list.\n";
        }
    }

    return 0;
}
