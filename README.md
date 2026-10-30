# Operating Systems Deadlock Prevention and Recovery System

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/Build-CMake%203.16+-green.svg)](CMakeLists.txt)
[![Tests](https://img.shields.io/badge/Tests-47%20Passed%2C%200%20Failed-brightgreen.svg)](tests/)
[![Architecture](https://img.shields.io/badge/Architecture-Modular%20Layered-orange.svg)](docs/architecture.md)

An interactive, educational Operating Systems simulation platform implementing multi-instance resource allocation, Coffman condition deadlock prevention policies, Shoshani-Coffman matrix reduction deadlock detection, heuristic process termination recovery, a native C++ HTTP REST API bridge, and a web-based Resource Allocation Graph (RAG) dashboard.

---

## Table of Contents

1. [Project Name and Purpose](#1-project-name-and-purpose)
2. [Features Actually Implemented](#2-features-actually-implemented)
3. [Module Explanations](#3-module-explanations)
4. [Folder Structure Overview](#4-folder-structure-overview)
5. [Required Tools and Dependencies](#5-required-tools-and-dependencies)
6. [Configure and Build Commands](#6-configure-and-build-commands)
7. [Running the Simulator and API Server](#7-running-the-simulator-and-api-server)
8. [Running Automated Tests](#8-running-automated-tests)
9. [Cleaning Build Artifacts](#9-cleaning-build-artifacts)
10. [Current Limitations and Planned Features](#10-current-limitations-and-planned-features)

---

## 1. Project Name and Purpose

**Project Name:** Operating Systems Deadlock Prevention and Recovery System  
**Course Context:** Course-Based Project (CBP) for Operating Systems & Systems Programming.

### Purpose
In modern multiprogrammed operating systems, concurrent processes compete for limited shared resources (CPUs, memory blocks, I/O devices, mutex locks). When multiple processes hold resources while waiting for resources held by others in a closed cycle, a **deadlock** occurs, permanently halting system execution.

This project provides an end-to-end, reproducible simulation testbed to explore, visualize, and benchmark:
- How resource competition leads to deadlock under unrestricted allocation.
- How protocol constraints (**Deadlock Prevention**) mathematically eliminate conditions required for deadlock.
- How **Deadlock Detection** algorithms systematically discover deadlocked subsets using matrix reduction on multi-instance resources.
- How **Deadlock Recovery** resolves deadlocks via cost-optimal victim process termination.
- How a real-time, interactive **Web Dashboard** connects directly to the C++ core engine to monitor system invariants.

---

## 2. Features Actually Implemented

### Core Resource Management Engine
- **Multi-Instance Resource Model:** Arbitrary resource types ($R_0, R_1, \dots, R_{m-1}$) with discrete instance capacities.
- **Dynamic Process Tracking:** Processes ($P_0, P_1, \dots, P_{n-1}$) with attributes including state (Running, Waiting, Terminated), priority (1–10), termination cost, and maximum demand vectors.
- **Mathematical Invariant Conservation:** Enforces $\mathbf{Available} + \sum \mathbf{Allocation}_i = \mathbf{Total}$ on every allocation, request, release, and termination.
- **Pending Request Queue:** Threads requesting unavailable instances enter a waiting queue and automatically awaken when resources are freed.

### Deadlock Prevention Policies (Phase 3)
- **Policy A (Hold-and-Wait Invalidation):** All-or-nothing allocation protocol. A process must claim its full resource requirement upfront before execution begins. If any instance is unavailable, partial allocations are prohibited, preventing the hold-and-wait condition.
- **Policy B (Circular-Wait Invalidation):** Strict total ordering $F: R \to \mathbb{N}$. Processes can only request resource types in strictly increasing order of their numerical identifier. Backward or out-of-order requests are rejected, mathematically precluding circular wait chains.

### Multi-Instance Deadlock Detection (Phase 4)
- **Shoshani-Coffman Algorithm:** Standard multi-instance matrix reduction using $\mathbf{Work}$ and $\mathbf{Finish}$ vectors.
- **Zero-Allocation Handling:** Processes holding zero resources are marked finished initially since they cannot cause deadlocks for others.
- **Deadlock Candidate Set:** Identifies the precise subset of processes $\mathcal{D} \subseteq \mathcal{P}$ locked in mutual wait.

### Deadlock Recovery Engine (Phase 5)
- **Process Termination:** Terminates a deadlocked process, reclaims all held resources back to $\mathbf{Available}$, cancels outstanding requests, and re-triggers detection.
- **Victim Selection Strategies:**
  - *Manual Selection:* Direct operator designation of victim process ID.
  - *Lowest Priority:* Selects process with lowest priority number.
  - *Lowest Termination Cost:* Selects process with lowest financial/computation penalty score.

### Bridge and Native HTTP REST API (Phase 7)
- **Zero-Dependency Winsock Server:** Custom HTTP/1.1 server running on Windows sockets (`ws2_32`).
- **REST Endpoints:** JSON endpoints for inspecting state, adding processes/resources, submitting requests, setting prevention policies, running detection scans, and executing recovery.
- **Preconfigured Scenarios:** Built-in presets for Safe State, Two-Process Deadlock Cycle, Multi-Resource Deadlock, Complex Chain, and a Custom Sandbox.

### Frontend Web UI Dashboard (Phase 6)
- **Interactive Single-Page Application (SPA):** Modern dark-themed dashboard built with clean HTML5/CSS3/JavaScript.
- **Dynamic SVG Resource Allocation Graph (RAG):** Renders bipartite graph showing process nodes (circles), resource nodes (rectangles with instance dots), allocation edges ($R \to P$), and request edges ($P \to R$).
- **Management Screens:** Dedicated views for Dashboard, Processes, Resources, Prevention, Detection, Recovery, and Event Log.

---

## 3. Module Explanations

| Module | Location | Description |
| :--- | :--- | :--- |
| **`app`** | `app/` | Contains the program entry points: `main.cpp` for the interactive terminal CLI and `server_main.cpp` for the HTTP REST API server. |
| **`core`** | `include/core/`, `src/core/` | Implements `ResourceManager` and data types (`Process`, `ResourceType`, `SystemState`). Manages matrix math, allocation tables, and conservation laws. |
| **`prevention`** | `include/prevention/`, `src/prevention/` | Implements `PreventionEngine`. Validates requests against Hold-and-Wait (all-or-nothing) and Circular-Wait (resource ordering) rules. |
| **`detection`** | `include/detection/`, `src/detection/` | Implements `DetectionEngine`. Executes the Shoshani-Coffman matrix reduction algorithm to detect deadlocked process candidates. |
| **`recovery`** | `include/recovery/`, `src/recovery/` | Implements `RecoveryEngine`. Handles victim process selection, resource reclamation, and iterative post-termination verification. |
| **`bridge`** | `include/bridge/`, `src/bridge/` | Implements `HttpServer`, `SimulationService`, `ApiRouter`, and `Json`. Bridges the C++ simulation core to external web clients. |
| **`frontend`** | `frontend/` | Web dashboard assets (HTML, CSS, JavaScript). Communicates with the C++ backend via `api_adapter.js`. |
| **`tests`** | `tests/` | Divided into `unit/` (algorithmic tests for each module) and `integration/` (end-to-end API and mutation workflows). |

---

## 4. Folder Structure Overview

```text
deadlock_prevention/
│
├── README.md                   # This project overview and documentation
├── CMakeLists.txt              # Root CMake configuration
├── .gitignore                  # Git ignore rules for build artifacts
├── build.bat                   # Automated Windows build script
├── run_tests.bat               # Automated test runner (47 tests)
├── start_server.bat            # Launches C++ HTTP server and opens browser
├── start_frontend.bat          # Launches frontend Python server (optional)
│
├── app/                        # Application entry points
│   ├── main.cpp                # Interactive CLI simulation executable
│   └── server_main.cpp         # HTTP REST API server executable
│
├── include/                    # Public C++ header files
│   ├── core/                   # State structs and ResourceManager
│   ├── prevention/             # PreventionEngine
│   ├── detection/              # DetectionEngine
│   ├── recovery/               # RecoveryEngine
│   └── bridge/                 # HttpServer, SimulationService, ApiRouter, Json
│
├── src/                        # C++ source files
│   ├── core/                   # Core resource tracking logic
│   ├── prevention/             # Prevention policy enforcement
│   ├── detection/              # Matrix reduction detection algorithm
│   ├── recovery/               # Victim termination logic
│   └── bridge/                 # HTTP server and REST dispatchers
│
├── tests/                      # Automated test suite
│   ├── CMakeLists.txt          # Test targets CMake configuration
│   ├── unit/                   # Unit test suites (35 tests)
│   │   ├── test_core.cpp       # Core engine and allocation tests (13 tests)
│   │   ├── test_prevention.cpp # Hold-and-Wait and Circular-Wait tests (8 tests)
│   │   ├── test_detection.cpp  # Multi-instance detection tests (7 tests)
│   │   └── test_recovery.cpp   # Termination and heuristic tests (7 tests)
│   └── integration/            # Integration test suites (12 tests)
│       └── test_api.cpp        # End-to-end REST API and state tests (12 tests)
│
├── docs/                       # Project documentation
│   ├── architecture.md         # Detailed architectural design and data flow
│   ├── project_structure.md    # Repository structure and modularity guide
│   ├── requirements.md         # Formal project specification
│   ├── test_plan.md            # Test matrix and algorithmic verification plan
│   ├── test-cases.md           # Individual test case specifications
│   ├── project-report-outline.md # Formal academic report outline
│   └── stitch_design/          # Google Stitch UI design files and assets
│
├── frontend/                   # Web frontend assets (HTML, CSS, JS)
│   ├── index.html              # Main single-page web dashboard
│   ├── css/                    # Modular styling (main, components, screens)
│   └── js/                     # Modular controllers, SVG graph, and API adapter
│
└── build/                      # Out-of-source CMake build directory
    ├── deadlock_sim.exe        # CLI executable
    ├── deadlock_server.exe     # HTTP server executable
    ├── deadlock_tests.exe      # Core unit tests
    ├── prevention_tests.exe    # Prevention unit tests
    ├── detection_tests.exe     # Detection unit tests
    ├── recovery_tests.exe      # Recovery unit tests
    └── api_tests.exe           # API integration tests
```

---

## 5. Required Tools and Dependencies

### Required
- **Operating System:** Windows 10 or Windows 11 (64-bit).
- **C++ Compiler:** GCC 8.1.0+ (MinGW-w64) or MSVC with C++17 support.
- **Build System:** CMake 3.16 or higher.
- **System Libraries:** Windows Sockets 2 (`ws2_32.lib` - included with Windows).
- **Web Browser:** Modern browser (Chrome, Edge, Firefox) for the dashboard.

### Optional
- **Python 3.x:** Only needed if using `start_frontend.bat` as an alternative static server instead of the native C++ HTTP server.

---

## 6. Configure and Build Commands

### Method A: Automated Batch Script (Recommended for Windows)

Double-click `build.bat` or run from PowerShell / Command Prompt:

```cmd
build.bat
```

This script automatically:
1. Detects the MinGW compiler toolchain.
2. Creates the `build/` directory if absent.
3. Runs CMake configuration with out-of-source settings.
4. Compiles the static library `deadlock_core`, both application executables, and all five test suites.

### Method B: Manual CMake Configuration

From the project root:

```cmd
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

*Note for MSVC users:*
```cmd
cmake -B build
cmake --build build --config Release
```

---

## 7. Running the Simulator and API Server

### Running the Web Dashboard and API Server

Launch the native C++ server:

```cmd
start_server.bat
```

Or execute directly:

```cmd
build\deadlock_server.exe 8080 frontend
```

- **Web Dashboard:** Open [http://localhost:8080](http://localhost:8080) in your browser.
- **REST API Base URL:** [http://localhost:8080/api/state](http://localhost:8080/api/state)
- Press `Ctrl + C` in the console window to stop the server cleanly.

### Running the Interactive Terminal CLI Simulator

To run the interactive command-line interface:

```cmd
build\deadlock_sim.exe
```

The CLI allows step-by-step console interaction to:
1. Initialize processes and resource capacities.
2. Allocate and release resources interactively.
3. Configure Hold-and-Wait or Circular-Wait prevention.
4. Run on-demand multi-instance deadlock detection scans.
5. Select and terminate victim processes to recover from deadlocks.

---

## 8. Running Automated Tests

The test suite contains **47 comprehensive automated tests** with 0 external testing dependencies.

### Run All Tests via Batch Script

```cmd
run_tests.bat
```

### Run Tests Individually

```cmd
build\deadlock_tests.exe      # Core Engine Tests (13 tests)
build\prevention_tests.exe    # Prevention Policy Tests (8 tests)
build\detection_tests.exe     # Multi-Instance Detection Tests (7 tests)
build\recovery_tests.exe      # Recovery & Heuristic Tests (7 tests)
build\api_tests.exe           # API & Integration Tests (12 tests)
```

### Test Suite Summary

```text
========================================================
 [ALL TESTS PASSED] 47/47 Unit and Integration Tests Succeeded!
========================================================
- Core Unit Tests:        13 Passed, 0 Failed
- Prevention Unit Tests:   8 Passed, 0 Failed
- Detection Unit Tests:    7 Passed, 0 Failed
- Recovery Unit Tests:     7 Passed, 0 Failed
- API Integration Tests:  12 Passed, 0 Failed
```

---

## 9. Cleaning Build Artifacts

To remove all compiled binaries, intermediate object files, and CMake cache files, simply delete the `build/` folder:

```cmd
rmdir /s /q build
```

Or in PowerShell:

```powershell
Remove-Item -Recurse -Force build
```

To recompile, simply execute `build.bat` again. The source directories remain pristine and unaffected.

---

## 10. Current Limitations and Planned Features

### Current Limitations
1. **Operating System Portability for HTTP Bridge:** The native `HttpServer` uses Windows Winsock2 API (`ws2_32`). Linux/macOS support currently requires compiling with standard POSIX socket headers.
2. **Recovery via Termination Only:** When deadlocks are detected, recovery is achieved exclusively through process abort (termination). Resource preemption with checkpoint/rollback is not currently implemented.
3. **Prevention Overhead:** In accordance with OS theory:
   - Hold-and-Wait prevention requires upfront maximum allocation, which may lead to low resource utilization.
   - Circular-Wait prevention requires total order compliance, which may unnecessarily delay independent requests.

### Planned / Future Features
- **Deadlock Avoidance (Banker's Algorithm Safe State Evaluation):** Theoretical safety checks and safe sequence determination $\langle P_0, P_1, \dots \rangle$ based on $\mathbf{Need} \le \mathbf{Available}$ as an alternative dynamic policy.
- **Resource Preemption and Rollback:** Checkpoint process execution states to permit temporary resource revocation without full process destruction.
- **Cross-Platform Networking Bridge:** Replace platform-specific Winsock calls with cross-platform abstractions to support native Linux and macOS builds.
