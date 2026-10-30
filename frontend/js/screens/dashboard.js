/**
 * Kernel Systems Deadlock Monitor - Dashboard Screen Controller
 */

(function (window) {
  'use strict';

  class DashboardScreen {
    constructor() {
      this.graph = null;
      this.initialized = false;
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      // Initialize SVG Graph
      this.graph = new window.ResourceAllocationGraph('rag-container');

      // Bind graph controls
      const btnZoomIn = document.getElementById('graph-zoom-in');
      const btnZoomOut = document.getElementById('graph-zoom-out');
      const btnResetView = document.getElementById('graph-reset-view');
      const filterSelect = document.getElementById('graph-edge-filter');

      if (btnZoomIn) btnZoomIn.addEventListener('click', () => this.graph.zoomIn());
      if (btnZoomOut) btnZoomOut.addEventListener('click', () => this.graph.zoomOut());
      if (btnResetView) btnResetView.addEventListener('click', () => this.graph.resetView());
      if (filterSelect) {
        filterSelect.addEventListener('change', (e) => this.graph.setFilter(e.target.value));
      }

      // Matrix toggle tabs
      const tabAlloc = document.getElementById('tab-matrix-alloc');
      const tabReq = document.getElementById('tab-matrix-req');
      const tabNeed = document.getElementById('tab-matrix-need');
      const matrixViews = ['view-matrix-alloc', 'view-matrix-req', 'view-matrix-need'];

      const setMatrixTab = (activeId, viewId) => {
        [tabAlloc, tabReq, tabNeed].forEach(tab => {
          if (!tab) return;
          if (tab.id === activeId) {
            tab.className = 'px-space-sm py-1 font-label-xs text-label-xs font-semibold rounded bg-surface-container text-primary border border-primary/20';
          } else {
            tab.className = 'px-space-sm py-1 font-label-xs text-label-xs font-medium rounded text-on-surface-variant hover:bg-surface-container';
          }
        });
        matrixViews.forEach(v => {
          const el = document.getElementById(v);
          if (el) el.style.display = (v === viewId) ? 'block' : 'none';
        });
      };

      if (tabAlloc) tabAlloc.addEventListener('click', () => setMatrixTab('tab-matrix-alloc', 'view-matrix-alloc'));
      if (tabReq) tabReq.addEventListener('click', () => setMatrixTab('tab-matrix-req', 'view-matrix-req'));
      if (tabNeed) tabNeed.addEventListener('click', () => setMatrixTab('tab-matrix-need', 'view-matrix-need'));
    }

    update(state) {
      if (!this.initialized) this.init();

      const { metrics, isDeadlocked, deadlockedProcesses, processes, resources, allocation, request, need, available } = state;

      // Update KPI Cards
      const kpiStateTitle = document.getElementById('kpi-state-title');
      const kpiStateDesc = document.getElementById('kpi-state-desc');
      const kpiStateIndicator = document.getElementById('kpi-state-indicator');

      if (kpiStateTitle) {
        if (isDeadlocked) {
          kpiStateTitle.textContent = 'DEADLOCK CANDIDATES';
          kpiStateTitle.className = 'font-headline-sm text-headline-sm text-error';
          kpiStateDesc.textContent = `Candidate set D = { ${deadlockedProcesses.join(', ')} }`;
          if (kpiStateIndicator) {
            kpiStateIndicator.className = 'px-space-xs py-0.5 rounded bg-error-container text-on-error-container font-label-xs text-label-xs font-semibold flex items-center gap-1';
            kpiStateIndicator.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-error animate-pulse"></span> IRREDUCIBLE';
          }
        } else {
          kpiStateTitle.textContent = 'SYSTEM SAFE';
          kpiStateTitle.className = 'font-headline-sm text-headline-sm text-tertiary';
          kpiStateDesc.textContent = 'No cyclic dependencies detected. Graph completely reducible.';
          if (kpiStateIndicator) {
            kpiStateIndicator.className = 'px-space-xs py-0.5 rounded bg-tertiary-fixed-dim/20 text-tertiary font-label-xs text-label-xs font-semibold flex items-center gap-1';
            kpiStateIndicator.innerHTML = '<span class="w-1.5 h-1.5 rounded-full bg-tertiary animate-pulse"></span> SAFE STATE';
          }
        }
      }

      // KPI: Utilization
      const kpiUtilVal = document.getElementById('kpi-util-val');
      const kpiUtilDesc = document.getElementById('kpi-util-desc');
      const kpiUtilBar = document.getElementById('kpi-util-bar');
      if (kpiUtilVal) kpiUtilVal.textContent = `${metrics.utilizationPct}%`;
      if (kpiUtilDesc) kpiUtilDesc.textContent = `${metrics.totalAllocated} / ${metrics.totalCapacity} units allocated`;
      if (kpiUtilBar) kpiUtilBar.style.width = `${Math.min(100, metrics.utilizationPct)}%`;

      // KPI: Active Threads
      const kpiThreadsVal = document.getElementById('kpi-threads-val');
      const kpiThreadsDesc = document.getElementById('kpi-threads-desc');
      if (kpiThreadsVal) kpiThreadsVal.textContent = `${metrics.totalProcesses} Processes`;
      if (kpiThreadsDesc) kpiThreadsDesc.textContent = `${metrics.runningProcesses} Running • ${metrics.waitingProcesses} Waiting • ${metrics.deadlockedProcessesCount} Deadlocked`;

      // KPI: Contention / Outstanding Requests
      const kpiReqVal = document.getElementById('kpi-requests-val');
      const kpiReqDesc = document.getElementById('kpi-requests-desc');
      if (kpiReqVal) kpiReqVal.textContent = `${metrics.totalRequested} Units`;
      if (kpiReqDesc) kpiReqDesc.textContent = metrics.totalRequested > 0 ? 'Pending resource grants' : 'Zero ungranted requests';

      // Render RAG
      if (this.graph) {
        this.graph.render(state);
      }

      // Render Available Vector
      const availContainer = document.getElementById('dashboard-available-vector');
      if (availContainer) {
        if (!resources || resources.length === 0) {
          availContainer.innerHTML = `<span class="text-outline text-label-xs italic">No resource classes defined</span>`;
        } else {
          availContainer.innerHTML = resources.map((r, idx) => `
            <div class="flex flex-col items-center bg-surface-container-low px-space-md py-space-xs rounded">
              <span class="font-mono text-label-xs text-outline font-semibold">${r.id}</span>
              <span class="font-mono text-headline-sm font-bold text-primary">${(available && available[idx]) || 0}</span>
              <span class="font-sans text-[10px] text-on-surface-variant truncate max-w-[80px]">${r.name}</span>
            </div>
          `).join('');
        }
      }

      // Render Allocation Matrix Table
      this._renderMatrixTable('table-matrix-alloc', processes, resources, allocation, 'alloc');
      // Render Request Matrix Table
      this._renderMatrixTable('table-matrix-req', processes, resources, request, 'req');
      // Render Need Matrix Table
      this._renderMatrixTable('table-matrix-need', processes, resources, need, 'need');
    }

    _renderMatrixTable(tableId, processes, resources, matrixData, type) {
      const container = document.getElementById(tableId);
      if (!container) return;

      if ((!processes || processes.length === 0) && (!resources || resources.length === 0)) {
        container.innerHTML = `<div class="p-space-lg text-center text-outline text-label-xs italic">No matrix data available. Manual / Custom Sandbox is currently empty.</div>`;
        return;
      }

      let html = `
        <table class="w-full text-left font-label-xs text-label-xs border-collapse">
          <thead>
            <tr class="bg-surface-container text-on-surface font-semibold uppercase">
              <th class="py-space-xs px-space-sm border-b border-surface-container-high">Process</th>
      `;

      resources.forEach(r => {
        html += `<th class="py-space-xs px-space-sm border-b border-surface-container-high font-mono text-center">${r.id}</th>`;
      });
      html += `<th class="py-space-xs px-space-sm border-b border-surface-container-high text-right">Sum</th></tr></thead><tbody class="divide-y divide-surface-container-high/40 font-mono">`;

      processes.forEach((p, pIdx) => {
        const rowData = matrixData[pIdx] || [];
        const rowSum = rowData.reduce((a, b) => a + b, 0);
        const isTerm = p.state === 'TERMINATED';
        const isDeadlock = p.state === 'DEADLOCKED';

        let pBadgeClass = 'text-primary font-semibold';
        if (isDeadlock) pBadgeClass = 'text-error font-bold';
        if (isTerm) pBadgeClass = 'text-outline line-through';

        html += `<tr class="hover:bg-surface-container-low/50">
          <td class="py-space-xs px-space-sm ${pBadgeClass}">
            ${p.id} <span class="font-sans font-normal text-[10px] text-outline">(${p.name})</span>
          </td>`;

        resources.forEach((_, rIdx) => {
          const val = rowData[rIdx] || 0;
          let cellClass = 'text-center text-outline';
          if (val > 0) {
            cellClass = (type === 'alloc') ? 'text-center text-tertiary font-bold' : (type === 'req' ? 'text-center text-error font-bold' : 'text-center text-primary font-bold');
          }
          html += `<td class="py-space-xs px-space-sm ${cellClass}">${val}</td>`;
        });

        html += `<td class="py-space-xs px-space-sm text-right font-bold text-on-surface">${rowSum}</td></tr>`;
      });

      html += `</tbody></table>`;
      container.innerHTML = html;
    }
  }

  window.DashboardScreen = new DashboardScreen();

})(window);
