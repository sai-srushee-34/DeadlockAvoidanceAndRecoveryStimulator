# Project Structure & Architecture Guide

## Overview

The **Operating Systems Deadlock Prevention and Recovery System** is organized into a modular, professional, and beginner-friendly repository structure adhering to modern C++ best practices.

---

## Directory Hierarchy

```text
deadlock_prevention/
│
├── README.md                   # Comprehensive project overview and quickstart guide
├── CMakeLists.txt              # Root build configuration for core library and executables
├── .gitignore                  # Ignores build artifacts, executables, and IDE files
├── build.bat                   # Automated Windows build script using MinGW GCC
├── run_tests.bat               # Automated test runner for all 47 tests
├── start_server.bat            # Launches native C++ HTTP server & opens frontend UI
├── start_frontend.bat          # Python HTTP server for standalone frontend debugging
│
├── app/                        # Application entry points (main routines only)
│   ├── main.cpp                # Interactive terminal CLI simulator
│   └── server_main.cpp         # Native HTTP REST API bridge & static web server
│
├── include/                    # Public C++ header files
│   ├── core/                   # State models and Resource Manager
│   │   ├── Types.hpp           # Data structures (Process, Resource, Request, Matrix)
│   │   └── ResourceManager.hpp # Allocation, release, and request tracking engine
│   ├── prevention/             # Deadlock Prevention policies
│   │   └── PreventionEngine.hpp# Hold-and-Wait & Circular-Wait ordering policies
│   ├── detection/              # Deadlock Detection algorithms
│   │   └── DetectionEngine.hpp # Multi-instance Banker's detection algorithm
│   ├── recovery/               # Deadlock Recovery strategies
│   │   └── RecoveryEngine.hpp  # Process termination heuristics & victim selection
│   └── bridge/                 # REST API, JSON, and HTTP bridge
│       ├── Json.hpp            # Lightweight zero-dependency JSON parser & serializer
│       ├── HttpServer.hpp      # High-performance Winsock HTTP/1.1 web server
│       ├── StateSerializer.hpp # System state to JSON serialization
│       ├── SimulationService.hpp # High-level thread-safe simulation facade
│       └── ApiRouter.hpp       # REST API endpoint registration and dispatch
│
├── src/                        # C++ implementation files (.cpp)
│   ├── core/
│   │   └── ResourceManager.cpp # Resource tracking & validation logic
│   ├── prevention/
│   │   └── PreventionEngine.cpp# Prevention algorithm implementations
│   ├── detection/
│   │   └── DetectionEngine.cpp # Multiple-instance detection implementation
│   ├── recovery/
│   │   └── RecoveryEngine.cpp  # Victim termination & recovery loop
│   └── bridge/
│       ├── HttpServer.cpp      # HTTP request handling & static asset server
│       ├── StateSerializer.cpp # JSON transformation for frontend consumption
│       ├── SimulationService.cpp# Scenario preloading & mutation management
│       └── ApiRouter.cpp       # HTTP routing to simulation service actions
│
├── tests/                      # Automated test suite
│   ├── CMakeLists.txt          # Test target definitions and CTest registration
│   ├── unit/                   # Unit test suites (35 tests)
│   │   ├── test_core.cpp       # Core resource management & allocation tests (13 tests)
│   │   ├── test_prevention.cpp # Hold-and-Wait & Circular-Wait tests (8 tests)
│   │   ├── test_detection.cpp  # Multi-instance Banker's detection tests (7 tests)
│   │   └── test_recovery.cpp   # Termination heuristics & rollback tests (7 tests)
│   └── integration/            # Integration test suites (12 tests)
│       └── test_api.cpp        # End-to-end REST API & mutation verification (12 tests)
│
├── docs/                       # Project documentation and specifications
│   ├── architecture.md         # Detailed architectural design and component diagrams
│   ├── requirements.md         # Course-Based Project (CBP) formal requirements
│   ├── test_plan.md            # Comprehensive test strategy and verification plan
│   ├── test-cases.md           # Granular test case specifications
│   ├── project-report-outline.md # College report template and documentation outline
│   ├── project_structure.md    # This structural and architectural guide
│   └── stitch_design/          # Google Stitch UI design export archives & assets
│
├── frontend/                   # Web application frontend (Google Stitch UI)
│   ├── index.html              # Single Page Application (SPA) dashboard entry point
│   ├── css/                    # Modular CSS styles
│   │   ├── main.css            # Base design system, typography, and layout
│   │   ├── components.css      # Badges, buttons, cards, tables, modals
│   │   └── screens.css         # Screen-specific styles
│   └── js/                     # Modular frontend JavaScript controllers
│       ├── app.js              # Application router, state dispatcher, and toast system
│       ├── api_adapter.js      # Live backend client communicating with C++ API
│       ├── graph.js            # SVG Resource Allocation Graph (RAG) visualizer
│       └── screens/            # Per-screen controllers
│           ├── dashboard.js    # Executive metrics, RAG view, quick action triggers
│           ├── processes.js    # Process table, creation form, manual deletion
│           ├── resources.js    # Resource types table, dynamic addition form
│           ├── prevention.js   # Hold-and-Wait & Circular-Wait policy controls
│           ├── detection.js    # Multi-instance detection scan & candidate list
│           ├── recovery.js     # Manual & heuristic victim selection & execution
│           └── event_log.js    # Real-time event log with filtering & clear
│
└── build/                      # Out-of-source CMake build output & compiled executables
    ├── deadlock_sim.exe        # Interactive CLI Simulator binary
    ├── deadlock_server.exe     # HTTP Bridge & API Server binary
    ├── deadlock_tests.exe      # Core Unit Tests binary
    ├── prevention_tests.exe    # Prevention Unit Tests binary
    ├── detection_tests.exe     # Detection Unit Tests binary
    ├── recovery_tests.exe      # Recovery Unit Tests binary
    └── api_tests.exe           # API Integration Tests binary
```

---

## Architectural Separation of Concerns

1. **`app/` vs `src/`**:
   - `app/` strictly houses executable entry points (`main()`). No core domain logic resides here.
   - `src/` contains all algorithmic and business logic compiled into the reusable static library `deadlock_core`.

2. **`include/` vs `src/`**:
   - `include/` exposes clean, encapsulated C++17 class interfaces with minimal transitive dependencies.
   - `src/` encapsulates internal implementation details.

3. **`tests/unit/` vs `tests/integration/`**:
   - `unit/` verifies individual mathematical algorithms and invariants (Available vector conservation, circular wait ordering, Banker's finish vector, priority-cost termination).
   - `integration/` verifies the unified system behavior, end-to-end API payload parsing, mutation handling, scenario switching, and error boundary responses.

4. **`frontend/`**:
   - Built with clean vanilla HTML5/CSS3/JavaScript without external heavyweight frameworks, allowing rapid native hosting directly from the C++ HTTP server.
