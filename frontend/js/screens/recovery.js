/**
 * Kernel Systems Deadlock Monitor - Recovery Screen Controller
 * (Phase 5 Recovery: Victim Selection Heuristics & Process Termination)
 */

(function (window) {
  'use strict';

  class RecoveryScreen {
    constructor() {
      this.initialized = false;
      this.recoveryHistory = [];
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      // Manual Victim Termination Trigger
      const btnManual = document.getElementById('btn-manual-recover');
      if (btnManual) {
        btnManual.addEventListener('click', () => {
          const select = document.getElementById('select-manual-victim');
          if (!select || !select.value) {
            window.App.showToast('Please select a victim process from the candidate list.', 'warning');
            return;
          }
          window.App.showRecoveryModal(select.value);
        });
      }

      // Automated Heuristic Recovery Trigger
      const btnAuto = document.getElementById('btn-auto-recover');
      if (btnAuto) {
        btnAuto.addEventListener('click', () => {
          this._executeAutomatedRecovery();
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();

      const { isDeadlocked, deadlockedProcesses, processes } = state;
      const candidates = window.MockSimulationAdapter.getVictimCandidates();

      // Update Recovery Required Banner
      const banner = document.getElementById('recovery-status-banner');
      const bannerTitle = document.getElementById('recovery-banner-title');
      const bannerDesc = document.getElementById('recovery-banner-desc');
      const btnAuto = document.getElementById('btn-auto-recover');
      const btnManual = document.getElementById('btn-manual-recover');

      if (isDeadlocked) {
        if (banner) banner.className = 'p-space-lg rounded bg-error-container/30 border border-error/40 flex flex-col sm:flex-row sm:items-center justify-between gap-space-md';
        if (bannerTitle) bannerTitle.innerHTML = `<span class="material-symbols-outlined text-error text-[20px]">warning</span><span class="font-headline-sm text-error font-bold">Deadlock Recovery Action Required</span>`;
        if (bannerDesc) bannerDesc.textContent = `${deadlockedProcesses.length} process(es) in candidate set D are blocked in circular dependency. Select a victim to terminate and reclaim resource leases.`;
        if (btnAuto) btnAuto.disabled = false;
        if (btnManual) btnManual.disabled = false;
      } else {
        if (banner) banner.className = 'p-space-lg rounded bg-tertiary-fixed-dim/20 border border-tertiary/30 flex flex-col sm:flex-row sm:items-center justify-between gap-space-md';
        if (bannerTitle) bannerTitle.innerHTML = `<span class="material-symbols-outlined text-tertiary text-[20px]">check_circle</span><span class="font-headline-sm text-tertiary font-bold">No Deadlock Present (Recovery Not Needed)</span>`;
        if (bannerDesc) bannerDesc.textContent = 'System is currently operating within safe invariants. All resource claims can be satisfied without process termination.';
        if (btnAuto) btnAuto.disabled = true;
        if (btnManual) btnManual.disabled = true;
      }

      // Populate Manual Victim Dropdown
      const victimSelect = document.getElementById('select-manual-victim');
      if (victimSelect) {
        if (candidates.length === 0) {
          const emptyHtml = `<option value="">No Deadlocked Candidates Available</option>`;
          if (victimSelect.dataset.renderedHtml !== emptyHtml) {
            victimSelect.innerHTML = emptyHtml;
            victimSelect.dataset.renderedHtml = emptyHtml;
          }
          victimSelect.disabled = true;
        } else {
          victimSelect.disabled = false;
          const newHtml = candidates.map(c => `
            <option value="${c.id}">${c.id} (${c.name}) • Prio ${c.priority} • Held: ${c.heldResourcesTotal} • Cost: ${c.calculatedCost}</option>
          `).join('');
          if (victimSelect.dataset.renderedHtml !== newHtml) {
            const currentVal = victimSelect.value;
            victimSelect.innerHTML = newHtml;
            victimSelect.dataset.renderedHtml = newHtml;
            if (currentVal && candidates.some(c => c.id === currentVal)) {
              victimSelect.value = currentVal;
            }
          }
        }
      }

      // Render Victim Candidates Table
      const tableBody = document.getElementById('table-recovery-candidates-body');
      if (tableBody) {
        if (candidates.length === 0) {
          tableBody.innerHTML = `<tr><td colspan="7" class="p-space-lg text-center text-outline">Candidate set D is empty. No deadlocked processes require termination.</td></tr>`;
        } else {
          tableBody.innerHTML = candidates.map((c, idx) => `
            <tr class="hover:bg-surface-container-lowest transition-colors font-mono">
              <td class="py-space-sm px-space-md font-bold ${idx === 0 ? 'text-tertiary' : 'text-primary'}">
                ${c.id} ${idx === 0 ? '<span class="ml-1 px-1.5 py-0.2 bg-tertiary text-white font-sans text-[9px] rounded uppercase font-semibold">Recommended</span>' : ''}
              </td>
              <td class="py-space-sm px-space-md font-sans font-medium text-body-sm">${c.name}</td>
              <td class="py-space-sm px-space-md text-on-surface">Prio ${c.priority}</td>
              <td class="py-space-sm px-space-md font-bold text-primary">${c.heldResourcesTotal} units</td>
              <td class="py-space-sm px-space-md text-error">${c.requestedTotal} units</td>
              <td class="py-space-sm px-space-md font-bold text-on-surface bg-surface-container-low px-2 rounded">
                ${c.calculatedCost}
              </td>
              <td class="py-space-sm px-space-md text-right">
                <button type="button" class="h-6 px-2.5 text-label-xs font-label-xs rounded bg-error-container text-on-error-container hover:bg-error/20 transition-colors font-semibold" onclick="window.App.showRecoveryModal('${c.id}')">
                  Terminate
                </button>
              </td>
            </tr>
          `).join('');
        }
      }

      // Render Recovery Execution History
      this._renderHistory();
    }

    async _executeAutomatedRecovery() {
      const strategySelect = document.getElementById('select-recovery-strategy');
      const strategy = strategySelect ? strategySelect.value : 'MIN_COST';

      let candidates = window.SimulationAdapter.getVictimCandidates();
      if (!candidates || candidates.length === 0) {
        window.App.showToast('No deadlocked processes to terminate.', 'info');
        return;
      }

      // Heuristic sorting
      if (strategy === 'LOW_PRIO') {
        candidates.sort((a, b) => a.priority - b.priority);
      } else if (strategy === 'HIGH_ALLOC') {
        candidates.sort((a, b) => b.heldResourcesTotal - a.heldResourcesTotal);
      } else {
        // MIN_COST (default)
        candidates.sort((a, b) => a.calculatedCost - b.calculatedCost);
      }

      const victim = candidates[0];
      try {
        const outcome = await window.SimulationAdapter.recoverVictim(victim.id, `AUTOMATED_${strategy}`);

        this.recoveryHistory.unshift({
          timestamp: outcome.timestamp || new Date().toLocaleTimeString(),
          victimId: outcome.victimId,
          victimName: outcome.victimName,
          strategy: strategy,
          reclaimed: outcome.totalUnitsReclaimed,
          resolved: outcome.deadlockResolved
        });

        if (outcome.deadlockResolved) {
          window.App.showToast(`Deadlock successfully resolved! Terminated ${victim.id} and reclaimed ${outcome.totalUnitsReclaimed} units.`, 'success');
        } else {
          const rem = outcome.remainingDeadlocked ? outcome.remainingDeadlocked.join(', ') : 'None';
          window.App.showToast(`Terminated ${victim.id}. Additional deadlock candidates remain: { ${rem} }.`, 'warning');
        }

        this._renderHistory();
      } catch (err) {
        window.App.showToast(`Automated recovery failed: ${err.message}`, 'error');
      }
    }

    _renderHistory() {
      const container = document.getElementById('recovery-history-body');
      if (!container) return;

      if (this.recoveryHistory.length === 0) {
        container.innerHTML = `<tr><td colspan="5" class="p-space-md text-center text-outline">No recovery interventions recorded in this session.</td></tr>`;
        return;
      }

      container.innerHTML = this.recoveryHistory.map(item => `
        <tr class="hover:bg-surface-container-lowest transition-colors font-mono text-label-xs">
          <td class="py-space-sm px-space-md text-outline">${item.timestamp}</td>
          <td class="py-space-sm px-space-md font-bold text-error">${item.victimId} <span class="font-sans font-normal text-outline">(${item.victimName})</span></td>
          <td class="py-space-sm px-space-md text-on-surface font-semibold">${item.strategy}</td>
          <td class="py-space-sm px-space-md text-tertiary font-bold">${item.reclaimed} Units Reclaimed</td>
          <td class="py-space-sm px-space-md">
            ${item.resolved ? `
              <span class="px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-semibold">RESOLVED (SAFE)</span>
            ` : `
              <span class="px-space-xs py-0.5 rounded bg-amber-100 text-amber-800 font-semibold">PARTIAL REDUCTION</span>
            `}
          </td>
        </tr>
      `).join('');
    }
  }

  window.RecoveryScreen = new RecoveryScreen();

})(window);
