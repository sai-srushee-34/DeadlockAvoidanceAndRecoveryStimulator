/**
 * Kernel Systems Deadlock Monitor - SVG Resource-Allocation Graph (RAG)
 * 
 * Bipartite directed graph visualization:
 * - Processes: Circles (P0, P1, ...) with live status borders (RUNNING, WAITING, DEADLOCKED)
 * - Resources: Rounded rectangles (R0, R1, ...) with internal instance dots (allocated vs available)
 * - Allocation Edges: R -> P (Solid, Emerald/Blue, Arrow pointing to Process)
 * - Request Edges: P -> R (Dashed, Amber/Crimson, Arrow pointing to Resource)
 * - Controls: Zoom (+/-), Reset pan/zoom, Filter edges (All, Allocation, Request)
 */

(function (window) {
  'use strict';

  class ResourceAllocationGraph {
    constructor(containerId) {
      this.container = document.getElementById(containerId);
      if (!this.container) return;

      this.svg = null;
      this.zoomGroup = null;
      this.scale = 1.0;
      this.panX = 0;
      this.panY = 0;
      this.isPanning = false;
      this.startX = 0;
      this.startY = 0;
      this.edgeFilter = 'all'; // 'all', 'alloc', 'req'
      this.selectedNode = null;

      this._init();
    }

    _init() {
      // Clear container
      this.container.innerHTML = '';

      // Create main SVG
      const width = this.container.clientWidth || 700;
      const height = this.container.clientHeight || 420;

      this.svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      this.svg.setAttribute('class', 'rag-canvas w-full h-full');
      this.svg.setAttribute('viewBox', `0 0 ${width} ${height}`);

      // Defs for arrowheads and filters
      const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');

      // Allocation Arrow Marker (R -> P)
      const markerAlloc = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
      markerAlloc.setAttribute('id', 'arrow-alloc');
      markerAlloc.setAttribute('viewBox', '0 0 10 10');
      markerAlloc.setAttribute('refX', '8');
      markerAlloc.setAttribute('refY', '5');
      markerAlloc.setAttribute('markerWidth', '6');
      markerAlloc.setAttribute('markerHeight', '6');
      markerAlloc.setAttribute('orient', 'auto-start-reverse');
      markerAlloc.innerHTML = '<path d="M 0 1 L 10 5 L 0 9 z" fill="#007d57" />';
      defs.appendChild(markerAlloc);

      // Request Arrow Marker (P -> R)
      const markerReq = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
      markerReq.setAttribute('id', 'arrow-req');
      markerReq.setAttribute('viewBox', '0 0 10 10');
      markerReq.setAttribute('refX', '8');
      markerReq.setAttribute('refY', '5');
      markerReq.setAttribute('markerWidth', '6');
      markerReq.setAttribute('markerHeight', '6');
      markerReq.setAttribute('orient', 'auto-start-reverse');
      markerReq.innerHTML = '<path d="M 0 1 L 10 5 L 0 9 z" fill="#e11d48" />';
      defs.appendChild(markerReq);

      this.svg.appendChild(defs);

      // Zoom/Pan group wrapper
      this.zoomGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      this.zoomGroup.setAttribute('id', 'rag-zoom-group');
      this.svg.appendChild(this.zoomGroup);

      this.container.appendChild(this.svg);

      // Bind pan/zoom events
      this._bindInteractions();
    }

    _bindInteractions() {
      this.svg.addEventListener('mousedown', (e) => {
        if (e.target.closest('.rag-node')) return; // Allow node clicks
        this.isPanning = true;
        this.startX = e.clientX - this.panX;
        this.startY = e.clientY - this.panY;
      });

      window.addEventListener('mousemove', (e) => {
        if (!this.isPanning) return;
        this.panX = e.clientX - this.startX;
        this.panY = e.clientY - this.startY;
        this._updateTransform();
      });

      window.addEventListener('mouseup', () => {
        this.isPanning = false;
      });

      this.svg.addEventListener('wheel', (e) => {
        e.preventDefault();
        const delta = e.deltaY > 0 ? -0.1 : 0.1;
        this.scale = Math.min(Math.max(0.4, this.scale + delta), 2.5);
        this._updateTransform();
      });
    }

    _updateTransform() {
      if (this.zoomGroup) {
        this.zoomGroup.setAttribute('transform', `translate(${this.panX}, ${this.panY}) scale(${this.scale})`);
      }
    }

    zoomIn() {
      this.scale = Math.min(2.5, this.scale + 0.15);
      this._updateTransform();
    }

    zoomOut() {
      this.scale = Math.max(0.4, this.scale - 0.15);
      this._updateTransform();
    }

    resetView() {
      this.scale = 1.0;
      this.panX = 0;
      this.panY = 0;
      this._updateTransform();
    }

    setFilter(filter) {
      this.edgeFilter = filter;
      const allocEdges = this.zoomGroup.querySelectorAll('.rag-edge-alloc');
      const reqEdges = this.zoomGroup.querySelectorAll('.rag-edge-req');

      allocEdges.forEach(el => {
        el.style.display = (filter === 'all' || filter === 'alloc') ? '' : 'none';
      });

      reqEdges.forEach(el => {
        el.style.display = (filter === 'all' || filter === 'req') ? '' : 'none';
      });
    }

    /**
     * Render graph from system state
     */
    render(state) {
      if (!this.zoomGroup) return;

      this.zoomGroup.innerHTML = '';
      const { processes, resources, allocation, request } = state;

      const width = this.container.clientWidth || 700;
      const height = this.container.clientHeight || 420;

      // Coordinate calculations
      const pCount = (processes || []).length;
      const rCount = (resources || []).length;

      if (pCount === 0 && rCount === 0) {
        const emptyGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
        emptyGroup.innerHTML = `
          <text x="${width/2}" y="${height/2 - 10}" text-anchor="middle" font-family="Inter, sans-serif" font-size="14px" font-weight="600" fill="#71787e">
            Manual / Custom Sandbox Active (Blank Canvas)
          </text>
          <text x="${width/2}" y="${height/2 + 16}" text-anchor="middle" font-family="Inter, sans-serif" font-size="12px" fill="#8b9298">
            Create custom processes and resources using the navigation tabs to visualize the Resource-Allocation Graph.
          </text>
        `;
        this.zoomGroup.appendChild(emptyGroup);
        return;
      }

      const pX = Math.max(120, width * 0.22);
      const rX = Math.min(width - 120, width * 0.78);

      const pSpacing = pCount > 1 ? (height - 120) / (pCount - 1) : 0;
      const pStartY = pCount > 1 ? 60 : height / 2;

      const rSpacing = rCount > 1 ? (height - 120) / (rCount - 1) : 0;
      const rStartY = rCount > 1 ? 60 : height / 2;

      const pPositions = {};
      const rPositions = {};

      processes.forEach((p, idx) => {
        const y = pCount === 1 ? height / 2 : pStartY + idx * pSpacing;
        pPositions[p.id] = { x: pX, y: y, data: p };
      });

      resources.forEach((r, idx) => {
        const y = rCount === 1 ? height / 2 : rStartY + idx * rSpacing;
        rPositions[r.id] = { x: rX, y: y, data: r };
      });

      // Layer 1: Edges Group
      const edgesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      edgesGroup.setAttribute('id', 'edges-layer');

      // 1. Allocation Edges: Resource -> Process (R -> P)
      resources.forEach((r, rIdx) => {
        processes.forEach((p, pIdx) => {
          const count = allocation[pIdx] ? allocation[pIdx][rIdx] : 0;
          if (count > 0 && p.state !== 'TERMINATED') {
            const edge = this._createEdge(
              rPositions[r.id].x - 30, rPositions[r.id].y,
              pPositions[p.id].x + 24, pPositions[p.id].y,
              'alloc',
              `Alloc: ${count}x`,
              `alloc_${r.id}_${p.id}`,
              r.id,
              p.id
            );
            edgesGroup.appendChild(edge);
          }
        });
      });

      // 2. Request Edges: Process -> Resource (P -> R)
      processes.forEach((p, pIdx) => {
        resources.forEach((r, rIdx) => {
          const count = request[pIdx] ? request[pIdx][rIdx] : 0;
          if (count > 0 && p.state !== 'TERMINATED') {
            const isDeadlocked = p.state === 'DEADLOCKED';
            const edge = this._createEdge(
              pPositions[p.id].x + 24, pPositions[p.id].y,
              rPositions[r.id].x - 30, rPositions[r.id].y,
              'req',
              `Req: ${count}x`,
              `req_${p.id}_${r.id}`,
              p.id,
              r.id,
              isDeadlocked
            );
            edgesGroup.appendChild(edge);
          }
        });
      });

      this.zoomGroup.appendChild(edgesGroup);

      // Layer 2: Process Nodes Group
      const nodesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      nodesGroup.setAttribute('id', 'nodes-layer');

      processes.forEach(p => {
        const pos = pPositions[p.id];
        const node = this._createProcessNode(pos.x, pos.y, p);
        nodesGroup.appendChild(node);
      });

      // Layer 3: Resource Nodes Group
      resources.forEach((r, rIdx) => {
        const pos = rPositions[r.id];
        let heldCount = 0;
        processes.forEach((p, pIdx) => {
          if (p.state !== 'TERMINATED' && allocation[pIdx]) {
            heldCount += allocation[pIdx][rIdx] || 0;
          }
        });
        const availCount = Math.max(0, r.capacity - heldCount);
        const node = this._createResourceNode(pos.x, pos.y, r, heldCount, availCount);
        nodesGroup.appendChild(node);
      });

      this.zoomGroup.appendChild(nodesGroup);

      // Apply current filter
      this.setFilter(this.edgeFilter);
    }

    _createEdge(x1, y1, x2, y2, type, label, id, sourceId, targetId, isDeadlocked = false) {
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('class', `rag-edge rag-edge-${type} transition-opacity duration-200`);
      g.setAttribute('data-id', id);
      g.setAttribute('data-source', sourceId);
      g.setAttribute('data-target', targetId);

      // Quadratic bezier curve for curved clarity
      const midX = (x1 + x2) / 2;
      const curvature = (type === 'alloc') ? -25 : 25;
      const midY = (y1 + y2) / 2 + curvature;

      const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      path.setAttribute('d', `M ${x1} ${y1} Q ${midX} ${midY} ${x2} ${y2}`);
      path.setAttribute('fill', 'none');

      if (type === 'alloc') {
        path.setAttribute('stroke', '#007d57');
        path.setAttribute('stroke-width', '2');
        path.setAttribute('marker-end', 'url(#arrow-alloc)');
      } else {
        path.setAttribute('stroke', isDeadlocked ? '#ba1a1a' : '#d97706');
        path.setAttribute('stroke-width', isDeadlocked ? '2.5' : '1.75');
        path.setAttribute('stroke-dasharray', '4 3');
        path.setAttribute('marker-end', 'url(#arrow-req)');
      }
      g.appendChild(path);

      // Edge badge text background & label
      const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      text.setAttribute('x', midX);
      text.setAttribute('y', midY - 6);
      text.setAttribute('text-anchor', 'middle');
      text.setAttribute('font-family', 'JetBrains Mono, monospace');
      text.setAttribute('font-size', '10px');
      text.setAttribute('font-weight', '600');
      text.setAttribute('fill', type === 'alloc' ? '#006243' : (isDeadlocked ? '#ba1a1a' : '#b45309'));
      text.textContent = label;
      g.appendChild(text);

      return g;
    }

    _createProcessNode(x, y, proc) {
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('class', `rag-node rag-node-process cursor-pointer`);
      g.setAttribute('data-node-id', proc.id);
      g.setAttribute('transform', `translate(${x}, ${y})`);

      // Outer Circle
      const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      circle.setAttribute('r', '22');
      circle.setAttribute('class', `rag-node-process ${proc.state.toLowerCase()}`);
      g.appendChild(circle);

      // Label (P0, P1)
      const textId = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textId.setAttribute('y', '4');
      textId.setAttribute('text-anchor', 'middle');
      textId.setAttribute('class', 'rag-node-label font-mono font-bold fill-current');
      textId.textContent = proc.id;
      g.appendChild(textId);

      // Sub-label below (Name/PID)
      const textSub = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textSub.setAttribute('y', '36');
      textSub.setAttribute('text-anchor', 'middle');
      textSub.setAttribute('class', 'font-sans text-[11px] fill-slate-600 font-medium');
      textSub.textContent = proc.name;
      g.appendChild(textSub);

      // State Pill badge
      const badgeG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      badgeG.setAttribute('transform', 'translate(0, -28)');

      const badgeText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      badgeText.setAttribute('text-anchor', 'middle');
      badgeText.setAttribute('font-family', 'JetBrains Mono');
      badgeText.setAttribute('font-size', '8px');
      badgeText.setAttribute('font-weight', '700');

      if (proc.state === 'DEADLOCKED') {
        badgeText.setAttribute('fill', '#ba1a1a');
        badgeText.textContent = 'DEADLOCKED';
      } else if (proc.state === 'WAITING') {
        badgeText.setAttribute('fill', '#d97706');
        badgeText.textContent = 'WAITING';
      } else if (proc.state === 'RUNNING') {
        badgeText.setAttribute('fill', '#007d57');
        badgeText.textContent = 'RUNNING';
      } else {
        badgeText.setAttribute('fill', '#64748b');
        badgeText.textContent = 'TERMINATED';
      }

      badgeG.appendChild(badgeText);
      g.appendChild(badgeG);

      // Highlight on hover
      g.addEventListener('mouseenter', () => this._highlightNode(proc.id));
      g.addEventListener('mouseleave', () => this._clearHighlight());

      return g;
    }

    _createResourceNode(x, y, res, heldCount, availCount) {
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('class', 'rag-node rag-node-resource cursor-pointer');
      g.setAttribute('data-node-id', res.id);
      g.setAttribute('transform', `translate(${x - 30}, ${y - 25})`);

      // Rectangle
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('width', '60');
      rect.setAttribute('height', '50');
      rect.setAttribute('rx', '6');
      rect.setAttribute('class', 'rag-node-resource fill-white stroke-slate-700 stroke-2 shadow-sm');
      g.appendChild(rect);

      // Label (R0)
      const textId = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textId.setAttribute('x', '30');
      textId.setAttribute('y', '16');
      textId.setAttribute('text-anchor', 'middle');
      textId.setAttribute('class', 'font-mono text-[11px] font-bold fill-primary');
      textId.textContent = res.id;
      g.appendChild(textId);

      // Instances visual indicator (dots)
      const totalDots = Math.min(res.capacity, 12);
      const dotsGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      dotsGroup.setAttribute('transform', 'translate(10, 24)');

      for (let i = 0; i < totalDots; i++) {
        const row = Math.floor(i / 6);
        const col = i % 6;
        const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        dot.setAttribute('cx', col * 7);
        dot.setAttribute('cy', row * 7);
        dot.setAttribute('r', '2.5');

        if (i < heldCount) {
          dot.setAttribute('class', 'fill-primary');
        } else {
          dot.setAttribute('class', 'fill-slate-200 stroke-slate-400 stroke-1');
        }
        dotsGroup.appendChild(dot);
      }
      g.appendChild(dotsGroup);

      // Sub-label below
      const textSub = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textSub.setAttribute('x', '30');
      textSub.setAttribute('y', '63');
      textSub.setAttribute('text-anchor', 'middle');
      textSub.setAttribute('class', 'font-sans text-[11px] fill-slate-600 font-medium');
      textSub.textContent = res.name;
      g.appendChild(textSub);

      // Capacity Badge
      const textCap = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textCap.setAttribute('x', '30');
      textCap.setAttribute('y', '76');
      textCap.setAttribute('text-anchor', 'middle');
      textCap.setAttribute('class', 'font-mono text-[9px] fill-slate-400 font-semibold');
      textCap.textContent = `${availCount}/${res.capacity} Avail`;
      g.appendChild(textCap);

      // Highlight on hover
      g.addEventListener('mouseenter', () => this._highlightNode(res.id));
      g.addEventListener('mouseleave', () => this._clearHighlight());

      return g;
    }

    _highlightNode(nodeId) {
      const edges = this.zoomGroup.querySelectorAll('.rag-edge');
      edges.forEach(edge => {
        const s = edge.getAttribute('data-source');
        const t = edge.getAttribute('data-target');
        if (s === nodeId || t === nodeId) {
          edge.style.opacity = '1.0';
          edge.querySelector('path').setAttribute('stroke-width', '3');
        } else {
          edge.style.opacity = '0.2';
        }
      });
    }

    _clearHighlight() {
      const edges = this.zoomGroup.querySelectorAll('.rag-edge');
      edges.forEach(edge => {
        edge.style.opacity = '1.0';
        const type = edge.classList.contains('rag-edge-alloc') ? 'alloc' : 'req';
        edge.querySelector('path').setAttribute('stroke-width', type === 'alloc' ? '2' : '1.75');
      });
    }
  }

  window.ResourceAllocationGraph = ResourceAllocationGraph;

})(window);
