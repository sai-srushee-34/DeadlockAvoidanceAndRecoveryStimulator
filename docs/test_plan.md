# Deadlock Prevention and Recovery System: Verification & Test Plan

**Project Title:** Deadlock Prevention and Recovery System  
**Course-Based Project (CBP):** Advanced Operating Systems & Systems Architecture  
**Document Version:** 1.0.0 (Phase 1: Test Plan & Algorithm Verification)  
**Author:** Senior OS Systems Engineer & Software Architect  

---

## 1. Overview & Verification Strategy

The purpose of this test plan is to establish a rigorous, reproducible verification harness for the C++ Deadlock Prevention and Recovery simulation engine. Testing is structured across three tiers:

1. **Tier 1: Unit & Invariant Testing:** Verifies vector and matrix math, state transitions, conservation laws, and input validation bounds.
2. **Tier 2: Algorithmic Verification:** Tests the Banker's Safety Algorithm, Banker's Request Evaluation, Shoshani-Coffman Multi-Instance Detection, and Hierarchical Ordering validation against textbook canonical benchmarks.
3. **Tier 3: Scenario & Recovery Integration:** Tests end-to-end multi-step scenarios including deadlock injection, iterative cost-based victim termination, resource preemption with checkpoint rollback, and starvation mitigation.

---

## 2. Canonical Benchmark Test Scenarios

### Scenario 1: Classical 5-Process, 3-Resource Safe State (Silberschatz Benchmark)

#### Objective
Validate that the Avoidance Engine accurately identifies a safe state and computes the expected safe execution sequence.

#### Initial Configuration
- **Resource Types ($m = 3$):**
  - $A: 10$ instances, $B: 5$ instances, $C: 7$ instances.
- **Processes ($n = 5$):** $P_0, P_1, P_2, P_3, P_4$.

$$\mathbf{Allocation} = \begin{pmatrix} 0 & 1 & 0 \\ 2 & 0 & 0 \\ 3 & 0 & 2 \\ 2 & 1 & 1 \\ 0 & 0 & 2 \end{pmatrix}, \quad \mathbf{Max} = \begin{pmatrix} 7 & 5 & 3 \\ 3 & 2 & 2 \\ 9 & 0 & 2 \\ 2 & 2 & 2 \\ 4 & 3 & 3 \end{pmatrix}$$

#### Derived Invariants
- Total Allocated $= [7, 2, 5]$
- $\mathbf{Available} = \mathbf{Capacity} - \sum \mathbf{Allocation} = [10 - 7, 5 - 2, 7 - 5] = [3, 3, 2]$
- $\mathbf{Need} = \mathbf{Max} - \mathbf{Allocation}$:

$$\mathbf{Need} = \begin{pmatrix} 7 & 4 & 3 \\ 1 & 2 & 2 \\ 6 & 0 & 0 \\ 0 & 1 & 1 \\ 4 & 3 & 1 \end{pmatrix}$$

#### Verification Procedure
1. Execute `AvoidanceEngine::evaluateSafety(state)`.
2. Verify return code `isSafe == true`.
3. Verify that the returned safe sequence is valid (e.g., $\langle P_1, P_3, P_4, P_0, P_2 \rangle$ or $\langle P_1, P_3, P_0, P_2, P_4 \rangle$).
4. Step trace:
   - Initial $\text{Work} = [3, 3, 2]$.
   - $P_1$: $\text{Need}_1 = [1, 2, 2] \le [3, 3, 2] \implies \text{Work} \gets [3, 3, 2] + [2, 0, 0] = [5, 3, 2]$.
   - $P_3$: $\text{Need}_3 = [0, 1, 1] \le [5, 3, 2] \implies \text{Work} \gets [5, 3, 2] + [2, 1, 1] = [7, 4, 3]$.
   - $P_4$: $\text{Need}_4 = [4, 3, 1] \le [7, 4, 3] \implies \text{Work} \gets [7, 4, 3] + [0, 0, 2] = [7, 4, 5]$.
   - $P_0$: $\text{Need}_0 = [7, 4, 3] \le [7, 4, 5] \implies \text{Work} \gets [7, 4, 5] + [0, 1, 0] = [7, 5, 5]$.
   - $P_2$: $\text{Need}_2 = [6, 0, 0] \le [7, 5, 5] \implies \text{Work} \gets [7, 5, 5] + [3, 0, 2] = [10, 5, 7]$.
   - All processes finished; system confirmed SAFE.

---

### Scenario 2: Prevention of Unsafe State (Available $\neq$ Safe)

#### Objective
Prove that the simulator rejects an allocation request that is physically available if granting it would leave no safe execution sequence.

#### Test Execution
1. Given the state from Scenario 1 where $\mathbf{Available} = [3, 3, 2]$.
2. Process $P_0$ requests vector $Y = [0, 2, 0]$.
3. Observe:
   - Physical check: $[0, 2, 0] \le \mathbf{Available} [3, 3, 2]$ is **TRUE**.
   - Claim check: $[0, 2, 0] \le \mathbf{Need}_0 [7, 4, 3]$ is **TRUE**.
4. Speculative Allocation:
   - $\text{Available}' = [3, 1, 2]$.
   - $\text{Allocation}_0' = [0, 3, 0]$.
   - $\text{Need}_0' = [7, 2, 3]$.
5. Run Safety Algorithm on tentative state:
   - $P_1$: $\text{Need}_1 = [1, 2, 2] \not\le [3, 1, 2]$ (Need for $B$ is 2, available is 1 $\implies$ FAILS).
   - $P_3$: $\text{Need}_3 = [0, 1, 1] \le [3, 1, 2] \implies \text{Work} \gets [3, 1, 2] + [2, 1, 1] = [5, 2, 3]$.
   - Next eligible?
     - $P_0: [7, 2, 3] \not\le [5, 2, 3]$ (Fails).
     - $P_1: [1, 2, 2] \le [5, 2, 3] \implies \text{Work} \gets [5, 2, 3] + [2, 0, 0] = [7, 2, 3]$.
     - $P_2: [6, 0, 0] \le [7, 2, 3] \implies \text{Work} \gets [7, 2, 3] + [3, 0, 2] = [10, 2, 5]$.
     - $P_4: [4, 3, 1] \not\le [10, 2, 5]$ (Need for $B$ is 3, available is 2 $\implies$ FAILS).
     - $P_0: [7, 2, 3] \not\le [10, 2, 5]$ (Need for $B$ is 2, available is 2, but what if $P_4$ is blocked?).
     - Note: Can $P_0$ finish? $\text{Need}_0 = [7, 2, 3] \le [10, 2, 5] \implies \text{Work} \gets [10, 2, 5] + [0, 3, 0] = [10, 5, 5]$.
     - Then $P_4: [4, 3, 1] \le [10, 5, 5] \implies$ finishes!
   - Now consider when $P_0$ requests $[0, 3, 0]$ or $P_4$ requests $[3, 3, 0]$:
     - Specifically test a request that leads to a definitive UNSAFE state:
     - Let $P_4$ request $[3, 3, 0]$ when $\text{Available} = [3, 3, 2]$.
     - If allocated: $\text{Available}' = [0, 0, 2]$.
     - Check: No process has $\text{Need}_i \le [0, 0, 2]$. Immediate deadlock!
6. Assert: `AvoidanceEngine::evaluateRequest` returns `isSafe == false`. State remains unchanged; $P_4$ transitions to $\text{WAITING}$.

---

### Scenario 3: Shoshani-Coffman Multi-Instance Deadlock Detection

#### Objective
Verify that the multi-instance matrix reduction algorithm accurately identifies a deadlock and outputs the precise set $\mathcal{D}$ of deadlocked processes.

#### Benchmark State
- **Capacity:** $A: 7, B: 2, C: 6$.
- **Allocation:**
  - $P_0: [0, 1, 0]$
  - $P_1: [2, 0, 0]$
  - $P_2: [3, 0, 3]$
  - $P_3: [2, 1, 1]$
  - $P_4: [0, 0, 2]$
- $\sum \text{Allocation} = [7, 2, 6] \implies \mathbf{Available} = [0, 0, 0]$.
- **Request Matrix:**
  - $P_0: [0, 0, 0]$
  - $P_1: [2, 0, 2]$
  - $P_2: [0, 0, 1]$ (Request injected)
  - $P_3: [1, 0, 0]$
  - $P_4: [0, 0, 2]$

#### Reduction Trace
1. $\text{Work} = [0, 0, 0]$.
2. $\text{Finish}$: All false because all processes hold non-zero allocations.
3. Process scan:
   - $P_0$: $\text{Request}_0 = [0, 0, 0] \le [0, 0, 0] \implies \text{Finish}[0] = \text{true}, \text{Work} \gets [0, 1, 0]$.
   - Check others with $\text{Work} = [0, 1, 0]$:
     - $P_1: [2, 0, 2] \not\le [0, 1, 0]$ (Fails).
     - $P_2: [0, 0, 1] \not\le [0, 1, 0]$ (Fails).
     - $P_3: [1, 0, 0] \not\le [0, 1, 0]$ (Fails).
     - $P_4: [0, 0, 2] \not\le [0, 1, 0]$ (Fails).
4. No further process can be reduced!
5. Assert:
   - `DetectionResult::isDeadlocked == true`.
   - `DetectionResult::deadlockedProcessIds == {1, 2, 3, 4}`.
   - `DetectionResult::reducibleProcessIds == {0}`.

---

### Scenario 4: Circular Wait Prevention Enforcement (Hierarchical Ordering)

#### Objective
Verify that the Total Ordering rule prevents cycle formation by rejecting out-of-order resource acquisition.

#### Configuration
- Global resource ranking:
  $$F(R_0) = 1, \quad F(R_1) = 2, \quad F(R_2) = 3$$
- Process $P_1$ currently holds $1$ instance of $R_1$ ($F(R_1) = 2$).
- $P_1$ submits a request for $1$ instance of $R_0$ ($F(R_0) = 1$).

#### Verification Procedure
1. Call `PreventionEngine::validateTotalOrder(state, 1, [1, 0, 0])`.
2. Verify:
   - `result.allowed == false`.
   - `result.violationReason` contains `"Requested resource rank 1 <= highest held resource rank 2"`.
3. Assert that no allocation occurs and invariant state is maintained.

---

### Scenario 5: Hold-and-Wait Prevention (Protocol B: Release First)

#### Objective
Verify that a process holding resources is forbidden from requesting additional resources without first relinquishing its existing allocation.

#### Verification Procedure
1. Configure `AllocationPolicy::PREVENTION_HOLD_AND_WAIT`.
2. Process $P_2$ holds $\text{Allocation}_2 = [0, 1, 0]$.
3. $P_2$ requests $[1, 0, 0]$.
4. Call `PreventionEngine::validateHoldAndWait(state, 2, [1, 0, 0], false)`.
5. Verify:
   - `result.allowed == false`.
   - Violation message: `"Process holds 1 resources; must release all before requesting new resources."`

---

### Scenario 6: Automated Recovery by Heuristic Victim Termination

#### Objective
Verify that upon detection of a deadlock, the recovery engine accurately selects the optimal victim based on the cost function, reclaims its resources, and resolves the deadlock.

#### Cost Parameters
- Priority Weight: $w_1 = 10.0$
- CPU Time Weight: $w_2 = 1.0$
- Resource Holding Weight: $w_3 = 5.0$
- Starvation Aging Weight: $w_4 = 15.0$

$$\text{Cost}(P_i) = 10 \cdot \rho_i + 1 \cdot t_{\text{cpu}} - 5 \cdot \sum \text{Alloc}[i] - 15 \cdot \kappa_i$$

#### Verification Procedure
1. Start with the deadlocked state from Scenario 3 ($\mathcal{D} = \{P_1, P_2, P_3, P_4\}$).
2. Given process attributes:
   - $P_1$: priority $= 5$, cpuTime $= 20$, held $= 2 \implies \text{Cost} = 50 + 20 - 10 = 60$.
   - $P_2$: priority $= 2$, cpuTime $= 5$, held $= 6 \implies \text{Cost} = 20 + 5 - 30 = -5$.
   - $P_3$: priority $= 7$, cpuTime $= 50$, held $= 4 \implies \text{Cost} = 70 + 50 - 20 = 100$.
   - $P_4$: priority $= 4$, cpuTime $= 10$, held $= 2 \implies \text{Cost} = 40 + 10 - 10 = 40$.
3. Execute `RecoveryEngine::selectBestVictim(state, D)`.
4. Assert: Process $P_2$ is selected as the victim (Lowest cost: $-5$).
5. Execute termination of $P_2$:
   - Reclaim $P_2$'s resources: $\text{Available} \gets [0, 0, 0] + [3, 0, 3] = [3, 0, 3]$.
6. Rerun Detection Algorithm:
   - $\text{Work} = [3, 0, 3]$.
   - $P_1$ requests $[2, 0, 2] \le [3, 0, 3] \implies$ finishes! $\text{Work} \gets [3, 0, 3] + [2, 0, 0] = [5, 0, 3]$.
   - $P_4$ requests $[0, 0, 2] \le [5, 0, 3] \implies$ finishes! $\text{Work} \gets [5, 0, 3] + [0, 0, 2] = [5, 0, 5]$.
   - $P_3$ requests $[1, 0, 0] \le [5, 0, 5] \implies$ finishes!
7. Assert: `isDeadlocked == false`. Single victim termination successfully broke the deadlock!

---

### Scenario 7: Malformed Input & Boundary Fuzzing

#### Test Matrix

| Test ID | Input Condition | Expected Result |
| :--- | :--- | :--- |
| `TC_BOUND_01` | Request with negative numbers (e.g., $[-1, 0, 2]$) | Rejected immediately with `ERR_NEGATIVE_VAL`. |
| `TC_BOUND_02` | Request exceeding process's declared max claim $\text{Need}_i$ | Rejected with `ERR_EXCEEDS_CLAIM`. |
| `TC_BOUND_03` | Release count exceeding process's current allocation | Rejected with `ERR_INVALID_RELEASE`. |
| `TC_BOUND_04` | Non-existent process ID ($p \ge n$ or $p < 0$) | Throws `std::out_of_range` or returns error code. |
| `TC_BOUND_05` | Non-existent resource ID ($r \ge m$ or $r < 0$) | Throws `std::out_of_range` or returns error code. |
| `TC_BOUND_06` | Invariant violation: corrupted sum $\sum \text{Alloc} + \text{Available} \neq \text{Capacity}$ | Assertion failure / `verifyInvariants() == false`. |

---

## 3. Test Automation & Execution Harness

In Phase 2, tests will be integrated using a lightweight, native C++ test harness (e.g., header-only Catch2 or a custom micro-test suite) executed via CTest:

```bash
# Build test suite
cmake -B build -S .
cmake --build build --config Release

# Run complete test suite
ctest --test-dir build --output-on-failure
```
