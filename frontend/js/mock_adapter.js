/**
 * Kernel Systems Deadlock Monitor - Mock Simulation Adapter
 * 
 * ============================================================================
 * [FRONTEND DEMO MOCK ADAPTER - C++ SIMULATION ENGINE BRIDGE PENDING]
 * ============================================================================
 * NOTE FOR ARCHITECTURAL COMPLIANCE:
 * The production C++ core simulation engine (ResourceManager, PreventionEngine,
 * DetectionEngine, RecoveryEngine) resides in `src/` and `include/`.
 * This client-side mock adapter acts as an isolated demonstration boundary
 * to power all UI screens, SVG graphs, and interactions without claiming
 * live C++ RPC connection until Phase 6 backend integration is established.
 * ============================================================================
 */

(function (window) {
  'use strict';

  // Deep clone helper
  function clone(obj) {
    return JSON.parse(JSON.stringify(obj));
  }

  // Pre-configured Demonstration Scenarios
  const SCENARIOS = {
    standard_safe: {
      id: 'standard_safe',
      name: 'Standard Safe State (3P, 3R)',
      description: 'System operating normally with sufficient available headroom. Banker algorithm safe sequence exists: P1 -> P0 -> P2.',
      processes: [
        { id: 'P0', name: 'Worker-Alpha', pid: 1040, state: 'RUNNING', priority: 5, cost: 45 },
        { id: 'P1', name: 'IO-Dispatcher', pid: 1041, state: 'RUNNING', priority: 8, cost: 20 },
        { id: 'P2', name: 'DB-Sync-Worker', pid: 1042, state: 'RUNNING', priority: 3, cost: 80 }
      ],
      resources: [
        { id: 'R0', name: 'CPU Cores', capacity: 10, orderRank: 1, unit: 'Cores' },
        { id: 'R1', name: 'Memory Pages', capacity: 5, orderRank: 2, unit: 'Blocks' },
        { id: 'R2', name: 'NVMe Channels', capacity: 7, orderRank: 3, unit: 'Channels' }
      ],
      // Alloc[P][R]
      allocation: [
        [3, 1, 2], // P0
        [2, 0, 1], // P1
        [3, 2, 2]  // P2
      ],
      // Request[P][R]
      request: [
        [0, 1, 0], // P0
        [1, 0, 1], // P1
        [0, 0, 1]  // P2
      ],
      // MaxDemand[P][R]
      maxDemand: [
        [7, 3, 4], // P0
        [3, 2, 2], // P1
        [9, 4, 5]  // P2
      ]
    },

    silberschatz_deadlock: {
      id: 'silberschatz_deadlock',
      name: 'Silberschatz Injected Deadlock (5P, 3R)',
      description: 'Classic multi-instance deadlock configuration from Silberschatz OS Concept. P0 can finish, but P1, P2, P3, and P4 enter mutual cyclic wait.',
      processes: [
        { id: 'P0', name: 'Init-Daemon', pid: 2000, state: 'RUNNING', priority: 9, cost: 10 },
        { id: 'P1', name: 'Compiler-Backend', pid: 2001, state: 'WAITING', priority: 4, cost: 75 },
        { id: 'P2', name: 'Renderer-Thread', pid: 2002, state: 'WAITING', priority: 3, cost: 60 },
        { id: 'P3', name: 'Telemetry-Sink', pid: 2003, state: 'WAITING', priority: 2, cost: 35 },
        { id: 'P4', name: 'Storage-Manager', pid: 2004, state: 'WAITING', priority: 6, cost: 90 }
      ],
      resources: [
        { id: 'R0', name: 'Compute Units', capacity: 7, orderRank: 1, unit: 'Cores' },
        { id: 'R1', name: 'Tape Buffers', capacity: 2, orderRank: 2, unit: 'Buffers' },
        { id: 'R2', name: 'Print Spoolers', capacity: 6, orderRank: 3, unit: 'Spools' }
      ],
      allocation: [
        [0, 1, 0], // P0 holds 1 R1
        [2, 0, 0], // P1 holds 2 R0
        [3, 0, 3], // P2 holds 3 R0, 3 R2
        [2, 1, 1], // P3 holds 2 R0, 1 R1, 1 R2
        [0, 0, 2]  // P4 holds 2 R2
      ],
      request: [
        [0, 0, 0], // P0 requests nothing (can complete)
        [2, 0, 2], // P1 requests 2 R0, 2 R2
        [0, 0, 1], // P2 requests 1 R2
        [1, 0, 0], // P3 requests 1 R0
        [0, 0, 2]  // P4 requests 2 R2
      ],
      maxDemand: [
        [0, 1, 0],
        [4, 0, 2],
        [3, 0, 4],
        [3, 1, 1],
        [0, 0, 4]
      ]
    },

    circular_wait: {
      id: 'circular_wait',
      name: 'Circular Wait Conflict (2P, 2R)',
      description: 'Strict cycle violating Havender Condition 4: P0 holds R0 and requests R1; P1 holds R1 and requests R0. Both wait perpetually.',
      processes: [
        { id: 'P0', name: 'Crypto-Signer', pid: 3001, state: 'WAITING', priority: 4, cost: 55 },
        { id: 'P1', name: 'Key-Custodian', pid: 3002, state: 'WAITING', priority: 7, cost: 40 }
      ],
      resources: [
        { id: 'R0', name: 'HSM Vault Lock', capacity: 1, orderRank: 1, unit: 'Hardware Token' },
        { id: 'R1', name: 'Entropy Pool Bus', capacity: 1, orderRank: 2, unit: 'Bus Line' }
      ],
      allocation: [
        [1, 0], // P0 holds R0
        [0, 1]  // P1 holds R1
      ],
      request: [
        [0, 1], // P0 requests R1
        [1, 0]  // P1 requests R0
      ],
      maxDemand: [
        [1, 1],
        [1, 1]
      ]
    },

    hold_wait_starvation: {
      id: 'hold_wait_starvation',
      name: 'Hold & Wait Resource Starvation (4P, 3R)',
      description: 'Partial allocations lock out arriving processes, inducing artificial resource exhaustion under Havender Condition 2 violation.',
      processes: [
        { id: 'P0', name: 'Batch-Job-0', pid: 4010, state: 'RUNNING', priority: 2, cost: 30 },
        { id: 'P1', name: 'Batch-Job-1', pid: 4011, state: 'WAITING', priority: 5, cost: 65 },
        { id: 'P2', name: 'Interactive-GUI', pid: 4012, state: 'WAITING', priority: 8, cost: 25 },
        { id: 'P3', name: 'Background-Sync', pid: 4013, state: 'WAITING', priority: 1, cost: 15 }
      ],
      resources: [
        { id: 'R0', name: 'Shared Memory', capacity: 6, orderRank: 1, unit: 'MB' },
        { id: 'R1', name: 'GPU Acceleration', capacity: 2, orderRank: 2, unit: 'Cards' },
        { id: 'R2', name: 'Network Sockets', capacity: 4, orderRank: 3, unit: 'FDs' }
      ],
      allocation: [
        [2, 1, 1], // P0
        [2, 0, 2], // P1
        [1, 1, 0], // P2
        [1, 0, 1]  // P3
      ],
      request: [
        [1, 0, 0], // P0 wants 1 R0
        [0, 1, 1], // P1 wants 1 GPU, 1 Net (GPU busy)
        [2, 0, 1], // P2 wants 2 Mem, 1 Net (Mem busy)
        [1, 1, 0]  // P3 wants 1 Mem, 1 GPU (both busy)
      ],
      maxDemand: [
        [3, 1, 1],
        [2, 1, 3],
        [3, 1, 1],
        [2, 1, 1]
      ]
    },

    custom_manual: {
      id: 'custom_manual',
      name: 'Manual / Custom Sandbox (Blank Canvas)',
      description: 'Blank simulation canvas. Define custom processes, configure resources, allocate instances, or delete components.',
      processes: [],
      resources: [],
      allocation: [],
      request: [],
      maxDemand: []
    }
  };

  class MockSimulationAdapter {
    constructor() {
      this.listeners = [];
      this.eventLog = [];
      this.activeScenarioId = 'standard_safe';
      this.state = null;
      this.detectionReport = null;
      this.preventionSettings = {
        holdWaitPolicy: 'B', // 'A': All-at-once, 'B': Release-before-request
        holdWaitActive: true,
        circularWaitActive: true
      };

      this.loadScenario(this.activeScenarioId, false);
      this._addSystemEvent('SYSTEM', 'Mock Simulation Adapter initialized. C++ engine bridge pending.', 'INFO');
    }

    /**
     * Subscribe to state change notifications
     */
    subscribe(callback) {
      if (typeof callback === 'function') {
        this.listeners.push(callback);
      }
      return () => {
        this.listeners = this.listeners.filter(cb => cb !== callback);
      };
    }

    _notify() {
      this.listeners.forEach(cb => {
        try {
          cb(this.getState());
        } catch (err) {
          console.error('[MockSimulationAdapter] Listener error:', err);
        }
      });
    }

    /**
     * Compute Available vector from Capacity and Allocation Matrix:
     * Available[j] = Capacity[j] - sum_i(Allocation[i][j])
     */
    _recomputeAvailable() {
      const pCount = this.state.processes.length;
      const rCount = this.state.resources.length;
      const avail = new Array(rCount).fill(0);

      for (let j = 0; j < rCount; j++) {
        let totalAllocated = 0;
        for (let i = 0; i < pCount; i++) {
          totalAllocated += (this.state.allocation[i] ? this.state.allocation[i][j] : 0) || 0;
        }
        avail[j] = Math.max(0, this.state.resources[j].capacity - totalAllocated);
      }
      this.state.available = avail;
    }

    /**
     * Compute Need Matrix:
     * Need[i][j] = MaxDemand[i][j] - Allocation[i][j]
     */
    _computeNeedMatrix() {
      const pCount = this.state.processes.length;
      const rCount = this.state.resources.length;
      const need = [];

      for (let i = 0; i < pCount; i++) {
        need[i] = [];
        for (let j = 0; j < rCount; j++) {
          const maxVal = (this.state.maxDemand[i] ? this.state.maxDemand[i][j] : 0) || 0;
          const allocVal = (this.state.allocation[i] ? this.state.allocation[i][j] : 0) || 0;
          need[i][j] = Math.max(0, maxVal - allocVal);
        }
      }
      return need;
    }

    /**
     * Calculate summary metrics (total allocations, requests, contention)
     */
    _computeMetrics() {
      let totalAlloc = 0;
      let totalReq = 0;
      let totalCap = 0;

      this.state.resources.forEach(r => totalCap += r.capacity);

      this.state.allocation.forEach(row => {
        row.forEach(val => totalAlloc += val);
      });

      this.state.request.forEach(row => {
        row.forEach(val => totalReq += val);
      });

      const utilization = totalCap > 0 ? ((totalAlloc / totalCap) * 100).toFixed(1) : '0.0';

      return {
        totalProcesses: this.state.processes.length,
        runningProcesses: this.state.processes.filter(p => p.state === 'RUNNING').length,
        waitingProcesses: this.state.processes.filter(p => p.state === 'WAITING').length,
        deadlockedProcessesCount: this.state.deadlockedProcesses.length,
        terminatedProcesses: this.state.processes.filter(p => p.state === 'TERMINATED').length,
        totalResources: this.state.resources.length,
        totalCapacity: totalCap,
        totalAllocated: totalAlloc,
        totalRequested: totalReq,
        utilizationPct: utilization
      };
    }

    /**
     * Load an isolated demo scenario
     */
    loadScenario(scenarioId, notify = true) {
      const template = SCENARIOS[scenarioId] || SCENARIOS.standard_safe;
      this.activeScenarioId = template.id;

      this.state = {
        scenarioId: template.id,
        scenarioName: template.name,
        description: template.description,
        processes: clone(template.processes),
        resources: clone(template.resources),
        allocation: clone(template.allocation),
        request: clone(template.request),
        maxDemand: clone(template.maxDemand),
        available: [],
        deadlockedProcesses: [],
        isDeadlocked: false
      };

      this._recomputeAvailable();

      // Run baseline detection on loaded state
      this._runInternalDetection();

      this._addSystemEvent(
        'SCENARIO',
        `Loaded scenario "${template.name}". Invariants verified.`,
        'INFO',
        { scenarioId: template.id, processes: this.state.processes.length, resources: this.state.resources.length }
      );

      if (notify) {
        this._notify();
      }
    }

    /**
     * Reset the active scenario to initial definition
     */
    resetScenario() {
      this.loadScenario(this.activeScenarioId, true);
      this._addSystemEvent('SYSTEM', `Reset scenario to initial state: ${this.state.scenarioName}`, 'WARNING');
    }

    /**
     * Shoshani-Coffman Multi-Instance Deadlock Detection Algorithm
     * (Isolated JavaScript implementation matching C++ DetectionEngine Phase 4 specs)
     */
    _runInternalDetection() {
      const pCount = this.state.processes.length;
      const rCount = this.state.resources.length;

      // 1. Initialize Work = Available
      const work = [...this.state.available];

      // 2. Initialize Finish vector
      // If Allocation[i] == 0 then Finish[i] = true, else false
      const finish = new Array(pCount).fill(false);
      for (let i = 0; i < pCount; i++) {
        if (this.state.processes[i].state === 'TERMINATED') {
          finish[i] = true;
          continue;
        }

        let hasAllocation = false;
        for (let j = 0; j < rCount; j++) {
          if (this.state.allocation[i][j] > 0) {
            hasAllocation = true;
            break;
          }
        }
        if (!hasAllocation) {
          finish[i] = true;
        }
      }

      const trace = [];
      let stepNumber = 1;
      let progress = true;

      while (progress) {
        progress = false;

        for (let i = 0; i < pCount; i++) {
          if (!finish[i]) {
            // Check if Request[i] <= Work component-wise
            let canSatisfy = true;
            for (let j = 0; j < rCount; j++) {
              if (this.state.request[i][j] > work[j]) {
                canSatisfy = false;
                break;
              }
            }

            if (canSatisfy) {
              const workBefore = [...work];

              // Simulate release: Work = Work + Allocation[i]
              for (let j = 0; j < rCount; j++) {
                work[j] += this.state.allocation[i][j];
              }

              finish[i] = true;
              progress = true;

              trace.push({
                step: stepNumber++,
                processId: this.state.processes[i].id,
                processName: this.state.processes[i].name,
                requestVector: [...this.state.request[i]],
                workBefore: workBefore,
                workAfter: [...work],
                allocatedReclaimed: [...this.state.allocation[i]],
                status: 'REDUCED'
              });
            }
          }
        }
      }

      // 3. Processes with Finish[i] == false are deadlocked candidates
      const deadlocked = [];
      for (let i = 0; i < pCount; i++) {
        if (!finish[i]) {
          deadlocked.push(this.state.processes[i].id);
        }
      }

      this.state.deadlockedProcesses = deadlocked;
      this.state.isDeadlocked = deadlocked.length > 0;

      // Update process states
      this.state.processes.forEach(p => {
        if (p.state !== 'TERMINATED') {
          if (deadlocked.includes(p.id)) {
            p.state = 'DEADLOCKED';
          } else {
            // Check if waiting for requests
            const pIdx = this._getProcessIndex(p.id);
            const hasRequests = pIdx !== -1 && this.state.request[pIdx].some(v => v > 0);
            p.state = hasRequests ? 'WAITING' : 'RUNNING';
          }
        }
      });

      this.detectionReport = {
        timestamp: new Date().toLocaleTimeString(),
        isDeadlocked: this.state.isDeadlocked,
        deadlockedProcesses: deadlocked,
        trace: trace,
        finalWork: work,
        finishVector: finish
      };

      return this.detectionReport;
    }

    /**
     * Trigger explicit detection run (invoked from UI button)
     */
    runDetection() {
      const report = this._runInternalDetection();
      if (report.isDeadlocked) {
        this._addSystemEvent(
          'DETECTION',
          `Deadlock detected! Unreduced set D = { ${report.deadlockedProcesses.join(', ')} }`,
          'ERROR',
          { deadlocked: report.deadlockedProcesses, stepsReduced: report.trace.length }
        );
      } else {
        this._addSystemEvent(
          'DETECTION',
          'Detection scan complete: System is in a SAFE state. Graph is completely reducible.',
          'SUCCESS',
          { stepsReduced: report.trace.length }
        );
      }
      this._notify();
      return report;
    }

    /**
     * Register a new process in the system
     */
    addProcess(processData) {
      const { id, name, priority, cost, maxDemand, initialAlloc } = processData;

      // Validation
      if (!id || !name) {
        throw new Error('Process ID and Name are required.');
      }
      if (this.state.processes.some(p => p.id.toUpperCase() === id.toUpperCase())) {
        throw new Error(`Process with ID '${id}' already exists.`);
      }

      const rCount = this.state.resources.length;
      const validMax = Array.isArray(maxDemand) ? maxDemand : new Array(rCount).fill(1);
      const validAlloc = Array.isArray(initialAlloc) ? initialAlloc : new Array(rCount).fill(0);

      // Validate capacity constraints
      for (let j = 0; j < rCount; j++) {
        if (validAlloc[j] > this.state.available[j]) {
          throw new Error(`Initial allocation for ${this.state.resources[j].id} (${validAlloc[j]}) exceeds currently available (${this.state.available[j]}).`);
        }
      }

      const newProcess = {
        id: id.toUpperCase(),
        name: name.trim(),
        pid: 1000 + Math.floor(Math.random() * 9000),
        state: 'RUNNING',
        priority: parseInt(priority, 10) || 5,
        cost: parseInt(cost, 10) || 50
      };

      this.state.processes.push(newProcess);
      this.state.allocation.push([...validAlloc]);
      this.state.request.push(new Array(rCount).fill(0));
      this.state.maxDemand.push([...validMax]);

      this._recomputeAvailable();
      this._runInternalDetection();

      this._addSystemEvent(
        'PROCESS',
        `Registered process ${newProcess.id} (${newProcess.name}, PID ${newProcess.pid}).`,
        'INFO',
        { id: newProcess.id, priority: newProcess.priority }
      );

      this._notify();
      return newProcess;
    }

    /**
     * Define a new resource class
     */
    addResource(resourceData) {
      const { id, name, capacity, orderRank, unit } = resourceData;

      if (!id || !name) {
        throw new Error('Resource ID and Name are required.');
      }
      if (this.state.resources.some(r => r.id.toUpperCase() === id.toUpperCase())) {
        throw new Error(`Resource with ID '${id}' already exists.`);
      }

      const cap = parseInt(capacity, 10);
      if (isNaN(cap) || cap <= 0) {
        throw new Error('Capacity must be a positive integer.');
      }

      const rank = parseInt(orderRank, 10) || (this.state.resources.length + 1);

      const newResource = {
        id: id.toUpperCase(),
        name: name.trim(),
        capacity: cap,
        orderRank: rank,
        unit: unit || 'Units'
      };

      this.state.resources.push(newResource);

      // Append column to matrices
      this.state.allocation.forEach(row => row.push(0));
      this.state.request.forEach(row => row.push(0));
      this.state.maxDemand.forEach(row => row.push(1));

      this._recomputeAvailable();
      this._runInternalDetection();

      this._addSystemEvent(
        'RESOURCE',
        `Defined resource class ${newResource.id} ("${newResource.name}", Capacity: ${cap}, Rank: ${rank}).`,
        'INFO',
        { id: newResource.id, capacity: cap }
      );

      this._notify();
      return newResource;
    }

    /**
     * Delete a process
     */
    deleteProcess(processId) {
      const pIdx = this._getProcessIndex(processId);
      if (pIdx === -1) throw new Error(`Unknown process: ${processId}`);

      const proc = this.state.processes[pIdx];
      this.state.processes.splice(pIdx, 1);
      this.state.allocation.splice(pIdx, 1);
      this.state.request.splice(pIdx, 1);
      this.state.maxDemand.splice(pIdx, 1);

      this._recomputeAvailable();
      this._runInternalDetection();

      this._addSystemEvent(
        'PROCESS',
        `Deleted process ${proc.id} (${proc.name}) from kernel table. Held resources reclaimed.`,
        'WARNING',
        { id: proc.id }
      );

      this._notify();
      return this.getState();
    }

    /**
     * Delete a resource class
     */
    deleteResource(resourceId) {
      const rIdx = this._getResourceIndex(resourceId);
      if (rIdx === -1) throw new Error(`Unknown resource: ${resourceId}`);

      const res = this.state.resources[rIdx];
      this.state.resources.splice(rIdx, 1);
      this.state.allocation.forEach(row => row.splice(rIdx, 1));
      this.state.request.forEach(row => row.splice(rIdx, 1));
      this.state.maxDemand.forEach(row => row.splice(rIdx, 1));

      this._recomputeAvailable();
      this._runInternalDetection();

      this._addSystemEvent(
        'RESOURCE',
        `Deleted resource class ${res.id} (${res.name}) from kernel catalog.`,
        'WARNING',
        { id: res.id }
      );

      this._notify();
      return this.getState();
    }

    /**
     * Clear all processes
     */
    clearProcesses() {
      this.state.processes = [];
      this.state.allocation = [];
      this.state.request = [];
      this.state.maxDemand = [];

      this._recomputeAvailable();
      this._runInternalDetection();
      this._addSystemEvent('PROCESS', 'Cleared all registered processes. Held resources returned to Available pool.', 'WARNING');
      this._notify();
      return this.getState();
    }

    /**
     * Clear all resources
     */
    clearResources() {
      this.state.resources = [];
      this.state.available = [];
      this.state.allocation.forEach(row => row.length = 0);
      this.state.request.forEach(row => row.length = 0);
      this.state.maxDemand.forEach(row => row.length = 0);

      this._runInternalDetection();
      this._addSystemEvent('RESOURCE', 'Cleared all resources from catalog.', 'WARNING');
      this._notify();
      return this.getState();
    }


    /**
     * Clear all processes and resources (Blank Canvas)
     */
    clearAll() {
      this.state.processes = [];
      this.state.resources = [];
      this.state.allocation = [];
      this.state.request = [];
      this.state.maxDemand = [];
      this.state.available = [];
      this.state.scenarioId = 'custom_manual';
      this.state.scenarioName = 'Manual / Custom Sandbox (Blank Canvas)';

      this._runInternalDetection();
      this._addSystemEvent(
        'SYSTEM',
        'Cleared all processes and resources. Blank custom sandbox initialized.',
        'WARNING'
      );

      this._notify();
      return this.getState();
    }

    /**
     * Submit a resource request
     */
    submitRequest(processId, resourceId, requestedUnits) {
      const pIdx = this._getProcessIndex(processId);
      const rIdx = this._getResourceIndex(resourceId);

      if (pIdx === -1) throw new Error(`Unknown process: ${processId}`);
      if (rIdx === -1) throw new Error(`Unknown resource: ${resourceId}`);

      const units = parseInt(requestedUnits, 10);
      if (isNaN(units) || units <= 0) {
        throw new Error('Requested units must be greater than zero.');
      }

      const res = this.state.resources[rIdx];
      const proc = this.state.processes[pIdx];

      // Check Prevention Policies if active
      if (this.preventionSettings.circularWaitActive) {
        // Find highest held resource rank
        let maxHeldRank = 0;
        for (let j = 0; j < this.state.resources.length; j++) {
          if (this.state.allocation[pIdx][j] > 0) {
            maxHeldRank = Math.max(maxHeldRank, this.state.resources[j].orderRank);
          }
        }
        if (maxHeldRank > 0 && res.orderRank <= maxHeldRank) {
          this._addSystemEvent(
            'PREVENTION',
            `Request BLOCKED by Circular-Wait policy: ${proc.id} holds rank ${maxHeldRank}, attempted claim on ${res.id} (rank ${res.orderRank}).`,
            'WARNING',
            { process: proc.id, requestedRank: res.orderRank, heldRank: maxHeldRank }
          );
          throw new Error(`Circular-Wait Prevention: Order violation! F(${res.id})=${res.orderRank} <= F(Held_Max)=${maxHeldRank}. Strict monotonic ordering required.`);
        }
      }

      if (this.preventionSettings.holdWaitActive && this.preventionSettings.holdWaitPolicy === 'B') {
        const totalHeld = this.state.allocation[pIdx].reduce((a, b) => a + b, 0);
        if (totalHeld > 0) {
          this._addSystemEvent(
            'PREVENTION',
            `Request REJECTED by Hold-and-Wait policy (Protocol B): ${proc.id} holds ${totalHeld} units. Must release all before requesting.`,
            'WARNING',
            { process: proc.id, heldUnits: totalHeld }
          );
          throw new Error(`Hold-and-Wait Prevention (Protocol B): Process ${proc.id} holds active allocations. You must release all resources before requesting new ones.`);
        }
      }

      // Check if immediate allocation is possible
      if (units <= this.state.available[rIdx]) {
        // Immediate Grant
        this.state.available[rIdx] -= units;
        this.state.allocation[pIdx][rIdx] += units;
        this.state.request[pIdx][rIdx] = 0;
        proc.state = 'RUNNING';

        this._addSystemEvent(
          'ALLOCATION',
          `Granted ${units}x ${res.name} (${res.id}) to ${proc.id}. Remaining available: ${this.state.available[rIdx]}.`,
          'SUCCESS',
          { process: proc.id, resource: res.id, granted: units }
        );
      } else {
        // Must Wait
        this.state.request[pIdx][rIdx] += units;
        proc.state = 'WAITING';

        this._addSystemEvent(
          'REQUEST',
          `Process ${proc.id} requested ${units}x ${res.id}. Insufficient units available (${this.state.available[rIdx]}). Added to request edge.`,
          'WARNING',
          { process: proc.id, resource: res.id, requested: units, available: this.state.available[rIdx] }
        );
      }

      this._runInternalDetection();
      this._notify();
    }

    /**
     * Terminate victim process for deadlock recovery
     */
    recoverVictim(processId, strategyName = 'MANUAL') {
      const pIdx = this._getProcessIndex(processId);
      if (pIdx === -1) {
        throw new Error(`Process ${processId} not found.`);
      }

      const proc = this.state.processes[pIdx];
      const reclaimed = {};
      let totalUnitsReclaimed = 0;

      // Reclaim all held allocations
      for (let j = 0; j < this.state.resources.length; j++) {
        const held = this.state.allocation[pIdx][j];
        if (held > 0) {
          const rId = this.state.resources[j].id;
          reclaimed[rId] = held;
          totalUnitsReclaimed += held;
          this.state.allocation[pIdx][j] = 0;
        }
        // Clear outstanding requests
        this.state.request[pIdx][j] = 0;
      }

      proc.state = 'TERMINATED';
      this._recomputeAvailable();

      // Re-run detection post-termination
      const report = this._runInternalDetection();

      const recoveryOutcome = {
        victimId: proc.id,
        victimName: proc.name,
        strategy: strategyName,
        reclaimedResources: reclaimed,
        totalUnitsReclaimed: totalUnitsReclaimed,
        remainingDeadlocked: report.deadlockedProcesses,
        deadlockResolved: !report.isDeadlocked,
        timestamp: new Date().toLocaleTimeString()
      };

      this._addSystemEvent(
        'RECOVERY',
        `Terminated victim ${proc.id} (${proc.name}) via ${strategyName}. Reclaimed ${totalUnitsReclaimed} units. Deadlock resolved: ${recoveryOutcome.deadlockResolved ? 'YES' : 'NO'}.`,
        recoveryOutcome.deadlockResolved ? 'SUCCESS' : 'WARNING',
        recoveryOutcome
      );

      this._notify();
      return recoveryOutcome;
    }

    /**
     * Heuristic calculation of victim candidates
     */
    getVictimCandidates() {
      const deadlockedIds = this.state.deadlockedProcesses;
      if (!deadlockedIds.length) return [];

      return deadlockedIds.map(id => {
        const pIdx = this._getProcessIndex(id);
        const proc = this.state.processes[pIdx];
        const heldTotal = this.state.allocation[pIdx].reduce((a, b) => a + b, 0);
        const reqTotal = this.state.request[pIdx].reduce((a, b) => a + b, 0);

        // Heuristic Cost formula: TerminationCost = BaseCost * PriorityWeight - (HeldResources * 10)
        // Lower cost means more attractive victim to terminate
        const calculatedCost = Math.max(1, (proc.cost || 50) + (proc.priority * 8) - (heldTotal * 12));

        return {
          id: proc.id,
          name: proc.name,
          pid: proc.pid,
          priority: proc.priority,
          baseCost: proc.cost,
          heldResourcesTotal: heldTotal,
          requestedTotal: reqTotal,
          calculatedCost: calculatedCost
        };
      }).sort((a, b) => a.calculatedCost - b.calculatedCost);
    }

    _getProcessIndex(id) {
      return this.state.processes.findIndex(p => p.id.toUpperCase() === id.toUpperCase());
    }

    _getResourceIndex(id) {
      return this.state.resources.findIndex(r => r.id.toUpperCase() === id.toUpperCase());
    }

    _addSystemEvent(category, message, severity = 'INFO', metadata = null) {
      const now = new Date();
      const timeStr = now.toTimeString().split(' ')[0] + '.' + String(now.getMilliseconds()).padStart(3, '0');

      const entry = {
        id: 'evt_' + Math.random().toString(36).substr(2, 9),
        timestamp: timeStr,
        category: category, // 'SYSTEM', 'SCENARIO', 'DETECTION', 'RECOVERY', 'PREVENTION', 'ALLOCATION', 'PROCESS', 'RESOURCE'
        message: message,
        severity: severity, // 'INFO', 'SUCCESS', 'WARNING', 'ERROR'
        metadata: metadata
      };

      this.eventLog.unshift(entry);
      if (this.eventLog.length > 250) {
        this.eventLog.pop();
      }
    }

    getEventLog() {
      return clone(this.eventLog);
    }

    clearEventLog() {
      this.eventLog = [];
      this._addSystemEvent('SYSTEM', 'Audit event log cleared by operator.', 'INFO');
      this._notify();
    }

    /**
     * Return public snapshot of system state
     */
    getState() {
      return {
        ...clone(this.state),
        need: this._computeNeedMatrix(),
        metrics: this._computeMetrics(),
        detectionReport: clone(this.detectionReport),
        preventionSettings: clone(this.preventionSettings),
        availableScenarios: Object.keys(SCENARIOS).map(k => ({
          id: SCENARIOS[k].id,
          name: SCENARIOS[k].name,
          description: SCENARIOS[k].description
        }))
      };
    }
  }

  // Export singleton to window
  window.MockSimulationAdapter = new MockSimulationAdapter();

})(window);
