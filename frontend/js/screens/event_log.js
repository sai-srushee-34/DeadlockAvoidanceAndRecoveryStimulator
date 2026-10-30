/**
 * Kernel Systems Deadlock Monitor - Event Log Screen Controller
 */

(function (window) {
  'use strict';

  class EventLogScreen {
    constructor() {
      this.initialized = false;
      this.categoryFilter = 'ALL';
      this.severityFilter = 'ALL';
      this.searchQuery = '';
    }

    init() {
      if (this.initialized) return;
      this.initialized = true;

      const catSelect = document.getElementById('log-category-filter');
      const sevSelect = document.getElementById('log-severity-filter');
      const searchInput = document.getElementById('log-search-input');
      const btnClear = document.getElementById('btn-clear-log');
      const btnExport = document.getElementById('btn-export-log');

      if (catSelect) {
        catSelect.addEventListener('change', (e) => {
          this.categoryFilter = e.target.value;
          this._render();
        });
      }

      if (sevSelect) {
        sevSelect.addEventListener('change', (e) => {
          this.severityFilter = e.target.value;
          this._render();
        });
      }

      if (searchInput) {
        searchInput.addEventListener('input', (e) => {
          this.searchQuery = e.target.value.toLowerCase().trim();
          this._render();
        });
      }

      if (btnClear) {
        btnClear.addEventListener('click', async () => {
          try {
            await window.SimulationAdapter.clearEventLog();
            window.App.showToast('Audit event log cleared.', 'info');
          } catch (err) {
            window.App.showToast(`Failed to clear log: ${err.message}`, 'error');
          }
        });
      }

      if (btnExport) {
        btnExport.addEventListener('click', () => {
          this._exportTelemetry();
        });
      }
    }

    update(state) {
      if (!this.initialized) this.init();
      this._render();
    }

    _render() {
      const tbody = document.getElementById('table-event-log-body');
      if (!tbody) return;

      const logs = window.MockSimulationAdapter.getEventLog();

      const filtered = logs.filter(entry => {
        if (this.categoryFilter !== 'ALL' && entry.category !== this.categoryFilter) return false;
        if (this.severityFilter !== 'ALL' && entry.severity !== this.severityFilter) return false;
        if (this.searchQuery) {
          const matchMsg = entry.message.toLowerCase().includes(this.searchQuery);
          const matchCat = entry.category.toLowerCase().includes(this.searchQuery);
          return matchMsg || matchCat;
        }
        return true;
      });

      // Update counter
      const counter = document.getElementById('log-entries-count');
      if (counter) {
        counter.textContent = `${filtered.length} of ${logs.length} events`;
      }

      if (filtered.length === 0) {
        tbody.innerHTML = `<tr><td colspan="4" class="p-space-lg text-center text-outline">No log entries matching the selected criteria.</td></tr>`;
        return;
      }

      tbody.innerHTML = filtered.map(item => {
        let sevBadge = 'bg-surface-container-high text-on-surface';
        if (item.severity === 'ERROR') sevBadge = 'bg-error-container text-on-error-container font-bold';
        if (item.severity === 'WARNING') sevBadge = 'bg-amber-100 text-amber-900 font-bold';
        if (item.severity === 'SUCCESS') sevBadge = 'bg-tertiary-fixed-dim/20 text-tertiary font-bold';
        if (item.severity === 'INFO') sevBadge = 'bg-blue-50 text-blue-700';

        let catBadge = 'bg-surface-container-low text-on-surface-variant';
        if (item.category === 'DETECTION') catBadge = 'bg-purple-50 text-purple-700';
        if (item.category === 'RECOVERY') catBadge = 'bg-rose-50 text-rose-700';
        if (item.category === 'PREVENTION') catBadge = 'bg-emerald-50 text-emerald-700';
        if (item.category === 'ALLOCATION') catBadge = 'bg-cyan-50 text-cyan-700';

        return `
          <tr class="hover:bg-surface-container-lowest transition-colors font-mono text-label-xs">
            <td class="py-space-sm px-space-md text-outline whitespace-nowrap">${item.timestamp}</td>
            <td class="py-space-sm px-space-md">
              <span class="px-space-xs py-0.5 rounded text-[10px] font-semibold uppercase ${catBadge}">
                ${item.category}
              </span>
            </td>
            <td class="py-space-sm px-space-md">
              <span class="px-space-xs py-0.5 rounded text-[10px] font-semibold ${sevBadge}">
                ${item.severity}
              </span>
            </td>
            <td class="py-space-sm px-space-md text-on-surface font-sans text-body-sm leading-snug">
              ${item.message}
            </td>
          </tr>
        `;
      }).join('');
    }

    _exportTelemetry() {
      const logs = window.MockSimulationAdapter.getEventLog();
      const state = window.MockSimulationAdapter.getState();
      const exportData = {
        title: 'Kernel Systems Deadlock Simulator - Audit Telemetry',
        exportedAt: new Date().toISOString(),
        scenario: state.scenarioName,
        metrics: state.metrics,
        state: state,
        eventLog: logs
      };

      const dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(exportData, null, 2));
      const downloadAnchor = document.createElement('a');
      downloadAnchor.setAttribute('href', dataStr);
      downloadAnchor.setAttribute('download', `deadlock_telemetry_${Date.now()}.json`);
      document.body.appendChild(downloadAnchor);
      downloadAnchor.click();
      downloadAnchor.remove();

      window.App.showToast('Telemetry trace exported successfully.', 'success');
    }
  }

  window.EventLogScreen = new EventLogScreen();

})(window);
