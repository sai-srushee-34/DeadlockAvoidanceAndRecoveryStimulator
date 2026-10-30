---
name: Kernel Systems Deadlock Monitor
colors:
  surface: '#f8f9ff'
  surface-dim: '#cbdbf5'
  surface-bright: '#f8f9ff'
  surface-container-lowest: '#ffffff'
  surface-container-low: '#eff4ff'
  surface-container: '#e5eeff'
  surface-container-high: '#dce9ff'
  surface-container-highest: '#d3e4fe'
  on-surface: '#0b1c30'
  on-surface-variant: '#434655'
  inverse-surface: '#213145'
  inverse-on-surface: '#eaf1ff'
  outline: '#737686'
  outline-variant: '#c3c6d7'
  surface-tint: '#0053db'
  primary: '#004ac6'
  on-primary: '#ffffff'
  primary-container: '#2563eb'
  on-primary-container: '#eeefff'
  inverse-primary: '#b4c5ff'
  secondary: '#565e74'
  on-secondary: '#ffffff'
  secondary-container: '#dae2fd'
  on-secondary-container: '#5c647a'
  tertiary: '#006243'
  on-tertiary: '#ffffff'
  tertiary-container: '#007d57'
  on-tertiary-container: '#bdffdc'
  error: '#ba1a1a'
  on-error: '#ffffff'
  error-container: '#ffdad6'
  on-error-container: '#93000a'
  primary-fixed: '#dbe1ff'
  primary-fixed-dim: '#b4c5ff'
  on-primary-fixed: '#00174b'
  on-primary-fixed-variant: '#003ea8'
  secondary-fixed: '#dae2fd'
  secondary-fixed-dim: '#bec6e0'
  on-secondary-fixed: '#131b2e'
  on-secondary-fixed-variant: '#3f465c'
  tertiary-fixed: '#85f8c4'
  tertiary-fixed-dim: '#68dba9'
  on-tertiary-fixed: '#002114'
  on-tertiary-fixed-variant: '#005137'
  background: '#f8f9ff'
  on-background: '#0b1c30'
  surface-variant: '#d3e4fe'
typography:
  headline-lg:
    fontFamily: Geist
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
    letterSpacing: -0.02em
  headline-md:
    fontFamily: Geist
    fontSize: 18px
    fontWeight: '600'
    lineHeight: 24px
    letterSpacing: -0.015em
  headline-sm:
    fontFamily: Geist
    fontSize: 15px
    fontWeight: '600'
    lineHeight: 20px
    letterSpacing: -0.01em
  body-md:
    fontFamily: Geist
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 18px
  body-sm:
    fontFamily: Geist
    fontSize: 12px
    fontWeight: '400'
    lineHeight: 16px
  label-md:
    fontFamily: JetBrains Mono
    fontSize: 12px
    fontWeight: '500'
    lineHeight: 16px
    letterSpacing: -0.01em
  label-sm:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '500'
    lineHeight: 14px
    letterSpacing: 0em
  label-xs:
    fontFamily: JetBrains Mono
    fontSize: 10px
    fontWeight: '400'
    lineHeight: 12px
    letterSpacing: 0.02em
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  gutter: 0.75rem
  gutter-desktop: 1rem
  margin: 1rem
  margin-desktop: 1.5rem
  space-xs: 0.25rem
  space-sm: 0.375rem
  space-md: 0.75rem
  space-lg: 1rem
  space-xl: 1.5rem
---

## Brand & Style

The design system embodies utilitarian precision, situational transparency, and structural control. Designed for systems engineers, kernel programmers, and computer science researchers orchestrating deadlock avoidance and recovery strategies (such as Banker's Algorithm, Wait-For Graph analysis, and preemptive resource allocation), the interface prioritizes immediate semantic parsing over aesthetic flourish. 

The stylistic framework is **Technical Minimalist / Precision Engineering**:
- High-density information architecture with strict visual grounding.
- Zero decorative embellishments: no gradient fills, no translucent blurs, and no expressive drop-shadows.
- Clear structural division defined by 1px architectural lines and deliberate micro-typography.
- Instant mechanical clarity that minimizes cognitive latency during critical deadlock resolution sequences.

## Colors

The palette is engineered for clinical legibility and instant diagnostic scanning under sustained cognitive load.

- **Primary (`#2563eb` / `#1d4ed8` hover):** Precision engineering blue. Dictates primary interactive triggers, active graph node focus, and programmatic state selection.
- **Secondary (`#0f172a`):** Deep slate foundation. Drives high-contrast data visualization axes, terminal headings, and active tab anchors.
- **Status Semantic Indicators:**
  - **Safe State (`#059669` emerald):** Concurrency paths with a proven safe execution sequence. Accompanied by `#ecfdf5` surface tinting.
  - **Waiting / Contention (`#d97706` amber):** Thread blocked awaiting mutex or resource lock. Accompanied by `#fffbeb` surface tinting.
  - **Deadlock Critical (`#e11d48` rose-red):** Circular wait cycle confirmed. Accompanied by `#fff1f2` background alerting.
- **Surfaces & Borders:** Neutral canvas layers utilize crisp off-white (`#f8fafc`) with subtle structural panels (`#ffffff`) and inner control wells (`#f1f5f9`). Gridlines and container dividers are strictly framed with `#e2e8f0`.

## Typography

Typography functions as an analytical instrument. We pair **Geist** for crisp, proportional UI labels and structure with **JetBrains Mono** for all machine-level entities, memory addresses, tabular matrices, and process allocations.

- **Monospace Usage:** Strictly enforced for all entity identifiers (`P0`, `P1`, `R0`, `R1`), thread states, vector arrays (`Allocation[i, j]`, `Need[i, j]`), hexadecimal addresses, and cycle paths.
- **Tight Verticality:** Line heights are restrained to maintain visual connection between contiguous rows in dense data grids and console feeds.
- **Micro Scales:** Body copy defaults to 13px (`body-md`), preserving dense information display across desktop workstations without sacrificing optical legibility.

## Layout & Spacing

The layout employs an engineering workbench model: a fixed-height, viewport-filling dashboard grid engineered for multi-monitor desktop environments with zero unnecessary whitespace.

- **Grid Architecture:** 12-column layout with 12px (`gutter`) to 16px (`gutter-desktop`) intervals. Standard arrangements feature a 3-column parameters/process control rail, a 6-column central topology graph/matrix canvas, and a 3-column trace inspection and telemetry drawer.
- **Structural Rhythm:** Spacing is compact and disciplined. Internal card padding is locked to `space-md` (12px), while inter-component button groupings and filter rows rely on `space-xs` (4px) and `space-sm` (6px).
- **Responsive Adaptations:** 
  - On screens `< 1024px`, the system collapses topology diagrams and matrices into stackable inspectable viewports, defaulting the navigation to segmented tabs (Topology, Matrix, Terminal Logs).

## Elevation & Depth

Visual hierarchy is communicated through structural outlines and tonal surface shifts rather than synthetic shadows:

- **Border Hierarchy:** Depth is created via a `1px solid #e2e8f0` grid framework. Nested modules switch to `#f8fafc` fills with `#cbd5e1` internal dividing rules.
- **Shadow Suppression:** Standard elevation utilizes a single flat micro-shadow (`0 1px 2px rgba(15, 23, 42, 0.04)`) exclusively to elevate floating contextual menus and tooltip modals above vector matrices.
- **Active State Depth:** Interactivity is declared via border color transitions (e.g., `#e2e8f0` to `#2563eb`) and subtle inner well fills (`#f1f5f9`), avoiding simulated 3D drop depths.

## Shapes

The design system implements a disciplined **Soft (`1`)** shape language.

- Standard buttons, inputs, data grid cells, and system alerts feature a precise `0.25rem` (4px) border radius.
- Nested panels and top-level diagnostic cards scale to `rounded-lg` (8px).
- Nodes in resource allocation graphs adopt formal computer science visual conventions: **Circles** for Processes (`P_n`) and **Rectangles** with tight 2px radii for Resources (`R_m`), containing dots for multiple resource instances.
- Pill shapes are rejected entirely to avoid casual or consumer-app connotations.

## Components

### Buttons & Trigger Controls
- **Primary:** Solid `#2563eb` fill, `#ffffff` text, 1px border (`#1d4ed8`), 28px height, 4px border radius. Used solely for deterministic execution triggers (e.g., "Run Detection Cycle", "Grant Lock").
- **Secondary / Action:** `#ffffff` background, `#0f172a` text, 1px `#e2e8f0` border. Hover changes background to `#f1f5f9` with `#cbd5e1` border.
- **Danger:** `#fff1f2` background, `#e11d48` text, 1px `#fecdd3` border. Used strictly for "Preempt Process", "Kill Thread", and "Rollback Transaction".

### Data Tables & Allocation Matrices
- **Grid Layout:** Fixed column widths; horizontal and vertical hairline dividers (`#e2e8f0`).
- **Headers:** Monospace 10px uppercase `#64748b` backgrounded with `#f8fafc`, height 24px.
- **Cells:** Tabular numerical alignment, 28px row height, zero line-wrap, monospace font (`JetBrains Mono`). Hovering a row highlights the corresponding node in the SVG topology graph.

### Status Indicators & Badges
- **Format:** Tight rectangular badges (18px height, 4px radius) with an explicit 6px status dot alongside monospace text.
  - **Safe:** `#ecfdf5` fill, `#059669` text, `#a7f3d0` border.
  - **Contended:** `#fffbeb` fill, `#d97706` text, `#fde68a` border.
  - **Deadlock:** `#fff1f2` fill, `#e11d48` text, `#fecdd3` border with a subtle SVG cycle icon.

### Form Inputs & Knobs
- **Input Fields:** 28px height, `#ffffff` background, `#e2e8f0` border, `Geist Mono` 12px for process and cycle parameters. Active focus applies a crisp `0 0 0 1px #2563eb`.

### SVG Topology & Wait-For Graphs
- **Canvas:** Clean `#f8fafc` backdrop overlaid with a 16px dot-grid `#cbd5e1`.
- **Edges:** Directed 1.5px arrows (`#64748b`). Active wait arcs pulse or switch to `#d97706`. Detected cycle arcs turn `#e11d48` with a 2px stroke width.
- **Nodes:** Process nodes are 36px circles with crisp 1.5px slate borders; Resource nodes are 32x32px rounded rectangles displaying instance dots.