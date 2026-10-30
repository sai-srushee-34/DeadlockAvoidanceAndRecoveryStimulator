/**
 * Kernel Systems Deadlock Monitor - Processes Screen Controller
 */

(function (window) {
  'use strict';

  class ProcessesScreen {
    constructor() {
      this.initialized = false;
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      // Bind Process Registration Form
      const addForm = document.getElementById('form-add-process');
      if (addForm) {
        addForm.addEventListener('submit', (e) => {
          e.preventDefault();
          this._handleAddProcess();
        });
      }

      // Bind Resource Request Form
      const reqForm = document.getElementById('form-submit-request');
      if (reqForm) {
        reqForm.addEventListener('submit', (e) => {
          e.preventDefault();
          this._handleSubmitRequest();
        });
      }

      // Bind Clear All Processes Button
      const clearBtn = document.getElementById('btn-clear-all-procs');
      if (clearBtn) {
        clearBtn.addEventListener('click', () => {
          this.clearAllProcesses();
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();

      const { processes, resources, allocation, request } = state;

      // Update Process Count Header
      const countEl = document.getElementById('proc-count-summary');
      if (countEl) {
        const running = processes.filter(p => p.state === 'RUNNING').length;
        const waiting = processes.filter(p => p.state === 'WAITING').length;
        const deadlocked = processes.filter(p => p.state === 'DEADLOCKED').length;
        const terminated = processes.filter(p => p.state === 'TERMINATED').length;
        countEl.textContent = `${processes.length} Registered Processes (${running} Running, ${waiting} Waiting, ${deadlocked} Deadlocked, ${terminated} Terminated)`;
      }

      // Populate Process Selectors in Forms
      this._updateProcessDropdowns(processes);
      this._updateResourceDropdowns(resources);

      // Render Dynamic Initial Allocations inputs in Add Process Form
      this._renderInitialAllocInputs(resources, state.available);

      // Render Process Table
      const tableBody = document.getElementById('table-processes-body');
      if (!tableBody) return;

      if (processes.length === 0) {
        tableBody.innerHTML = `<tr><td colspan="7" class="p-space-lg text-center text-outline">No processes currently registered in kernel table.</td></tr>`;
        return;
      }

      tableBody.innerHTML = processes.map((proc, pIdx) => {
        const isDeadlock = proc.state === 'DEADLOCKED';
        const isTerminated = proc.state === 'TERMINATED';
        const isWaiting = proc.state === 'WAITING';

        let badgeClass = 'bg-tertiary-fixed-dim/20 text-tertiary';
        let badgeDot = 'bg-tertiary';
        if (isDeadlock) {
          badgeClass = 'bg-error-container text-on-error-container animate-pulse';
          badgeDot = 'bg-error';
        } else if (isWaiting) {
          badgeClass = 'bg-amber-100 text-amber-800';
          badgeDot = 'bg-amber-500';
        } else if (isTerminated) {
          badgeClass = 'bg-slate-100 text-slate-500';
          badgeDot = 'bg-slate-400';
        }

        const allocVec = allocation[pIdx] ? allocation[pIdx].map((v, i) => `${resources[i].id}:${v}`).join(', ') : 'None';
        const reqVec = request[pIdx] ? request[pIdx].map((v, i) => v > 0 ? `${resources[i].id}:${v}` : null).filter(Boolean).join(', ') : 'None';

        return `
          <tr class="hover:bg-surface-container-lowest transition-colors ${isTerminated ? 'opacity-60 bg-slate-50/50' : ''}">
            <td class="py-space-sm px-space-md font-mono font-semibold text-primary">
              ${proc.id}
              <span class="block font-sans text-[11px] text-outline font-normal">PID: ${proc.pid}</span>
            </td>
            <td class="py-space-sm px-space-md font-body-sm font-medium text-on-surface">
              ${proc.name}
            </td>
            <td class="py-space-sm px-space-md">
              <span class="inline-flex items-center gap-1 px-space-xs py-0.5 rounded font-label-xs text-label-xs font-semibold ${badgeClass}">
                <span class="w-1.5 h-1.5 rounded-full ${badgeDot}"></span>
                ${proc.state}
              </span>
            </td>
            <td class="py-space-sm px-space-md font-mono text-label-xs">
              <span class="px-1.5 py-0.5 bg-surface-container-low rounded font-semibold text-on-surface">Prio ${proc.priority}</span>
              <span class="text-outline text-[10px] ml-1">Cost: ${proc.cost}</span>
            </td>
            <td class="py-space-sm px-space-md font-mono text-label-xs text-tertiary font-semibold">
              [ ${allocVec || '0'} ]
            </td>
            <td class="py-space-sm px-space-md font-mono text-label-xs ${reqVec && reqVec !== 'None' ? 'text-error font-semibold' : 'text-outline'}">
              [ ${reqVec && reqVec !== '' ? reqVec : '∅'} ]
            </td>
            <td class="py-space-sm px-space-md text-right">
              ${!isTerminated ? `
                <button type="button" class="h-6 px-2 text-label-xs font-label-xs rounded bg-surface-container-low text-on-surface hover:bg-surface-container transition-colors mr-1" onclick="window.ProcessesScreen.quickClaim('${proc.id}')">
                  Claim
                </button>
                <button type="button" class="h-6 px-2 text-label-xs font-label-xs rounded bg-surface-container-low text-on-surface hover:bg-surface-container transition-colors mr-1" onclick="window.ProcessesScreen.quickRelease('${proc.id}')">
                  Release
                </button>
                <button type="button" class="h-6 px-2 text-label-xs font-label-xs rounded bg-error-container text-on-error-container hover:bg-error/20 transition-colors mr-1" onclick="window.ProcessesScreen.confirmTerminate('${proc.id}')">
                  Terminate
                </button>
              ` : `
                <span class="font-label-xs text-outline italic mr-1">Terminated</span>
              `}
              <button type="button" class="h-6 px-2 text-label-xs font-label-xs rounded bg-error/15 text-error hover:bg-error hover:text-white transition-colors" title="Delete process from kernel table" onclick="window.ProcessesScreen.deleteProcess('${proc.id}')">
                Delete
              </button>
            </td>
          </tr>
        `;
      }).join('');
    }

    _updateProcessDropdowns(processes) {
      const activeProcesses = (processes || []).filter(p => p.state !== 'TERMINATED');
      const dropdown = document.getElementById('req-target-process');
      if (dropdown) {
        if (activeProcesses.length === 0) {
          const emptyHtml = `<option disabled selected value="">No active processes</option>`;
          if (dropdown.dataset.renderedHtml !== emptyHtml) {
            dropdown.innerHTML = emptyHtml;
            dropdown.dataset.renderedHtml = emptyHtml;
          }
          return;
        }
        const newHtml = activeProcesses.map(p => `
          <option value="${p.id}">${p.id} (${p.name}) • PID ${p.pid}</option>
        `).join('');
        if (dropdown.dataset.renderedHtml !== newHtml) {
          const currentVal = dropdown.value;
          dropdown.innerHTML = newHtml;
          dropdown.dataset.renderedHtml = newHtml;
          if (currentVal && activeProcesses.some(p => p.id === currentVal)) {
            dropdown.value = currentVal;
          }
        }
      }
    }

    _updateResourceDropdowns(resources) {
      const dropdown = document.getElementById('req-target-resource');
      if (dropdown) {
        if (!resources || resources.length === 0) {
          const emptyHtml = `<option disabled selected value="">No resource classes</option>`;
          if (dropdown.dataset.renderedHtml !== emptyHtml) {
            dropdown.innerHTML = emptyHtml;
            dropdown.dataset.renderedHtml = emptyHtml;
          }
          return;
        }
        const newHtml = resources.map(r => `
          <option value="${r.id}">${r.id} (${r.name}) • Rank ${r.orderRank}</option>
        `).join('');
        if (dropdown.dataset.renderedHtml !== newHtml) {
          const currentVal = dropdown.value;
          dropdown.innerHTML = newHtml;
          dropdown.dataset.renderedHtml = newHtml;
          if (currentVal && resources.some(r => r.id === currentVal)) {
            dropdown.value = currentVal;
          }
        }
      }
    }

    _renderInitialAllocInputs(resources, available) {
      const container = document.getElementById('add-proc-alloc-inputs');
      if (!container) return;

      if (!resources || resources.length === 0) {
        container.innerHTML = `<div class="col-span-3 text-outline text-[11px] italic p-1">No resource classes defined yet. Define resources in the Resources tab or create process with 0 allocations.</div>`;
        return;
      }

      // Check if the current inputs in the container already match the resource list
      const existingInputs = container.querySelectorAll('input[name^="alloc_"]');
      const matches = existingInputs.length === resources.length && resources.every(r => {
        return container.querySelector(`input[name="alloc_${r.id}"]`) !== null;
      });

      if (matches) {
        // Update in-place without replacing innerHTML, preserving user inputs and focus
        resources.forEach((r, idx) => {
          const maxAvail = (available && available[idx] !== undefined) ? available[idx] : 0;
          const input = container.querySelector(`input[name="alloc_${r.id}"]`);
          if (input) {
            input.max = maxAvail;
            const label = input.previousElementSibling;
            if (label) {
              label.textContent = `${r.id} (Max Avail: ${maxAvail})`;
            }
          }
        });
        return;
      }

      // If resources changed or initial render, preserve any values currently typed by user
      const savedValues = {};
      existingInputs.forEach(inp => {
        savedValues[inp.name] = inp.value;
      });

      container.innerHTML = resources.map((r, idx) => {
        const maxAvail = (available && available[idx] !== undefined) ? available[idx] : 0;
        const curVal = savedValues[`alloc_${r.id}`] !== undefined ? savedValues[`alloc_${r.id}`] : '0';
        return `
          <div class="flex flex-col gap-0.5">
            <label class="font-label-xs text-[10px] text-outline uppercase font-semibold">${r.id} (Max Avail: ${maxAvail})</label>
            <input type="number" min="0" max="${maxAvail}" value="${curVal}" name="alloc_${r.id}" class="h-7 px-2 bg-surface-container-low text-on-surface font-mono text-label-xs rounded border border-surface-container-high focus:outline-none focus:border-primary">
          </div>
        `;
      }).join('');
    }

    async _handleAddProcess() {
      const idInput = document.getElementById('add-proc-id');
      const nameInput = document.getElementById('add-proc-name');
      const prioInput = document.getElementById('add-proc-prio');
      const costInput = document.getElementById('add-proc-cost');
      const errBox = document.getElementById('add-proc-error');

      if (errBox) errBox.classList.add('hidden');

      const id = idInput.value.trim();
      const name = nameInput.value.trim();
      const priority = parseInt(prioInput.value, 10);
      const cost = parseInt(costInput.value, 10);

      const state = window.SimulationAdapter.getState();
      const initialAlloc = [];
      state.resources.forEach(r => {
        const input = document.querySelector(`input[name="alloc_${r.id}"]`);
        initialAlloc.push(input ? parseInt(input.value, 10) || 0 : 0);
      });

      const maxDemand = initialAlloc.map((v, idx) => {
        const cap = (state.resources && state.resources[idx]) ? state.resources[idx].capacity : 10;
        return Math.min(cap, Math.max(v, v + 1));
      });

      try {
        await window.SimulationAdapter.addProcess({
          id,
          name,
          priority,
          cost,
          initialAlloc,
          maxDemand
        });

        // Reset inputs
        idInput.value = '';
        nameInput.value = '';
        prioInput.value = '5';
        costInput.value = '50';
        // Reset allocation inputs
        state.resources.forEach(r => {
          const input = document.querySelector(`input[name="alloc_${r.id}"]`);
          if (input) input.value = '0';
        });
        window.App.showToast(`Process ${id || name} successfully added to kernel table.`, 'success');
      } catch (err) {
        if (errBox) {
          errBox.textContent = err.message;
          errBox.classList.remove('hidden');
        }
        window.App.showToast(err.message, 'error');
      }
    }

    async _handleSubmitRequest() {
      const procSelect = document.getElementById('req-target-process');
      const resSelect = document.getElementById('req-target-resource');
      const unitsInput = document.getElementById('req-units');
      const errBox = document.getElementById('req-error');

      if (errBox) errBox.classList.add('hidden');

      const procId = procSelect.value;
      const resId = resSelect.value;
      const units = parseInt(unitsInput.value, 10);

      try {
        await window.SimulationAdapter.submitRequest(procId, resId, units);
        window.App.showToast(`Resource request submitted for ${procId} (${units}x ${resId}).`, 'info');
      } catch (err) {
        if (errBox) {
          errBox.textContent = err.message;
          errBox.classList.remove('hidden');
        }
        window.App.showToast(err.message, 'error');
      }
    }

    quickClaim(processId) {
      const dropdown = document.getElementById('req-target-process');
      if (dropdown) dropdown.value = processId;
      const unitsInput = document.getElementById('req-units');
      if (unitsInput) unitsInput.focus();
    }

    async quickRelease(processId) {
      const state = window.SimulationAdapter.getState();
      const pIdx = state.processes.findIndex(p => p.id === processId);
      if (pIdx === -1) return;

      let released = false;
      for (let j = 0; j < state.resources.length; j++) {
        const held = state.allocation[pIdx] ? state.allocation[pIdx][j] : 0;
        if (held > 0) {
          try {
            await window.SimulationAdapter.releaseResources(processId, state.resources[j].id, held);
            window.App.showToast(`Released ${held} unit(s) of ${state.resources[j].id} from ${processId}.`, 'success');
            released = true;
            break;
          } catch (err) {
            window.App.showToast(`Release failed: ${err.message}`, 'error');
            return;
          }
        }
      }

      if (!released) {
        window.App.showToast(`Process ${processId} has no allocated resources to release.`, 'info');
      }
    }

    async deleteProcess(processId) {
      try {
        const state = window.SimulationAdapter.getState();
        const proc = (state.processes || []).find(p => p.id === processId);
        const name = proc ? proc.name : processId;

        await window.SimulationAdapter.deleteProcess(processId);
        window.App.showToast(`Deleted ${name} (${processId}) from kernel table. Held resources reclaimed.`, 'success');
      } catch (err) {
        window.App.showToast(`Delete failed: ${err.message}`, 'error');
      }
    }

    async clearAllProcesses() {
      const state = window.SimulationAdapter.getState();
      if (!state.processes || state.processes.length === 0) {
        window.App.showToast('No processes to delete.', 'info');
        return;
      }
      try {
        await window.SimulationAdapter.clearProcesses();
        window.App.showToast('All registered processes deleted. Resources returned to Available pool.', 'success');
      } catch (err) {
        window.App.showToast(`Failed to clear processes: ${err.message}`, 'error');
      }
    }

    confirmTerminate(processId) {
      window.App.showRecoveryModal(processId);
    }
  }

  window.ProcessesScreen = new ProcessesScreen();

})(window);
