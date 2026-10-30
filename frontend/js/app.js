/**
 * Kernel Systems Deadlock Monitor - Core Application Orchestrator
 */

(function (window) {
  'use strict';

  class App {
    constructor() {
      this.currentScreen = 'dashboard';
      this.modalVictimId = null;
      this.screens = {
        'dashboard': window.DashboardScreen,
        'processes': window.ProcessesScreen,
        'resources': window.ResourcesScreen,
        'prevention': window.PreventionScreen,
        'detection': window.DetectionScreen,
        'recovery': window.RecoveryScreen,
        'event-log': window.EventLogScreen
      };

      this._init();
    }

    _init() {
      document.addEventListener('DOMContentLoaded', () => {
        this._bindNavigation();
        this._bindHeaderControls();
        this._bindModals();

        // Subscribe to MockSimulationAdapter
        window.MockSimulationAdapter.subscribe((state) => this._onStateChange(state));

        // Initial render
        const initialState = window.MockSimulationAdapter.getState();
        this._populateScenarioDropdown(initialState);
        this.navigateTo('dashboard');
        this._onStateChange(initialState);
      });
    }

    _bindNavigation() {
      const navLinks = document.querySelectorAll('aside nav a[data-screen]');
      navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
          e.preventDefault();
          const screenId = link.getAttribute('data-screen');
          this.navigateTo(screenId);
        });
      });
    }

    navigateTo(screenId) {
      if (!this.screens[screenId]) screenId = 'dashboard';
      this.currentScreen = screenId;

      // Update Screen Containers
      document.querySelectorAll('.screen-view').forEach(el => {
        el.classList.remove('active');
      });
      const target = document.getElementById(`screen-${screenId}`);
      if (target) {
        target.classList.add('active');
      }

      // Update Nav Link Active Classes
      const navLinks = document.querySelectorAll('aside nav a[data-screen]');
      navLinks.forEach(link => {
        const id = link.getAttribute('data-screen');
        if (id === screenId) {
          link.className = 'flex items-center gap-space-sm px-space-md py-space-sm rounded transition-colors bg-surface-container text-primary font-semibold';
          link.setAttribute('aria-current', 'page');
        } else {
          link.className = 'flex items-center gap-space-sm px-space-md py-space-sm rounded text-on-surface-variant font-body-sm text-body-sm hover:bg-surface-container hover:text-on-surface transition-colors';
          link.removeAttribute('aria-current');
        }
      });

      // Update Breadcrumb Title
      const breadcrumbEl = document.getElementById('header-breadcrumb-title');
      if (breadcrumbEl) {
        const titleMap = {
          'dashboard': 'System Overview',
          'processes': 'Process Subsystem',
          'resources': 'Resource Inventory',
          'prevention': 'Deadlock Prevention (Havender)',
          'detection': 'Deadlock Detection (Shoshani-Coffman)',
          'recovery': 'Deadlock Recovery & Mitigation',
          'event-log': 'Kernel Audit Log'
        };
        breadcrumbEl.textContent = titleMap[screenId] || 'System Overview';
      }

      // Trigger screen update
      const state = window.MockSimulationAdapter.getState();
      if (this.screens[screenId] && typeof this.screens[screenId].update === 'function') {
        this.screens[screenId].update(state);
      }
    }

    _bindHeaderControls() {
      // Scenario Select
      const selectScenario = document.getElementById('header-scenario-select');
      if (selectScenario) {
        selectScenario.addEventListener('change', async (e) => {
          try {
            await window.SimulationAdapter.loadScenario(e.target.value);
            this.showToast(`Switched scenario to: ${selectScenario.options[selectScenario.selectedIndex].text}`, 'info');
          } catch (err) {
            this.showToast(`Failed to load scenario: ${err.message}`, 'error');
          }
        });
      }

      // Reset State Button
      const btnReset = document.getElementById('header-btn-reset');
      if (btnReset) {
        btnReset.addEventListener('click', async () => {
          try {
            await window.SimulationAdapter.resetScenario();
            this.showToast('System scenario reset to initial baseline.', 'info');
          } catch (err) {
            this.showToast(`Failed to reset: ${err.message}`, 'error');
          }
        });
      }

      // Run Detection Button
      const btnDetect = document.getElementById('header-btn-detect');
      if (btnDetect) {
        btnDetect.addEventListener('click', async () => {
          try {
            const report = await window.SimulationAdapter.runDetection();
            if (report.isDeadlocked) {
              this.showToast(`Deadlock detected in candidate set { ${report.deadlockedProcesses.join(', ')} }`, 'error');
              this.navigateTo('detection');
            } else {
              this.showToast('Detection complete: All processes successfully reduced. Safe state.', 'success');
            }
          } catch (err) {
            this.showToast(`Detection error: ${err.message}`, 'error');
          }
        });
      }
    }

    _populateScenarioDropdown(state) {
      const select = document.getElementById('header-scenario-select');
      if (!select || !state.availableScenarios || !state.availableScenarios.length) return;

      if (select.options.length !== state.availableScenarios.length) {
        select.innerHTML = state.availableScenarios.map(s => `
          <option value="${s.id}" ${s.id === state.scenarioId ? 'selected' : ''}>${s.name}</option>
        `).join('');
      } else if (select.value !== state.scenarioId) {
        select.value = state.scenarioId;
      }
    }

    _bindModals() {
      const modal = document.getElementById('modal-recovery-confirm');
      const btnCancel = document.getElementById('modal-btn-cancel');
      const btnClose = document.getElementById('modal-btn-close');
      const btnConfirm = document.getElementById('modal-btn-confirm');

      const hide = () => {
        if (modal) modal.classList.add('hidden');
        this.modalVictimId = null;
      };

      if (btnCancel) btnCancel.addEventListener('click', hide);
      if (btnClose) btnClose.addEventListener('click', hide);

      if (btnConfirm) {
        btnConfirm.addEventListener('click', async () => {
          if (!this.modalVictimId) return;
          const victimId = this.modalVictimId;
          hide();

          try {
            const outcome = await window.SimulationAdapter.recoverVictim(victimId, 'MANUAL_OPERATOR');
            if (outcome.deadlockResolved) {
              this.showToast(`Process ${victimId} terminated. All deadlocks resolved!`, 'success');
            } else {
              const remaining = outcome.remainingDeadlocked ? outcome.remainingDeadlocked.join(', ') : 'None';
              this.showToast(`Process ${victimId} terminated. Candidate set remaining: { ${remaining} }`, 'warning');
            }
          } catch (err) {
            this.showToast(`Recovery failed: ${err.message}`, 'error');
          }
        });
      }

      // Close modal on escape
      window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && modal && !modal.classList.contains('hidden')) {
          hide();
        }
      });
    }

    showRecoveryModal(processId) {
      this.modalVictimId = processId;
      const modal = document.getElementById('modal-recovery-confirm');
      const victimText = document.getElementById('modal-victim-id');
      const reclaimText = document.getElementById('modal-reclaim-summary');

      const state = window.MockSimulationAdapter.getState();
      const pIdx = state.processes.findIndex(p => p.id === processId);
      if (pIdx === -1) return;

      const proc = state.processes[pIdx];
      if (victimText) {
        victimText.textContent = `${proc.id} (PID ${proc.pid} • "${proc.name}")`;
      }

      if (reclaimText) {
        const heldList = [];
        state.resources.forEach((r, rIdx) => {
          const count = (state.allocation[pIdx] && state.allocation[pIdx][rIdx]) || 0;
          if (count > 0) heldList.push(`${r.id}: ${count} ${r.unit || 'units'}`);
        });
        reclaimText.textContent = heldList.length ? heldList.join(', ') : 'No active resource leases';
      }

      if (modal) modal.classList.remove('hidden');
    }

    _onStateChange(state) {
      // Update Global State Pill in Header
      const statePill = document.getElementById('header-state-pill');
      if (statePill) {
        if (state.isDeadlocked) {
          statePill.className = 'flex items-center gap-space-xs px-space-sm py-0.5 rounded bg-error-container text-on-error-container font-label-xs text-label-xs font-semibold';
          statePill.innerHTML = `<span class="w-1.5 h-1.5 rounded-full bg-error animate-pulse"></span><span>DEADLOCK (${state.deadlockedProcesses.length}P)</span>`;
        } else {
          statePill.className = 'flex items-center gap-space-xs px-space-sm py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold';
          statePill.innerHTML = `<span class="w-1.5 h-1.5 rounded-full bg-tertiary animate-pulse"></span><span>SAFE STATE</span>`;
        }
      }

      // Ensure Scenario Select options are populated
      this._populateScenarioDropdown(state);

      // Forward to active screen
      if (this.screens[this.currentScreen] && typeof this.screens[this.currentScreen].update === 'function') {
        this.screens[this.currentScreen].update(state);
      }
    }

    showToast(message, type = 'info') {
      const container = document.getElementById('toast-container');
      if (!container) return;

      const toast = document.createElement('div');
      toast.className = `toast-item toast-${type} flex items-center gap-space-sm px-space-md py-space-sm rounded shadow-lg transition-all transform duration-300`;

      let icon = 'info';
      if (type === 'success') icon = 'check_circle';
      if (type === 'error') icon = 'error';
      if (type === 'warning') icon = 'warning';

      toast.innerHTML = `
        <span class="material-symbols-outlined text-[18px]">${icon}</span>
        <span class="font-body-sm font-medium">${message}</span>
      `;

      container.appendChild(toast);

      setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateY(-10px)';
        setTimeout(() => toast.remove(), 300);
      }, 3500);
    }
  }

  window.App = new App();

})(window);
