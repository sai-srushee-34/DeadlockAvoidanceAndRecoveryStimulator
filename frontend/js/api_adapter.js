/**
 * Kernel Systems Deadlock Monitor - Live C++ API Simulation Adapter (Phase 7)
 * 
 * ============================================================================
 * [AUTHORITATIVE C++ SIMULATION ENGINE BRIDGE]
 * ============================================================================
 * Communicates with the live C++ HTTP server (deadlock_server.exe) over local REST API.
 * All state mutations, prevention validation, deadlock detection (Shoshani-Coffman),
 * and victim recovery are executed authoritatively inside the C++ kernel core.
 * ============================================================================
 */

(function (window) {
  'use strict';

  function clone(obj) {
    if (!obj) return obj;
    return JSON.parse(JSON.stringify(obj));
  }

  class LiveSimulationAdapter {
    constructor() {
      // API Base URL resolution:
      // If served directly from deadlock_server.exe on port 8080, use same-origin relative paths.
      // Otherwise default to http://localhost:8080 with CORS support.
      this.baseUrl = window.DEADLOCK_API_URL || 
        (window.location.protocol.startsWith('http') && window.location.port === '8080' ? '' : 'http://localhost:8080');

      this.listeners = [];
      this.connected = false;
      this.isSyncing = false;
      this.lastSync = null;

      // Authoritative State Cache
      this.state = {
        scenarioId: 'standard_safe',
        scenarioName: 'Standard Safe State (3P, 3R)',
        description: 'Connecting to live C++ kernel simulation server...',
        step: 0,
        policy: 'None (Unconstrained)',
        policyCode: 'NONE',
        preventionSettings: {
          policy: 'B',
          holdWaitActive: false,
          circularWaitActive: false
        },
        processes: [],
        resources: [],
        available: [],
        allocation: [],
        request: [],
        maxDemand: [],
        need: [],
        metrics: {
          totalProcesses: 0,
          runningProcesses: 0,
          waitingProcesses: 0,
          deadlockedProcessesCount: 0,
          terminatedProcesses: 0,
          totalResources: 0,
          totalCapacity: 0,
          totalAllocated: 0,
          totalRequested: 0,
          utilizationPct: '0.0'
        },
        isDeadlocked: false,
        deadlockedProcesses: [],
        availableScenarios: [
          { id: 'standard_safe', name: 'Standard Safe State (3P, 3R)', description: 'System operating normally with sufficient available headroom.' },
          { id: 'silberschatz_deadlock', name: 'Silberschatz Injected Deadlock (5P, 3R)', description: 'Classic multi-instance deadlock configuration from Silberschatz OS Concept.' },
          { id: 'circular_wait', name: 'Circular Wait Conflict (2P, 2R)', description: 'Strict cycle violating Havender Condition 4.' },
          { id: 'hold_wait_starvation', name: 'Hold-and-Wait Starvation (4P, 3R)', description: 'Processes claim resources incrementally while holding leases.' }
        ]
      };

      this.eventLog = [];
      this.detectionReport = null;

      // Reactive Prevention Proxy
      const self = this;
      this.preventionSettings = {
        get holdWaitActive() {
          return self.state.preventionSettings ? self.state.preventionSettings.holdWaitActive : false;
        },
        set holdWaitActive(val) {
          self.setPreventionPolicy(val ? 'HOLD_AND_WAIT' : 'NONE');
        },
        get circularWaitActive() {
          return self.state.preventionSettings ? self.state.preventionSettings.circularWaitActive : false;
        },
        set circularWaitActive(val) {
          self.setPreventionPolicy(val ? 'CIRCULAR_WAIT' : 'NONE');
        }
      };

      // Periodic synchronization & telemetry heartbeat (every 3 seconds)
      this._startHeartbeat();
      // Initial state sync
      this.syncState();
    }

    _startHeartbeat() {
      setInterval(() => {
        // Only run background sync if not currently executing an active user action
        if (!this.isSyncing) {
          this.syncState(true);
        }
      }, 3000);
    }

    /**
     * Subscribe a listener callback to state updates
     */
    subscribe(callback) {
      if (typeof callback === 'function') {
        this.listeners.push(callback);
        // Immediately invoke with current state snapshot
        try {
          callback(this.getState());
        } catch (err) {
          console.error('[SimulationAdapter] Subscriber error on init:', err);
        }
      }
    }

    _notify() {
      const snapshot = this.getState();
      this.listeners.forEach(fn => {
        try {
          fn(snapshot);
        } catch (err) {
          console.error('[SimulationAdapter] Listener error during dispatch:', err);
        }
      });
      this.updateTelemetryBadge();
    }

    /**
     * Update sidebar badge to show connection status
     */
    updateTelemetryBadge() {
      const adapterBadge = document.getElementById('sidebar-adapter-badge');
      const bridgeBadge = document.getElementById('sidebar-bridge-badge');

      if (adapterBadge) {
        if (this.connected) {
          adapterBadge.className = 'font-label-xs text-label-xs text-primary font-semibold font-mono flex items-center gap-1';
          adapterBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary animate-pulse"></span> Live C++ API';
        } else {
          adapterBadge.className = 'font-label-xs text-label-xs text-outline font-mono';
          adapterBadge.textContent = 'Disconnected';
        }
      }

      if (bridgeBadge) {
        if (this.connected) {
          bridgeBadge.className = 'font-label-xs text-label-xs text-tertiary font-semibold font-mono';
          bridgeBadge.textContent = 'Port 8080 Active';
        } else {
          bridgeBadge.className = 'font-label-xs text-label-xs text-error font-semibold font-mono';
          bridgeBadge.textContent = 'Offline (Launch server)';
        }
      }
    }

    /**
     * Internal HTTP request helper
     */
    async _request(endpoint, options = {}) {
      const url = `${this.baseUrl}${endpoint}`;
      const defaultHeaders = {
        'Accept': 'application/json'
      };

      if (options.body && typeof options.body === 'object') {
        options.body = JSON.stringify(options.body);
        defaultHeaders['Content-Type'] = 'application/json';
      }

      options.headers = { ...defaultHeaders, ...(options.headers || {}) };

      try {
        const response = await fetch(url, options);
        const text = await response.text();
        let data = null;
        try {
          data = text ? JSON.parse(text) : null;
        } catch (parseErr) {
          data = { error: text || 'Invalid JSON response from server' };
        }

        if (!response.ok) {
          const errMsg = (data && (data.error || data.message)) || `HTTP ${response.status}: ${response.statusText}`;
          const err = new Error(errMsg);
          err.status = response.status;
          err.data = data;
          throw err;
        }

        if (!this.connected) {
          this.connected = true;
          this.updateTelemetryBadge();
        }

        return data;
      } catch (err) {
        if (err.name === 'TypeError' || err.message.includes('Failed to fetch') || err.message.includes('NetworkError')) {
          this.connected = false;
          this.updateTelemetryBadge();
          throw new Error(`Cannot connect to C++ simulation server at ${this.baseUrl}. Is deadlock_server.exe running?`);
        }
        throw err;
      }
    }

    /**
     * 1. GET /api/state - Sync authoritative system state
     */
    async syncState(isBackground = false) {
      if (this.isSyncing) return this.state;
      this.isSyncing = true;

      try {
        const data = await this._request('/api/state', { method: 'GET' });
        if (data) {
          this.state = data;
          if (data.detectionReport) {
            this.detectionReport = data.detectionReport;
          }
          this.lastSync = new Date();
          this.connected = true;

          // Also refresh events asynchronously
          this.fetchEvents().catch(() => {});

          if (!isBackground) {
            this._notify();
          } else {
            // Update UI gently in background
            this._notify();
          }
        }
      } catch (err) {
        if (!isBackground) {
          console.warn('[SimulationAdapter] State sync failed:', err.message);
        }
        this.connected = false;
        this.updateTelemetryBadge();
      } finally {
        this.isSyncing = false;
      }

      return this.getState();
    }

    /**
     * Synchronous snapshot accessor (returns current authoritative state cache)
     */
    getState() {
      return {
        ...clone(this.state),
        detectionReport: clone(this.detectionReport || this.state.detectionReport),
        preventionSettings: clone(this.state.preventionSettings || {
          policy: 'B',
          holdWaitActive: false,
          circularWaitActive: false
        })
      };
    }

    /**
     * 2. POST /api/processes - Add a new process
     */
    async addProcess(procData) {
      const payload = {
        name: procData.name || `Process-${procData.id || Date.now()}`,
        priority: parseInt(procData.priority, 10) || 5,
        cost: parseInt(procData.cost, 10) || 50,
        maxDemand: procData.maxDemand || [],
        initialAlloc: procData.initialAlloc || []
      };

      const res = await this._request('/api/processes', {
        method: 'POST',
        body: payload
      });

      if (res && res.processes) {
        this.state = res;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 3. POST /api/resources - Add a new resource class
     */
    async addResource(resData) {
      const payload = {
        name: resData.name || `Resource-${resData.id || Date.now()}`,
        capacity: parseInt(resData.capacity, 10) || 1,
        orderRank: parseInt(resData.orderRank, 10) || 1
      };

      const res = await this._request('/api/resources', {
        method: 'POST',
        body: payload
      });

      if (res && res.resources) {
        this.state = res;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 4. POST /api/requests - Submit a resource request
     */
    async submitRequest(processId, resourceId, units) {
      const payload = {
        processId: processId,
        resourceId: resourceId,
        units: parseInt(units, 10) || 1
      };

      const res = await this._request('/api/requests', {
        method: 'POST',
        body: payload
      });

      if (res && res.processes) {
        this.state = res;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 5. POST /api/release - Release held resource units
     */
    async releaseResources(processId, resourceId, units) {
      const payload = {
        processId: processId,
        resourceId: resourceId,
        units: parseInt(units, 10) || 1
      };

      const res = await this._request('/api/release', {
        method: 'POST',
        body: payload
      });

      if (res && res.processes) {
        this.state = res;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 6. POST /api/prevention/policy - Configure active deadlock prevention policy
     */
    async setPreventionPolicy(policyStr) {
      let policy = policyStr;
      if (policy === 'HOLD_AND_WAIT') policy = 'HOLD_AND_WAIT_CONSERVATIVE';
      if (policy === 'CIRCULAR_WAIT') policy = 'CIRCULAR_WAIT_TOTAL_ORDER';

      await this._request('/api/prevention/policy', {
        method: 'POST',
        body: { policy: policy }
      });

      // Refetch full state to update policyCode and preventionSettings
      await this.syncState();
      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 7. POST /api/detection/run - Execute multi-instance deadlock detection
     */
    async runDetection() {
      const report = await this._request('/api/detection/run', {
        method: 'POST'
      });

      this.detectionReport = report;
      this.state.isDeadlocked = report.isDeadlocked;
      this.state.deadlockedProcesses = report.deadlockedProcesses || [];
      this.state.detectionReport = report;

      // Update process states in cache
      if (this.state.processes) {
        this.state.processes.forEach(p => {
          if (p.state !== 'TERMINATED') {
            if (report.deadlockedProcesses && report.deadlockedProcesses.includes(p.id)) {
              p.state = 'DEADLOCKED';
            } else if (p.state === 'DEADLOCKED') {
              p.state = 'WAITING';
            }
          }
        });
      }

      this._notify();
      await this.fetchEvents();
      return report;
    }

    /**
     * 8. POST /api/recovery/terminate - Terminate selected process to resolve deadlock
     */
    async recoverVictim(victimId, strategy = 'MANUAL') {
      const payload = {
        victimProcessId: victimId,
        policy: strategy
      };

      const outcome = await this._request('/api/recovery/terminate', {
        method: 'POST',
        body: payload
      });

      // State is updated by recovery execution
      await this.syncState();
      await this.fetchEvents();
      return outcome;
    }

    /**
     * 8b. Heuristic Victim Candidates
     */
    getVictimCandidates() {
      const deadlockedIds = this.state.deadlockedProcesses;
      if (!deadlockedIds || !deadlockedIds.length) return [];

      return deadlockedIds.map(id => {
        const pIdx = this.state.processes.findIndex(p => p.id.toUpperCase() === id.toUpperCase());
        if (pIdx === -1) return null;
        const proc = this.state.processes[pIdx];
        const heldTotal = (this.state.allocation[pIdx] || []).reduce((a, b) => a + b, 0);
        const reqTotal = (this.state.request[pIdx] || []).reduce((a, b) => a + b, 0);

        // Standard termination cost calculation matching C++ engine:
        // BaseCost + Priority * 8 - Held * 12
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
      }).filter(Boolean).sort((a, b) => a.calculatedCost - b.calculatedCost);
    }

    async fetchVictimCandidates() {
      try {
        const candidates = await this._request('/api/recovery/candidates', { method: 'GET' });
        return candidates || this.getVictimCandidates();
      } catch (err) {
        return this.getVictimCandidates();
      }
    }

    /**
     * 9. GET /api/events - Retrieve simulation audit event history
     */
    async fetchEvents() {
      try {
        const events = await this._request('/api/events', { method: 'GET' });
        if (Array.isArray(events)) {
          this.eventLog = events;
        }
      } catch (err) {
        // Silent fail for event polling
      }
      return this.eventLog;
    }

    getEventLog() {
      return clone(this.eventLog);
    }

    /**
     * 9b. POST /api/events/clear - Clear audit event log
     */
    async clearEventLog() {
      await this._request('/api/events/clear', { method: 'POST' });
      this.eventLog = [];
      this._notify();
      return true;
    }

    /**
     * 10. POST /api/scenario/load - Load predefined scenario
     */
    async loadScenario(scenarioId) {
      const res = await this._request('/api/scenario/load', {
        method: 'POST',
        body: { scenarioId: scenarioId }
      });

      if (res && res.processes) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 10b. POST /api/scenario/reset - Reset current scenario to initial baseline
     */
    async resetScenario() {
      const res = await this._request('/api/scenario/reset', {
        method: 'POST'
      });

      if (res && res.processes) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      await this.fetchEvents();
      return this.getState();
    }

    /**
     * 10c. POST /api/processes/delete - Delete process entirely
     */
    async deleteProcess(processId) {
      const res = await this._request('/api/processes/delete', {
        method: 'POST',
        body: { id: processId }
      });

      if (res && Array.isArray(res.processes)) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      this.fetchEvents().catch(() => {});
      return this.getState();
    }

    /**
     * 10c2. POST /api/processes/clear - Clear all registered processes
     */
    async clearProcesses() {
      const res = await this._request('/api/processes/clear', {
        method: 'POST'
      });

      if (res && Array.isArray(res.processes)) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      this.fetchEvents().catch(() => {});
      return this.getState();
    }

    /**
     * 10d. POST /api/resources/delete - Delete resource class entirely
     */
    async deleteResource(resourceId) {
      const res = await this._request('/api/resources/delete', {
        method: 'POST',
        body: { id: resourceId }
      });

      if (res && Array.isArray(res.processes)) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      this.fetchEvents().catch(() => {});
      return this.getState();
    }

    /**
     * 10d2. POST /api/resources/clear - Clear all resource classes
     */
    async clearResources() {
      const res = await this._request('/api/resources/clear', {
        method: 'POST'
      });

      if (res && Array.isArray(res.processes)) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      this.fetchEvents().catch(() => {});
      return this.getState();
    }

    /**
     * 10e. POST /api/simulation/clear - Clear all processes and resources (Blank Canvas)
     */
    async clearAll() {
      const res = await this._request('/api/simulation/clear', {
        method: 'POST'
      });

      if (res && Array.isArray(res.processes)) {
        this.state = res;
        this.detectionReport = res.detectionReport || null;
        this._notify();
      } else {
        await this.syncState();
      }

      this.fetchEvents().catch(() => {});
      return this.getState();
    }
  }

  // Export singleton to window as both LiveSimulationAdapter and MockSimulationAdapter (for seamless drop-in compatibility)
  const adapterInstance = new LiveSimulationAdapter();
  window.LiveSimulationAdapter = adapterInstance;
  window.SimulationAdapter = adapterInstance;
  window.MockSimulationAdapter = adapterInstance;

})(window);
