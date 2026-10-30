# Deadlock Prevention and Recovery System: Academic Project Report Outline

**Course:** Advanced Operating Systems & Systems Architecture  
**Project Type:** Course-Based Project (CBP)  
**Project Title:** Interactive Deadlock Prevention, Detection, and Recovery System with Native C++ Simulation Engine and Web-Based Bipartite RAG Visualization  
**Document Type:** Formal Project Report Structure & Academic Blueprint  
**Version:** 2.0.0 (Final Submission)

---

## Abstract

Deadlocks represent one of the most critical synchronization failures in concurrent multiprogramming operating systems. When concurrent processes compete for non-shareable resources, mutual dependency cycles can halt system progress permanently. This Course-Based Project (CBP) designs, implements, and empirically validates a high-performance, user-space Deadlock Management Simulator.

The architecture cleanly decouples an authoritative **Modern C++17 Core Engine** from an interactive **Web Presentation Dashboard** through a native, zero-dependency **Winsock HTTP/1.1 REST API Bridge**. The system models multi-instance resource allocation, verifies physical conservation laws ($\sum \text{Alloc} + \text{Avail} == \text{Cap}$), enforces Havender Coffman-condition prevention protocols (Strict Total Ordering and Conservative Hold-and-Wait), implements the Shoshani-Coffman multi-instance matrix reduction detection algorithm, executes multi-criteria heuristic victim recovery, and renders a live, interactive SVG-based bipartite Resource-Allocation Graph (RAG). 

Verification across 43 automated unit tests and interactive browser sessions demonstrates sub-millisecond API response latency, deterministic matrix reduction, and mathematical state consistency across all operating states.

---

## 1. Introduction & Project Motivation

### 1.1 The Deadlock Problem in Multiprogrammed Systems
- Concurrency, resource contention, and mutual exclusion.
- The fundamental cost of deadlocks: wasted CPU cycles, thread starvation, system lockups, and unrecoverable state loss.
- Motivation for building an interactive educational simulator bridging theoretical Operating Systems concepts with concrete systems software engineering.

### 1.2 Project Objectives & Scope
- **Objective 1:** Build a robust C++17 resource-management simulation engine modeling processes, resource types, and multiple instances per type.
- **Objective 2:** Implement static Deadlock Prevention protocols (Circular-Wait Total Ordering and Hold-and-Wait Invalidation).
- **Objective 3:** Implement dynamic Deadlock Avoidance foundations (Banker's Algorithm claim matrices: $\mathbf{Max}, \mathbf{Need}$).
- **Objective 4:** Implement runtime Deadlock Detection for multi-instance resource systems via Shoshani-Coffman matrix reduction.
- **Objective 5:** Implement Deadlock Recovery via manual victim selection and configurable cost-based heuristics.
- **Objective 6:** Provide an intuitive, interactive web frontend with SVG Bipartite Graph visualization connected via a native C++ HTTP REST API.

---

## 2. Theoretical Foundations: Comparative Analysis

### 2.1 The Four Coffman Conditions
Deadlock can occur if and only if four necessary conditions hold simultaneously:
1. **Mutual Exclusion:** Exclusive unshareable holding of resources.
2. **Hold and Wait:** Holding acquired resources while awaiting new requests.
3. **No Preemption:** Resources cannot be revoked forcibly before voluntary release.
4. **Circular Wait:** Closed directed cyclic dependency chain of waiting processes.

### 2.2 Deep Distinction: Prevention vs. Avoidance vs. Detection vs. Recovery

```
                      DEADLOCK MANAGEMENT STRATEGIES
                                    │
       ┌────────────────────────────┴───────────────────────────┐
       ▼                                                        ▼
STATIC CONSTRAINTS                                     RUNTIME DYNAMICS
(Compile/Protocol Time)                                (Execution Time)
       │                                                        │
┌──────┴────────┐                               ┌───────────────┴───────────────┐
▼               ▼                               ▼                               ▼
PREVENTION      AVOIDANCE                       DETECTION                       RECOVERY
Invalidates     Evaluates requests              Grants requests freely;         Breaks active cycles
$\ge 1$ Coffman against Banker's                periodically checks for         by aborting victims or
Condition       Safe Sequence                   irreducible matrix cycles       preempting leases
```

| Dimension | Prevention | Avoidance | Detection | Recovery |
| :--- | :--- | :--- | :--- | :--- |
| **Philosophical Goal** | Prevent deadlock from ever being physically possible by constraining how requests are made. | Steer the system away from unsafe states dynamically before granting any allocation. | Maximize resource utilization by allocating freely; inspect system state when contention arises. | Restore forward progress after detection confirms that an actual deadlock exists. |
| **When Applied** | Compile time / System startup / Protocol enforcement on every syscall. | Synchronously on every individual resource request. | Periodically or upon resource exhaustion / thread block. | Reactively, post-detection. |
| **Required Information** | Static resource ordering rank $F(R)$ or upfront declaration of all requirements. | **A priori knowledge** of maximum lifetime claim vector $\mathbf{Max}[i]$ for every process. | Current $\mathbf{Allocation}$ matrix, current $\mathbf{Request}$ matrix, and $\mathbf{Available}$ vector. | Process priorities, accumulated CPU time, held resource count, and rollback cost. |
| **Computational Complexity** | $O(1)$ per request (simple order comparison or held-count check). | $O(m \cdot n^2)$ safety algorithm on every grant. | $O(m \cdot n^2)$ matrix reduction performed only during detection sweep. | $O(n \log n)$ heuristic victim ranking + iterative re-detection. |
| **Key Limitation** | Drastically reduced resource utilization and potential starvation. | Difficult to know maximum resource demand in advance; runtime overhead. | Deadlock actually occurs; threads are blocked until detection runs. | Lost computation from terminated processes; rollback and cascading abort overhead. |

### 2.3 The "Available $\ne$ Safe" Theorem
- Proof that satisfying physical availability ($\mathbf{Request} \le \mathbf{Available}$) is a necessary condition for allocation, but **not a sufficient condition** for system safety.
- State space Venn diagram: $\text{Safe States} \subset \text{All States}$, $\text{Deadlocked States} \subset \text{Unsafe States}$.

---

## 3. Mathematical Models & Algorithmic Formulations

### 3.1 State Matrices & Verified Invariants
For a system with $n$ processes ($P_0 \dots P_{n-1}$) and $m$ resource types ($R_0 \dots R_{m-1}$):
- Physical Capacity: $\mathbf{Capacity} \in \mathbb{N}^m$
- Available Pool: $\mathbf{Available} \in \mathbb{N}_0^m$
- Allocation Matrix: $\mathbf{Allocation} \in \mathbb{N}_0^{n \times m}$
- Outstanding Request Matrix: $\mathbf{Request} \in \mathbb{N}_0^{n \times m}$
- Maximum Demand: $\mathbf{Max} \in \mathbb{N}_0^{n \times m}$
- Remaining Need: $\mathbf{Need} \in \mathbb{N}_0^{n \times m}$ where $\text{Need}[i][j] = \text{Max}[i][j] - \text{Alloc}[i][j] \ge 0$

**Conservation Invariant:**
$$\forall j \in [0, m-1]: \quad \text{Available}[j] + \sum_{i=0}^{n-1} \text{Allocation}[i][j] = \text{Capacity}[j]$$

### 3.2 Havender Circular-Wait Prevention (Total Resource Ordering)
- Define a bijective indexing function $F: R \to \{1, 2, \dots, m\}$.
- **Enforcement Invariant:** A process $P_i$ holding resource instances may request resource class $R_k$ if and only if:
  $$F(R_k) > \max \{ F(R_j) \mid \text{Allocation}[i][j] > 0 \}$$
- **Mathematical Proof of Cycle Impossibility:**
  Suppose a circular wait exists: $P_0 \to P_1 \to \dots \to P_{k-1} \to P_0$.  
  By protocol, $P_0$ waiting for $R_{r_1}$ held by $P_1$ implies $F(R_{r_1}) > F(R_{r_0})$.  
  Extending along the cycle: $F(R_{r_0}) < F(R_{r_1}) < \dots < F(R_{r_0})$, which implies $F(R_{r_0}) < F(R_{r_0})$—a contradiction. Therefore, no cycle can form.

### 3.3 Shoshani-Coffman Multi-Instance Deadlock Detection Algorithm
1. Initialize $\mathbf{Work} \leftarrow \mathbf{Available}$.
2. Initialize $\mathbf{Finish} \in \{\text{true}, \text{false}\}^n$:
   $$\text{Finish}[i] = \begin{cases} \text{true} & \text{if } \mathbf{Allocation}[i] = \vec{0} \\ \text{false} & \text{otherwise} \end{cases}$$
3. Find an index $i$ such that $\text{Finish}[i] == \text{false}$ and $\mathbf{Request}[i] \le \mathbf{Work}$.
4. If such $i$ exists:
   - $\mathbf{Work} \leftarrow \mathbf{Work} + \mathbf{Allocation}[i]$
   - $\text{Finish}[i] \leftarrow \text{true}$
   - Repeat from Step 3.
5. If no such $i$ exists:
   - The set of deadlocked candidate processes is $\mathcal{D} = \{P_i \mid \text{Finish}[i] == \text{false}\}$.
   - If $\mathcal{D} == \emptyset$, the system is in a **Safe State**.

### 3.4 Multi-Criteria Heuristic Victim Cost Function
When $\mathcal{D} \ne \emptyset$, the recovery engine selects a victim minimizing:
$$\text{Cost}(P_i) = \max\left(1, \, \text{BaseCost}_i + (\text{Priority}_i \times 8) - \left(\sum_{j=0}^{m-1} \text{Allocation}[i][j] \times 12\right)\right)$$
- Favors lower priority processes.
- Favors processes holding larger allocations (maximizing reclaimed resources).
- Protects critical jobs through configurable base cost weights.

---

## 4. Software Architecture & Implementation

### 4.1 Modular Decomposition
- **`core::ResourceManager`**: Authoritative store maintaining state matrices and thread states; enforces physical conservation laws.
- **`prevention::PreventionEngine`**: Gatekeeper intercepting claims before allocation queue mutation.
- **`detection::DetectionEngine`**: Pure algorithmic reducer operating on cloned snapshots.
- **`recovery::RecoveryEngine`**: Evaluates victim costs, executes abort transitions, and verifies post-recovery state.
- **`bridge::HttpServer` & `bridge::ApiRouter`**: Native C++ Winsock REST daemon running on port 8080.
- **`bridge::SimulationService`**: Thread-safe facade synchronizing kernel state with client callers.

### 4.2 Web Presentation Tier (Google Stitch UI)
- Pure HTML5, Vanilla CSS3, ES6+ JavaScript.
- 7 Dedicated Functional Screens:
  1. **Dashboard**: High-level telemetry, available vector cards, real-time matrix viewers, and live SVG Bipartite RAG.
  2. **Processes**: Active process registry table, process registration form, claim submission interface, and quick-claim/release controls.
  3. **Resources**: Hardware resource inventory table, multi-instance slot map, and class definition form.
  4. **Prevention**: Havender invariant sandboxes, protocol selectors, interactive syscall trap evaluation, and dynamic ordering hierarchy.
  5. **Detection**: Matrix reduction status card, step-by-step reduction timeline, and finish vector inspection.
  6. **Recovery**: Deadlock alert banner, victim candidate cost table, confirmation modal, and auto-recovery execution.
  7. **Event Log**: Chronological kernel audit log with category/severity filters and telemetry export.

---

## 5. Experimental Verification & Test Results

### 5.1 Test Harness Summary (43/43 Passing)
- **Core Engine Tests (10/10):** Dimensions, immediate allocation, release, conservation laws, out-of-bounds rejection, request queuing, completion reclamation.
- **Prevention Tests (8/8):** Upfront all-at-once grant, incremental hold-and-wait rejection, atomic zero-partial allocation, increasing order grant, decreasing rank rejection, equal rank rejection, dynamic policy switching.
- **Detection Tests (7/7):** Safe state full reduction, 2-process circular wait, Silberschatz multi-instance benchmark ($\mathcal{D} = \{P_1, P_2, P_3, P_4\}$), zero allocation handling, partial deadlocks, corrupted state handling.
- **Recovery Tests (7/7):** Unnecessary recovery check, 2-process recovery, Silberschatz recovery with single termination, invalid victim rejection, post-recovery re-detection, conservation post-recovery, heuristic cost selection.
- **API Tests (11/11):** Snapshot retrieval, scenario loading, process addition, resource configuration, request submission, prevention rejection via HTTP 422, detection execution, victim termination, over-release rejection.

### 5.2 Canonical Demonstration Benchmarks
1. **Scenario A (Standard Safe State):** 3 processes, 3 resources. Reduction sequence $P_1 \to P_0 \to P_2$. Deadlocked set $\mathcal{D} = \emptyset$.
2. **Scenario B (Circular Wait Conflict):** 2 processes, 2 resources. Mutual cyclic lock ($P_0 \leftrightarrow P_1$). Irreducible candidate set $\mathcal{D} = \{P_0, P_1\}$.
3. **Scenario C (Prevention Interception):** Process $P_0$ holding $R_2$ (rank 3) claims $R_0$ (rank 1). Syscall intercepted and rejected with HTTP 422.
4. **Scenario D (Recovery by Termination):** Silberschatz state. Terminating victim $P_1$ releases 2 units of $R_0$, breaking the cycle and resolving the deadlock.

---

## 6. Demonstration Script for College Faculty Evaluation

A step-by-step 5-minute practical demonstration protocol:
1. **Startup (30 seconds):**
   - Execute `start_server.bat` in Windows Command Prompt.
   - Point out that the C++ daemon starts on port 8080 and opens the browser.
   - Highlight the sidebar telemetry badge: `[Live C++ API]` and `[Port 8080 Active]`.
2. **Dashboard & Safe State (1 minute):**
   - Show initial `Standard Safe State (3P, 3R)`.
   - Explain the SVG Resource Allocation Graph: Process circles, Resource boxes with instance dots, green allocation edges.
   - Switch between Allocation, Request, and Need matrix tabs.
3. **Deadlock Injection & Detection (1.5 minutes):**
   - In header dropdown, switch to **Silberschatz Injected Deadlock (5P, 3R)**.
   - Show top pill turning red: `DEADLOCK (4P)`.
   - Click **Run Detection**. Point out the toast: `Deadlock detected in candidate set { P1, P2, P3, P4 }`.
   - Navigate to the **Detection** screen: Walk through the step-by-step reduction timeline showing $P_0$ completing, while $P_1 \dots P_4$ cannot complete.
4. **Heuristic Recovery (1 minute):**
   - Navigate to the **Recovery** screen.
   - Explain the candidate cost ranking table: $P_1$ identified as the lowest-cost victim (Cost = 21).
   - Click **Terminate Process** on $P_1$. Confirm in the modal.
   - Point out that the banner switches to green: `No Deadlock Present`.
   - Navigate to **Processes** screen: Show $P_1$ marked `TERMINATED` with allocations returned to Available.
5. **Prevention Policy Interception (1 minute):**
   - Reset scenario to **Standard Safe State**.
   - Navigate to **Prevention** screen. Toggle **Circular-Wait Prevention** to `ACTIVATED`.
   - Navigate to **Processes** screen. In the claim form, attempt to request $R_0$ (Rank 1) for $P_0$ (holding Rank 3).
   - Point out the instant inline rejection: `Prevention Policy Violation: Circular-Wait violation`.
   - Navigate to **Event Log** to view the audit record.

---

## 7. Known Limitations & Real-World OS Trade-Offs

1. **User-Space vs. Kernel-Space Model:**
   - The simulator runs in user-space. Real OS kernels (Linux, Windows NT) implement deadlocks across spinlocks, mutexes, and I/O drivers rather than centralized matrix allocators.
2. **Absence of Automatic Checkpointing:**
   - Recovery is demonstrated via process termination. True enterprise database recovery (e.g., PostgreSQL, Oracle) uses Write-Ahead Logging (WAL) and checkpoint rollback rather than hard process termination.
3. **Starvation Risks under Strict Prevention:**
   - Total ordering prevents circular wait, but processes requiring high-ordered resources must hold them for longer durations, causing potential starvation of low-priority tasks.
4. **Banker's Algorithm Pragmatism:**
   - In practice, processes rarely know their true maximum resource claims in advance, which is why commodity operating systems rely on detection and OOM (Out-Of-Memory) victim killing rather than Banker's avoidance.

---

## 8. Conclusion

This project successfully fulfills all academic requirements for an Advanced Operating Systems CBP. By marrying theoretical Coffman condition proofs with an authoritative C++ simulation core and an interactive Google Stitch web visualization, the system provides a comprehensive, empirically verified platform for studying concurrency, deadlock prevention, detection, and recovery.
