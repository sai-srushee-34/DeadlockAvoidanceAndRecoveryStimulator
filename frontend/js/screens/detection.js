/**
 * Kernel Systems Deadlock Monitor - Detection Screen Controller
 * (Shoshani-Coffman Multi-Instance Matrix Reduction Engine)
 */

(function (window) {
  'use strict';

  class DetectionScreen {
    constructor() {
      this.initialized = false;
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      const btnScan = document.getElementById('btn-run-detection-scan');
      if (btnScan) {
        btnScan.addEventListener('click', async () => {
          btnScan.disabled = true;
          btnScan.innerHTML = `<span class="material-symbols-outlined text-[16px] animate-spin">sync</span><span>Analyzing Matrix Invariants...</span>`;

          try {
            const report = await window.SimulationAdapter.runDetection();
            btnScan.disabled = false;
            btnScan.innerHTML = `<span class="material-symbols-outlined text-[16px]">troubleshoot</span><span>Execute Detection Scan</span>`;
            if (report.isDeadlocked) {
              window.App.showToast(`Deadlock detected in candidate set { ${report.deadlockedProcesses.join(', ')} }`, 'error');
            } else {
              window.App.showToast('Detection complete: All processes successfully reduced. Safe state.', 'success');
            }
          } catch (err) {
            btnScan.disabled = false;
            btnScan.innerHTML = `<span class="material-symbols-outlined text-[16px]">troubleshoot</span><span>Execute Detection Scan</span>`;
            window.App.showToast(`Detection error: ${err.message}`, 'error');
          }
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();

      const { detectionReport, processes, resources, allocation, request, available } = state;
      const report = detectionReport || window.MockSimulationAdapter.detectionReport;

      // Update Detection Status Card
      const statusCard = document.getElementById('detection-status-card');
      const statusTitle = document.getElementById('detection-status-title');
      const statusBadge = document.getElementById('detection-status-badge');
      const statusDesc = document.getElementById('detection-status-desc');
      const candidateList = document.getElementById('detection-candidate-list');

      if (report && report.isDeadlocked) {
        if (statusCard) statusCard.className = 'p-space-lg rounded bg-surface-container-lowest border-l-4 border-error shadow-sm flex flex-col gap-space-sm';
        if (statusTitle) statusTitle.textContent = 'Irreducible Deadlock Cycle Detected';
        if (statusBadge) {
          statusBadge.className = 'px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-label-xs text-label-xs font-semibold flex items-center gap-1';
          statusBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-error animate-pulse"></span> DEADLOCK PRESENT';
        }
        if (statusDesc) {
          statusDesc.textContent = `The matrix reduction terminated before all processes finished. Outstanding requests exceed remaining available capacity, creating an unresolvable cycle.`;
        }
        if (candidateList) {
          candidateList.innerHTML = `
            <div class="flex items-center gap-space-sm p-space-sm bg-error-container/20 rounded border border-error/30">
              <span class="font-label-xs text-label-xs uppercase font-bold text-error">Unreduced Candidate Set D:</span>
              <div class="flex flex-wrap gap-1 font-mono text-label-xs font-bold text-error">
                ${report.deadlockedProcesses.map(pId => `<span class="px-2 py-0.5 bg-error text-white rounded">${pId}</span>`).join('')}
              </div>
            </div>
          `;
        }
      } else {
        if (statusCard) statusCard.className = 'p-space-lg rounded bg-surface-container-lowest border-l-4 border-tertiary shadow-sm flex flex-col gap-space-sm';
        if (statusTitle) statusTitle.textContent = 'System Matrix Completely Reducible (Safe State)';
        if (statusBadge) {
          statusBadge.className = 'px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold flex items-center gap-1';
          statusBadge.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary"></span> ZERO CYCLES';
        }
        if (statusDesc) {
          statusDesc.textContent = `A complete reduction sequence exists where all processes can sequentially finish and release their held resources. Candidate set D = ∅.`;
        }
        if (candidateList) {
          candidateList.innerHTML = `
            <div class="flex items-center gap-space-sm p-space-sm bg-tertiary-fixed-dim/10 rounded border border-tertiary/20">
              <span class="font-label-xs text-label-xs uppercase font-bold text-tertiary">Unreduced Candidate Set D:</span>
              <span class="font-mono text-label-xs text-outline font-semibold">∅ (Empty Set - All processes finish)</span>
            </div>
          `;
        }
      }

      // Render Matrix Reduction Trace Timeline
      const traceBody = document.getElementById('detection-trace-body');
      if (traceBody) {
        if (!report || !report.trace || report.trace.length === 0) {
          traceBody.innerHTML = `<tr><td colspan="6" class="p-space-lg text-center text-outline">No reduction steps recorded. Either no processes were holding resources or zero processes were satisfied.</td></tr>`;
        } else {
          traceBody.innerHTML = report.trace.map(step => `
            <tr class="hover:bg-surface-container-lowest transition-colors font-mono">
              <td class="py-space-sm px-space-md font-bold text-outline">Step #${step.step}</td>
              <td class="py-space-sm px-space-md font-semibold text-primary">
                ${step.processId} <span class="font-sans font-normal text-[10px] text-outline">(${step.processName})</span>
              </td>
              <td class="py-space-sm px-space-md text-tertiary">
                [ ${step.requestVector.join(', ')} ] ≤ [ ${step.workBefore.join(', ')} ]
              </td>
              <td class="py-space-sm px-space-md text-on-surface">
                + [ ${step.allocatedReclaimed.join(', ')} ]
              </td>
              <td class="py-space-sm px-space-md font-bold text-primary">
                [ ${step.workAfter.join(', ')} ]
              </td>
              <td class="py-space-sm px-space-md">
                <span class="px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold">
                  REDUCED
                </span>
              </td>
            </tr>
          `).join('');
        }
      }

      // Render Finish Vector State Table
      const finishTableBody = document.getElementById('detection-finish-vector-body');
      if (finishTableBody && report && report.finishVector) {
        finishTableBody.innerHTML = processes.map((p, pIdx) => {
          const isFinished = report.finishVector[pIdx];
          const isDeadlock = report.deadlockedProcesses.includes(p.id);

          return `
            <tr class="hover:bg-surface-container-lowest transition-colors font-mono">
              <td class="py-space-sm px-space-md font-semibold ${isDeadlock ? 'text-error' : 'text-primary'}">${p.id}</td>
              <td class="py-space-sm px-space-md font-sans text-body-sm">${p.name}</td>
              <td class="py-space-sm px-space-md text-on-surface">[ ${allocation[pIdx] ? allocation[pIdx].join(', ') : '0'} ]</td>
              <td class="py-space-sm px-space-md text-outline">[ ${request[pIdx] ? request[pIdx].join(', ') : '0'} ]</td>
              <td class="py-space-sm px-space-md">
                ${isFinished ? `
                  <span class="px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs font-semibold">TRUE</span>
                ` : `
                  <span class="px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-label-xs font-semibold">FALSE (DEADLOCKED)</span>
                `}
              </td>
            </tr>
          `;
        }).join('');
      }
    }
  }

  window.DetectionScreen = new DetectionScreen();

})(window);
