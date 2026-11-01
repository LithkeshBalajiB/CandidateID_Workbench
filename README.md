# Unified Engineering Workbench

**PASS Science Works R&D Internship 2026–2027 Common Technical Assignment**  
**Candidate ID:** [FILL: Candidate ID]  
**Archive:** `CandidateID_Workbench.zip`  
**Target Environment:** Windows 11 x64 (Clean Machine Compatible)  

---

## 1. One-Click Execution (No Setup Required)

To run the packaged application on any clean Windows 11 x64 machine:
1. Extract `CandidateID_Workbench.zip`.
2. Double-click `Launch.exe` located at the root of the extracted folder.
3. **That's it!**
   - **NO** command prompt or PowerShell windows.
   - **NO** administrator rights required.
   - **NO** compiler, Python, Node.js, or package installation.
   - **NO** first-run build step.
   - **NO** internet connection (100% offline).

`Launch.exe` starts the local high-performance C++ backend server in memory and immediately opens your default browser at `http://127.0.0.1:8080`.

---

## 2. Editor Run Configuration (VS Code / Antigravity)

For development inside VS Code or Antigravity IDE:
1. Open this repository directory as the workspace.
2. Select the configuration: **"Launch Workbench (Packaged Application)"** in the Run & Debug view (`Ctrl+Shift+D`).
3. Press **`F5`** (or Run).
   - VS Code will execute the pre-launch task to build the project and launch the application in one single action.
4. *Prerequisite Extension:* C/C++ Extension (`ms-vscode.cpptools`).

---

## 3. High-Level Architecture

The workbench enforces strict separation of concerns across three distinct layers:

```text
workbench/
├── Launch.exe                  # Standalone GUI launcher (C++17, static runtime)
├── bin/
│   └── workbench_server.exe    # Localhost transport server binary
├── src/
│   ├── core/                   # Pure C++17 library (no UI, no HTTP, no console I/O)
│   │   ├── thermal_fea.*       # Task C: 1D Thermal FEA solver & convergence engine
│   │   ├── backprop.*          # Task F: Non-autodiff backpropagation & finite differences
│   │   ├── parallel_batch.*    # Task D: Multicore bounded thread pool engine
│   │   ├── cad_ansys_parser.*  # Task B: ASCII STL and ANSYS CSV parsers
│   │   └── embedded_device.*   # Task E: Mock MCU, FPGA, and JTAG adapters
│   ├── server/                 # Thin transport layer (cpp-httplib + nlohmann/json)
│   └── launcher/               # Windows WinMain process lifecycle launcher
├── web/                        # Static offline frontend (plain HTML5, CSS3, vanilla JS)
│   ├── index.html              # 4 Views: Geometry, Thermal Analysis, Performance, Hardware
│   ├── css/style.css           # Modern dark-mode responsive design system
│   └── js/
│       ├── app.js              # Viewports coordinator & canvas charting
│       └── three.min.js        # Vendored Three.js r128 (100% offline WebGL)
├── data/                       # Bundled datasets & provenance documentation
├── results/                    # Exported CSVs, labelled benchmark plots, commit logs
├── docs/                       # Technical documentation, 6-page report, and BUILD guide
├── scripts/                    # Test runners and packaging audit scripts
└── tests/                      # Unit test suites (doctest framework)
```

---

## 4. Supported Formats & Capabilities

1. **ASCII STL Geometry (`.stl`):**
   - Full 3D rendering with orbit, pan, zoom, and fit-to-view.
   - Robust C++ parsing: checks triangular loop closure, calculates bounding box, and rejects binary STL files or non-finite values with line-numbered messages.
2. **ANSYS Nodal Temperature CSV (`.csv`):**
   - Exact schema: `node_id,x_m,y_m,z_m,temperature_C`.
   - 3D point cloud visualization with heat-map color scale (20°C to 185°C), temperature legend, and interactive node inspection.
   - Strictly validates duplicate node IDs, column counts, and non-finite NaN/Inf tokens.
3. **1D Thermal FEA Rod Simulation:**
   - Assembles linear element stiffness and load vectors.
   - Solves $-d/dx(kA dT/dx) = qA$ with prescribed boundary temperatures.
   - Reports nodal and midpoint errors with convergence order $p \approx 2.00$.
4. **Multicore Parallel Engine:**
   - Independent batch execution using bounded `std::thread` pool.
   - Preserves deterministic input ordering.
   - Serial vs. parallel agreement verified to $\max |\Delta T| = 0.000\times 10^0$ °C ($\le 10^{-8}$ °C).
5. **Embedded Integration Mocks:**
   - Simulated MCU, FPGA, and JTAG probe telemetry.
   - Demonstrates recoverable UART timeout failure and retry restoration.

---

## 5. Building from Source & Testing

For complete source-build instructions using CMake, refer to [docs/BUILD.md](docs/BUILD.md).

### Quick Commands (PowerShell)
```powershell
# 1. Run all unit tests and benchmarks in one action
powershell -ExecutionPolicy Bypass -File scripts\run_all_tests.ps1

# 2. Audit repository for submission packaging compliance
powershell -ExecutionPolicy Bypass -File scripts\check_package.ps1
```

---

## 6. Known Limitations

1. **Binary STL and Native CAD Formats:** By design, native CAD solids (`.step`, `.sldprt`) and binary STL formats are out of scope. Binary STL files are actively detected and rejected with an actionable conversion recommendation.
2. **ANSYS File Scope:** Native proprietary ANSYS binary files (`.rst`, `.wbpj`) are out of scope; only nodal CSV exports are supported.
3. **Simulated Hardware:** Embedded devices (MCU, FPGA, JTAG) are purely software mocks labelled `[SIMULATED]`; real FPGA bitstream flashing or physical circuit routing is outside the assignment scope.
