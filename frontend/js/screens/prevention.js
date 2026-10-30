/**
 * Kernel Systems Deadlock Monitor - Prevention Screen Controller
 * (Reusing and enhancing the Google Stitch Havender Invariant Sandboxes)
 */

(function (window) {
  'use strict';

  class PreventionScreen {
    constructor() {
      this.initialized = false;
      this.interceptionHistory = [
        { time: '14:02:19.408', pid: 'PID 1088 (P3)', op: 'CLAIM(R0) while holds [R2]', invariant: 'Circular-Wait: F(R0)=1 ≤ F(R2)=3', decision: 'BLOCK_IMMEDIATE', mitigation: 'Thread trap raised; Context preserved without lock' },
        { time: '14:02:18.992', pid: 'PID 1042 (P1)', op: 'CLAIM(R2) with active [R1]', invariant: 'Hold-and-Wait: Protocol B Non-Zero Held', decision: 'REJECT_PREEMPT', mitigation: 'Client re-invoked with auto-relinquish flag' },
        { time: '14:02:17.114', pid: 'PID 1102 (P7)', op: 'CLAIM(R0) while holds [R3]', invariant: 'Circular-Wait: F(R0)=1 ≤ F(R3)=4', decision: 'BLOCK_IMMEDIATE', mitigation: 'Signal SIGDEADLK delivered to user thread' }
      ];
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      // Hold & Wait Interactive Sandbox Controls
      const hwProcess = document.getElementById('hw-target-process');
      const hwResource = document.getElementById('hw-target-resource');
      const hwDesc = document.getElementById('hw-held-desc');
      const btnEvalHw = document.getElementById('btn-eval-hw');
      const btnAutoRelHw = document.getElementById('btn-auto-release-hw');
      const hwBadge = document.getElementById('hw-badge-status');
      const hwMsg = document.getElementById('hw-eval-msg');

      if (hwProcess && hwDesc) {
        hwProcess.addEventListener('change', () => this._updateHwHeldDesc());
      }

      if (btnEvalHw) {
        btnEvalHw.addEventListener('click', () => {
          const procId = hwProcess.value;
          const resId = hwResource.value;
          const state = window.MockSimulationAdapter.getState();
          const pIdx = state.processes.findIndex(p => p.id === procId);
          const held = (pIdx !== -1 && state.allocation[pIdx]) ? state.allocation[pIdx].reduce((a, b) => a + b, 0) : 0;

          if (held > 0) {
            hwBadge.className = 'px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-label-xs text-label-xs font-semibold flex items-center gap-1';
            hwBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-error"></span> REJECTED: POLICY VIOLATION';
            hwMsg.innerHTML = `[KERNEL_TRAP] sys_resource_claim(PID=${procId}, Res=${resId}) REJECTED.<br>` +
              `Hold-and-Wait Violation: Process holds ${held} active unit(s). Protocol B mandates 0 allocations prior to issuing new claims.<br>` +
              `Interception: Prevented potential resource hoarding starvation.`;
            
            this._recordInterception(procId, `CLAIM(${resId}) while holds [${held} units]`, 'Hold-and-Wait: Protocol B Non-Zero Held', 'REJECT_PREEMPT', 'Syscall rejected; Client must relinquish');
          } else {
            hwBadge.className = 'px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold flex items-center gap-1';
            hwBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary"></span> PERMITTED: COMPLIANT WITH PROTOCOL B';
            hwMsg.innerHTML = `[KERNEL_OK] sys_resource_claim(PID=${procId}, Res=${resId}) APPROVED.<br>` +
              `Hold check verified: Process currently holds 0 allocations.<br>` +
              `Invariant satisfied: Transition allowed to allocation scheduler.`;
          }
        });
      }

      if (btnAutoRelHw) {
        btnAutoRelHw.addEventListener('click', () => {
          const procId = hwProcess.value;
          const resId = hwResource.value;
          hwBadge.className = 'px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold flex items-center gap-1';
          hwBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary"></span> PERMITTED VIA AUTO-RELEASE';
          hwMsg.innerHTML = `[KERNEL_STEP] Relinquish sequence triggered for ${procId}:<br>` +
            `1. Discharged all currently leased units back to pool.<br>` +
            `2. Atomic verification: |Held(${procId})| == 0.<br>` +
            `3. Approved claim request for target ${resId}. Hold-and-wait condition invalidated.`;
        });
      }

      // Circular Wait Hierarchy Sandbox Controls
      const cwProc = document.getElementById('cw-process');
      const cwReq = document.getElementById('cw-request');
      const btnEvalCw = document.getElementById('btn-eval-cw');

      if (cwProc && cwReq) {
        cwProc.addEventListener('change', () => this._updateCwLabels());
        cwReq.addEventListener('change', () => this._updateCwLabels());
      }

      if (btnEvalCw) {
        btnEvalCw.addEventListener('click', () => this._evaluateCircularWait());
      }

      // Policy Toggles
      const toggleHw = document.getElementById('toggle-hold-wait');
      if (toggleHw) {
        toggleHw.addEventListener('click', async () => {
          const active = toggleHw.getAttribute('aria-checked') === 'true';
          const next = !active;
          try {
            await window.SimulationAdapter.setPreventionPolicy(next ? 'HOLD_AND_WAIT' : 'NONE');
            window.App.showToast(`Hold-and-Wait Prevention Policy: ${next ? 'ACTIVATED' : 'DISABLED'}`, next ? 'success' : 'info');
          } catch (err) {
            window.App.showToast(`Policy update failed: ${err.message}`, 'error');
          }
        });
      }

      const toggleCw = document.getElementById('toggle-circular-wait');
      if (toggleCw) {
        toggleCw.addEventListener('click', async () => {
          const active = toggleCw.getAttribute('aria-checked') === 'true';
          const next = !active;
          try {
            await window.SimulationAdapter.setPreventionPolicy(next ? 'CIRCULAR_WAIT' : 'NONE');
            window.App.showToast(`Circular-Wait Hierarchy Policy: ${next ? 'ACTIVATED' : 'DISABLED'}`, next ? 'success' : 'info');
          } catch (err) {
            window.App.showToast(`Policy update failed: ${err.message}`, 'error');
          }
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();

      // Sync toggle buttons with state
      const toggleHw = document.getElementById('toggle-hold-wait');
      if (toggleHw && state.preventionSettings) {
        const isHw = Boolean(state.preventionSettings.holdWaitActive);
        toggleHw.setAttribute('aria-checked', String(isHw));
        const thumb = toggleHw.querySelector('span');
        if (isHw) {
          toggleHw.className = 'relative inline-flex h-6 w-11 items-center rounded-full bg-tertiary transition-colors focus:outline-none';
          if (thumb) thumb.className = 'translate-x-6 inline-block h-4 w-4 transform rounded-full bg-surface-container-lowest transition-transform';
        } else {
          toggleHw.className = 'relative inline-flex h-6 w-11 items-center rounded-full bg-outline transition-colors focus:outline-none';
          if (thumb) thumb.className = 'translate-x-1 inline-block h-4 w-4 transform rounded-full bg-surface-container-lowest transition-transform';
        }
      }

      const toggleCw = document.getElementById('toggle-circular-wait');
      if (toggleCw && state.preventionSettings) {
        const isCw = Boolean(state.preventionSettings.circularWaitActive);
        toggleCw.setAttribute('aria-checked', String(isCw));
        const thumb = toggleCw.querySelector('span');
        if (isCw) {
          toggleCw.className = 'relative inline-flex h-6 w-11 items-center rounded-full bg-tertiary transition-colors focus:outline-none';
          if (thumb) thumb.className = 'translate-x-6 inline-block h-4 w-4 transform rounded-full bg-surface-container-lowest transition-transform';
        } else {
          toggleCw.className = 'relative inline-flex h-6 w-11 items-center rounded-full bg-outline transition-colors focus:outline-none';
          if (thumb) thumb.className = 'translate-x-1 inline-block h-4 w-4 transform rounded-full bg-surface-container-lowest transition-transform';
        }
      }

      const { processes, resources, allocation } = state;

      // Populate Hold & Wait Sandbox Process Dropdown
      const hwProcess = document.getElementById('hw-target-process');
      if (hwProcess) {
        const newHtml = processes.filter(p => p.state !== 'TERMINATED').map((p, pIdx) => {
          const heldList = [];
          resources.forEach((r, rIdx) => {
            const count = (allocation[pIdx] && allocation[pIdx][rIdx]) || 0;
            if (count > 0) heldList.push(`${r.id} (${count}x)`);
          });
          return `<option value="${p.id}">${p.id} (PID: ${p.pid}) • Holds [${heldList.length ? heldList.join(', ') : 'None ∅'}]</option>`;
        }).join('');
        if (hwProcess.dataset.renderedHtml !== newHtml) {
          const cur = hwProcess.value;
          hwProcess.innerHTML = newHtml;
          hwProcess.dataset.renderedHtml = newHtml;
          if (cur && processes.some(p => p.id === cur)) hwProcess.value = cur;
          this._updateHwHeldDesc();
        }
      }

      // Populate Hold & Wait Resource Dropdown
      const hwResource = document.getElementById('hw-target-resource');
      if (hwResource) {
        const newHtml = resources.map(r => `
          <option value="${r.id}">${r.id} (${r.name} • Order ${r.orderRank})</option>
        `).join('');
        if (hwResource.dataset.renderedHtml !== newHtml) {
          const cur = hwResource.value;
          hwResource.innerHTML = newHtml;
          hwResource.dataset.renderedHtml = newHtml;
          if (cur && resources.some(r => r.id === cur)) hwResource.value = cur;
        }
      }

      // Populate Circular Wait Sandbox Dropdowns
      const cwProc = document.getElementById('cw-process');
      if (cwProc) {
        const newHtml = processes.filter(p => p.state !== 'TERMINATED').map((p, pIdx) => {
          let maxRank = 0;
          let maxRes = 'None';
          resources.forEach((r, rIdx) => {
            if (allocation[pIdx] && allocation[pIdx][rIdx] > 0) {
              if (r.orderRank > maxRank) {
                maxRank = r.orderRank;
                maxRes = r.id;
              }
            }
          });
          return `<option value="${p.id}" data-max-rank="${maxRank}">${p.id} • Highest Held: ${maxRes} (Rank ${maxRank})</option>`;
        }).join('');
        if (cwProc.dataset.renderedHtml !== newHtml) {
          const cur = cwProc.value;
          cwProc.innerHTML = newHtml;
          cwProc.dataset.renderedHtml = newHtml;
          if (cur && processes.some(p => p.id === cur)) cwProc.value = cur;
        }
      }

      const cwReq = document.getElementById('cw-request');
      if (cwReq) {
        const newHtml = resources.map(r => `
          <option value="${r.id}" data-rank="${r.orderRank}">${r.id} (${r.name} • Rank ${r.orderRank})</option>
        `).join('');
        if (cwReq.dataset.renderedHtml !== newHtml) {
          const cur = cwReq.value;
          cwReq.innerHTML = newHtml;
          cwReq.dataset.renderedHtml = newHtml;
          if (cur && resources.some(r => r.id === cur)) cwReq.value = cur;
        }
      }

      this._updateCwLabels();

      // Render Dynamic Ordering Table
      const orderTableBody = document.getElementById('prevention-order-table-body');
      if (orderTableBody) {
        orderTableBody.innerHTML = resources.map((r, idx) => `
          <div class="grid grid-cols-12 items-center px-space-md py-space-sm text-on-surface hover:bg-surface-container-lowest transition-colors">
            <span class="col-span-3 flex items-center gap-1 font-label-sm text-label-sm text-outline">
              <span class="material-symbols-outlined text-[14px]">${idx === 0 ? 'south' : (idx === resources.length - 1 ? 'north' : 'unfold_more')}</span>
              ${r.orderRank} ${idx === 0 ? '(Lowest)' : (idx === resources.length - 1 ? '(Highest)' : '')}
            </span>
            <span class="col-span-3 font-mono font-semibold text-primary">${r.id}</span>
            <span class="col-span-4 font-body-sm text-body-sm truncate">${r.name}</span>
            <span class="col-span-2 text-right font-mono font-semibold bg-surface-container-high px-1.5 py-0.5 rounded text-label-xs">${r.orderRank}</span>
          </div>
        `).join('');
      }

      // Render Interception Table
      this._renderInterceptionTable();
    }

    _updateHwHeldDesc() {
      const hwProcess = document.getElementById('hw-target-process');
      const hwDesc = document.getElementById('hw-held-desc');
      if (!hwProcess || !hwDesc) return;

      const procId = hwProcess.value;
      const state = window.MockSimulationAdapter.getState();
      const pIdx = state.processes.findIndex(p => p.id === procId);
      if (pIdx === -1) return;

      const heldParts = [];
      state.resources.forEach((r, rIdx) => {
        const c = (state.allocation[pIdx] && state.allocation[pIdx][rIdx]) || 0;
        if (c > 0) heldParts.push(`${r.id} (${c} unit${c > 1 ? 's' : ''})`);
      });

      hwDesc.innerHTML = `Currently holding: <strong class="text-on-surface">${heldParts.length ? heldParts.join(', ') : 'None [Empty Set ∅]'}</strong>`;
    }

    _updateCwLabels() {
      const cwProc = document.getElementById('cw-process');
      const cwReq = document.getElementById('cw-request');
      const cwHeldRank = document.getElementById('cw-held-rank');
      const cwReqRank = document.getElementById('cw-req-rank');

      if (!cwProc || !cwReq) return;

      const selectedProcOpt = cwProc.options[cwProc.selectedIndex];
      const selectedReqOpt = cwReq.options[cwReq.selectedIndex];

      const maxRank = selectedProcOpt ? selectedProcOpt.getAttribute('data-max-rank') || '0' : '0';
      const reqRank = selectedReqOpt ? selectedReqOpt.getAttribute('data-rank') || '1' : '1';

      if (cwHeldRank) cwHeldRank.textContent = `F(Held_Max) = ${maxRank}`;
      if (cwReqRank) cwReqRank.textContent = `F(Requested: ${cwReq.value}) = ${reqRank}`;
    }

    _evaluateCircularWait() {
      const cwProc = document.getElementById('cw-process');
      const cwReq = document.getElementById('cw-request');
      const cwStatusBadge = document.getElementById('cw-status-badge');
      const cwMath = document.getElementById('cw-math-statement');
      const cwRationale = document.getElementById('cw-rationale-text');

      if (!cwProc || !cwReq) return;

      const selectedProcOpt = cwProc.options[cwProc.selectedIndex];
      const selectedReqOpt = cwReq.options[cwReq.selectedIndex];

      const hRank = parseInt(selectedProcOpt.getAttribute('data-max-rank'), 10) || 0;
      const rRank = parseInt(selectedReqOpt.getAttribute('data-rank'), 10) || 1;
      const rId = cwReq.value;
      const pId = cwProc.value;

      if (rRank > hRank) {
        // Valid
        cwStatusBadge.className = 'px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold flex items-center gap-1';
        cwStatusBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary"></span> REQUEST GRANTED (HIERARCHY PRESERVED)';
        cwMath.className = 'text-tertiary font-semibold';
        cwMath.textContent = `F(Requested: ${rId}) = ${rRank} > F(Held_Max) = ${hRank}`;
        cwRationale.textContent = `Strictly ascending order verified. Because each subsequent request must possess a higher index, closed cyclic dependency graphs cannot form.`;
      } else {
        // Rejected
        cwStatusBadge.className = 'px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-label-xs text-label-xs font-semibold flex items-center gap-1';
        cwStatusBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-error"></span> REQUEST REJECTED (CYCLE PREVENTION)';
        cwMath.className = 'text-error font-semibold';
        cwMath.textContent = `F(Requested: ${rId}) = ${rRank} ≤ F(Held_Max) = ${hRank}`;
        cwRationale.textContent = `Violates hierarchical ordering rule. Requesting lower-ordered resource while holding higher-ordered resource could induce circular wait cycles across concurrent threads.`;

        this._recordInterception(pId, `CLAIM(${rId}) while holds rank ${hRank}`, `Circular-Wait: F(${rId})=${rRank} ≤ F(Held_Max)=${hRank}`, 'BLOCK_IMMEDIATE', 'Syscall blocked; Thread trapped without lock');
      }
    }

    _recordInterception(pid, op, invariant, decision, mitigation) {
      const now = new Date();
      const timeStr = now.toTimeString().split(' ')[0] + '.' + String(now.getMilliseconds()).padStart(3, '0');
      this.interceptionHistory.unshift({
        time: timeStr,
        pid: pid,
        op: op,
        invariant: invariant,
        decision: decision,
        mitigation: mitigation
      });
      if (this.interceptionHistory.length > 25) this.interceptionHistory.pop();
      this._renderInterceptionTable();
    }

    _renderInterceptionTable() {
      const container = document.getElementById('prevention-audit-body');
      if (!container) return;

      container.innerHTML = this.interceptionHistory.map(row => `
        <tr class="hover:bg-surface-container-lowest transition-colors">
          <td class="py-space-sm px-space-md text-outline">${row.time}</td>
          <td class="py-space-sm px-space-md font-semibold text-primary">${row.pid}</td>
          <td class="py-space-sm px-space-md"><code class="text-on-surface font-mono">${row.op}</code></td>
          <td class="py-space-sm px-space-md text-error font-mono">${row.invariant}</td>
          <td class="py-space-sm px-space-md">
            <span class="px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-semibold">${row.decision}</span>
          </td>
          <td class="py-space-sm px-space-md text-on-surface-variant font-sans">${row.mitigation}</td>
        </tr>
      `).join('');
    }
  }

  window.PreventionScreen = new PreventionScreen();

})(window);
