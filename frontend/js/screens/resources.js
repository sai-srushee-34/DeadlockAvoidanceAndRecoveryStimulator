/**
 * Kernel Systems Deadlock Monitor - Resources Screen Controller
 */

(function (window) {
  'use strict';

  class ResourcesScreen {
    constructor() {
      this.initialized = false;
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      // Bind Resource Creation Form
      const addForm = document.getElementById('form-add-resource');
      if (addForm) {
        addForm.addEventListener('submit', (e) => {
          e.preventDefault();
          this._handleAddResource();
        });
      }

      // Bind Clear All Resources Button
      const clearBtn = document.getElementById('btn-clear-all-resources');
      if (clearBtn) {
        clearBtn.addEventListener('click', () => {
          this.clearAllResources();
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();

      const { resources, allocation, available, processes } = state;

      // Update Summary Header
      const countEl = document.getElementById('res-count-summary');
      if (countEl) {
        let totalCap = 0;
        let totalAlloc = 0;
        (resources || []).forEach((r, idx) => {
          totalCap += r.capacity;
          (processes || []).forEach((p, pIdx) => {
            if (p.state !== 'TERMINATED' && allocation[pIdx]) {
              totalAlloc += allocation[pIdx][idx] || 0;
            }
          });
        });
        const totalAvail = Math.max(0, totalCap - totalAlloc);
        countEl.textContent = `${(resources || []).length} Resource Classes (${totalCap} Total Instances • ${totalAlloc} Allocated • ${totalAvail} Available)`;
      }

      // Render Resource Inventory Table
      const tableBody = document.getElementById('table-resources-body');
      if (!tableBody) return;

      if (!resources || resources.length === 0) {
        tableBody.innerHTML = `<tr><td colspan="8" class="p-space-lg text-center text-outline">No resource classes registered. Use the form to define custom resources.</td></tr>`;
        return;
      }

      tableBody.innerHTML = resources.map((r, rIdx) => {
        let heldCount = 0;
        (processes || []).forEach((p, pIdx) => {
          if (p.state !== 'TERMINATED' && allocation[pIdx]) {
            heldCount += allocation[pIdx][rIdx] || 0;
          }
        });
        const availCount = Math.max(0, r.capacity - heldCount);
        const utilPct = r.capacity > 0 ? ((heldCount / r.capacity) * 100).toFixed(0) : 0;

        return `
          <tr class="hover:bg-surface-container-lowest transition-colors">
            <td class="py-space-sm px-space-md font-mono font-bold text-primary">
              ${r.id}
            </td>
            <td class="py-space-sm px-space-md font-body-sm font-medium text-on-surface">
              ${r.name}
              <span class="block font-sans text-[10px] text-outline font-normal">${r.unit || 'Units'}</span>
            </td>
            <td class="py-space-sm px-space-md font-mono font-semibold text-on-surface">
              ${r.capacity}
            </td>
            <td class="py-space-sm px-space-md font-mono font-bold text-primary">
              ${heldCount}
            </td>
            <td class="py-space-sm px-space-md font-mono font-bold text-tertiary">
              ${availCount}
            </td>
            <td class="py-space-sm px-space-md font-mono text-label-xs">
              <span class="px-2 py-0.5 rounded bg-surface-container-high font-semibold text-on-surface">
                F(r) = ${r.orderRank}
              </span>
            </td>
            <td class="py-space-sm px-space-md">
              <div class="flex items-center gap-space-sm">
                <div class="w-24 h-2 bg-surface-container rounded-full overflow-hidden">
                  <div class="h-full bg-primary rounded-full transition-all duration-300" style="width: ${utilPct}%"></div>
                </div>
                <span class="font-mono text-[10px] font-semibold text-outline">${utilPct}%</span>
              </div>
            </td>
            <td class="py-space-sm px-space-md text-right">
              <button type="button" class="h-6 px-2 text-label-xs font-label-xs rounded bg-error/15 text-error hover:bg-error hover:text-white transition-colors" title="Delete resource class" onclick="window.ResourcesScreen.deleteResource('${r.id}')">
                Delete
              </button>
            </td>
          </tr>
        `;
      }).join('');

      // Render Visual Instance Slots
      const slotsContainer = document.getElementById('res-instances-grid');
      if (slotsContainer) {
        slotsContainer.innerHTML = resources.map((r, rIdx) => {
          let heldCount = 0;
          processes.forEach((p, pIdx) => {
            if (p.state !== 'TERMINATED' && allocation[pIdx]) {
              heldCount += allocation[pIdx][rIdx] || 0;
            }
          });
          const availCount = Math.max(0, r.capacity - heldCount);

          let dotsHtml = '';
          for (let i = 0; i < r.capacity; i++) {
            if (i < heldCount) {
              dotsHtml += `<div class="w-3.5 h-3.5 rounded-sm bg-primary border border-primary flex items-center justify-center text-[8px] text-white font-mono font-bold" title="Instance #${i+1}: Allocated">A</div>`;
            } else {
              dotsHtml += `<div class="w-3.5 h-3.5 rounded-sm bg-surface-container-lowest border border-tertiary flex items-center justify-center text-[8px] text-tertiary font-mono font-bold" title="Instance #${i+1}: Available">✓</div>`;
            }
          }

          return `
            <div class="p-space-md rounded bg-surface-container-lowest shadow-sm flex flex-col gap-space-xs border border-surface-container-high/60">
              <div class="flex items-center justify-between pb-1 border-b border-surface-container-low">
                <div class="flex items-center gap-1.5">
                  <span class="font-mono font-bold text-primary text-label-sm">${r.id}</span>
                  <span class="font-body-sm font-semibold text-on-surface">${r.name}</span>
                </div>
                <span class="font-mono text-[10px] text-outline font-semibold bg-surface-container-low px-1.5 py-0.5 rounded">Rank ${r.orderRank}</span>
              </div>
              <div class="flex items-center justify-between font-label-xs text-[10px] text-outline pt-0.5">
                <span>Allocated: <strong class="text-primary font-mono">${heldCount}</strong></span>
                <span>Available: <strong class="text-tertiary font-mono">${availCount}</strong></span>
              </div>
              <div class="flex flex-wrap gap-1.5 pt-1">
                ${dotsHtml}
              </div>
            </div>
          `;
        }).join('');
      }
    }

    async _handleAddResource() {
      const idInput = document.getElementById('add-res-id');
      const nameInput = document.getElementById('add-res-name');
      const capInput = document.getElementById('add-res-capacity');
      const rankInput = document.getElementById('add-res-rank');
      const unitInput = document.getElementById('add-res-unit');
      const errBox = document.getElementById('add-res-error');

      if (errBox) errBox.classList.add('hidden');

      const id = idInput.value.trim();
      const name = nameInput.value.trim();
      const capacity = parseInt(capInput.value, 10);
      const orderRank = parseInt(rankInput.value, 10);
      const unit = unitInput.value.trim() || 'Units';

      try {
        await window.SimulationAdapter.addResource({
          id,
          name,
          capacity,
          orderRank,
          unit
        });

        // Clear inputs
        idInput.value = '';
        nameInput.value = '';
        capInput.value = '4';
        rankInput.value = '';
        unitInput.value = 'Units';
        window.App.showToast(`Resource class ${id || name} defined successfully.`, 'success');
      } catch (err) {
        if (errBox) {
          errBox.textContent = err.message;
          errBox.classList.remove('hidden');
        }
        window.App.showToast(err.message, 'error');
      }
    }

    async deleteResource(resourceId) {
      try {
        const state = window.SimulationAdapter.getState();
        const res = (state.resources || []).find(r => r.id === resourceId);
        const name = res ? res.name : resourceId;

        await window.SimulationAdapter.deleteResource(resourceId);
        window.App.showToast(`Deleted resource class ${name} (${resourceId}) from kernel catalog.`, 'success');
      } catch (err) {
        window.App.showToast(`Delete failed: ${err.message}`, 'error');
      }
    }

    async clearAllResources() {
      const state = window.SimulationAdapter.getState();
      if (!state.resources || state.resources.length === 0) {
        window.App.showToast('No resource classes to delete.', 'info');
        return;
      }
      try {
        await window.SimulationAdapter.clearResources();
        window.App.showToast('All resource classes deleted from hardware catalog.', 'success');
      } catch (err) {
        window.App.showToast(`Failed to clear resources: ${err.message}`, 'error');
      }
    }
  }

  window.ResourcesScreen = new ResourcesScreen();

})(window);
