# Unified Engineering Workbench - Demonstration Script & Shot List

**Target Duration:** 3 to 5 minutes  
**Format:** Live Screen Recording or Timestamped Screenshot Walkthrough  
**Target Audience:** PASS Science Works Technical Reviewers & Interview Committee  

---

## Shot List Summary Table

| Timestamp | View / Scene | User Action | Observed System Response | Technical Rubric Verified |
| :--- | :--- | :--- | :--- | :--- |
| **00:00 - 00:35** | Desktop / Windows Explorer | Double-click `Launch.exe` | Zero black terminal popups. Embedded HTTP server starts on port `8080`. Default browser opens immediately to `http://localhost:8080/`. Launch latency `< 1000 ms`. | **Gate:** Clean launch, standalone EXE, 0 terminal windows. |
| **00:35 - 01:20** | **View 1: Thermal FEA & Convergence** | Set $L=1.0\,\text{m}$, $k=50\,\text{W/(m}\cdot\text{K)}$, $q=10000\,\text{W/m}^3$, $T_L=100^\circ\text{C}$, $T_R=200^\circ\text{C}$, elements $N=20$. Click **"Run 1D FEA"**. | Instantaneous profile plot (analytical vs. FEA curve). Quadratic bow visible. Click **"Run Mesh Convergence"**: Table shows $N=10, 20, 40, 80$, midpoint error scaling with $h^2$, observed order $p = 2.0000$. | **Task C:** 1D FEA formulation, Thomas algorithm, convergence order $O(h^2)$. |
| **01:20 - 02:05** | **View 2: Neural Verification (Task F)** | Click **"Run Handwritten Backprop"** (target $y=2.0$, initial $w=0.5, b=0.2, v=0.3, c=0.1$, $\eta=0.1$). | Interactive loss chart plots 100 steps. Loss monotonically decreases from initial value to $0.000000$. Final output $\hat{y} = 2.000000$. Gradient verification status shows agreement with finite differences $< 10^{-10}$. | **Task F:** Handwritten backprop, zero autodiff, finite diff check, monotone descent. |
| **02:05 - 02:55** | **View 3: CAD & ANSYS Cloud (Task B)** | Click **"Load Sample STL"** (`data/sample_rod.stl`). Then click **"Load ANSYS Cloud"** (`data/ansys_thermal_synthetic.csv`). | Three.js WebGL viewport renders 3D faceted geometry with smooth orbit controls. Thermal point cloud overlays with temperature colormap (blue $100^\circ\text{C}$ to red $188^\circ\text{C}$). Clicking a node reveals exact ID, coordinates, and temperature. | **Task B:** ASCII STL parser, ANSYS CSV parser, offline 3D rendering, point inspection. |
| **02:55 - 03:50** | **View 4: Multicore Batch (Task D)** | Select **Batch Size: 2000 cases**, Mesh $N=100$. Run benchmark across **1, 2, and 4 threads**. | Real-time progress bar streams completion percentage. Speedup chart renders measured timings: 1 worker ($\approx 7.4\,\text{ms}$), 2 workers ($\approx 5.3\,\text{ms}$, $1.39\times$), 4 workers ($\approx 4.3\,\text{ms}$, $1.72\times$). Numerical verification badge confirms $\max|\Delta T| = 0.000^\circ\text{C}$. | **Task D:** Bounded thread pool, atomic index dispatch, real multicore scaling, cancellation safety. |
| **03:50 - 04:30** | **View 4: Embedded Mocks (Task E)** | Select Device: **MCU (STM32H7 Mock)**. Toggle connection. Click **"Inject Timeout Fault"**, then **"Clear Fault & Retry"**. | Telemetry meters display live voltage ($3.29\,\text{V}$) and core temperature ($42.1^\circ\text{C}$). Status turns Red (`Faulted: Timeout`). Clicking clear restores Green (`Ready`) after mock reset. Device logs clearly labelled `[SIMULATED]`. | **Task E:** Device abstraction, fault recovery, transparent simulation markers. |
| **04:30 - 04:45** | Server Shutdown & Exit | Click **"Shutdown Workbench Server"** in UI. | Server gracefully terminates child process; browser shows disconnected state. Zero orphaned processes in Windows Task Manager. | Clean lifecycle management. |

---

## Detailed Step-by-Step Walkthrough & Narration

### Scene 1: Zero-Friction Launch (00:00 - 00:35)
- **Narration:**  
  *"Welcome to the Unified Engineering Workbench demonstration. Notice our working directory: it contains a single self-contained executable `Launch.exe`, our web front-end, data files, and documentation. When I double-click `Launch.exe`, there are no black console flashes, no PowerShell scripts, and no environment setup. Within under 1 second, our local C++ server starts silently on port 8080 and launches the default web browser."*
- **Visual Cue:**  
  Double click `Launch.exe`. Desktop remains completely clean. Chrome/Edge opens directly to `http://localhost:8080/`. The header displays *Unified Engineering Workbench — Offline C++ Backend*.

### Scene 2: 1D Steady-State Thermal FEA (00:35 - 01:20)
- **Narration:**  
  *"We begin in the Thermal FEA view. The backend implements pure linear 1D finite elements with exact row/column Dirichlet elimination and an $O(N)$ Thomas tridiagonal solver. With internal heat generation $q = 10,000\,\text{W/m}^3$, thermal conductivity $k = 50\,\text{W/(m}\cdot\text{K)}$, and boundary conditions $100^\circ\text{C}$ and $200^\circ\text{C}$, we click 'Run 1D FEA'. The temperature curve exhibits the physical quadratic temperature bulge. Next, we run the Mesh Convergence Study across $N = 10, 20, 40, 80$ elements. Because linear elements produce nodal superconvergence, discretization error is correctly measured at element midpoints: the error halves by a factor of 4 when mesh spacing halves, proving exact quadratic $O(h^2)$ convergence with an observed order of $p = 2.0000$."*
- **Visual Cue:**  
  Click "Run 1D FEA" to inspect chart. Click "Run Mesh Convergence" to review table and log-log convergence curve.

### Scene 3: Handwritten Backpropagation Verification (01:20 - 02:05)
- **Narration:**  
  *"In the Neural Verification tab, we inspect Task F: a 2-layer scalar network with tanh activation and identity output. Crucially, this implementation contains zero automatic differentiation libraries. Every partial derivative—$\frac{\partial J}{\partial w}, \frac{\partial J}{\partial b}, \frac{\partial J}{\partial v}, \frac{\partial J}{\partial c}$—is derived analytically by hand and validated against central finite differences with $\epsilon = 10^{-6}$. The numerical error between analytical and finite difference gradients is $1.13 \times 10^{-10}$, well below our $10^{-5}$ threshold. Clicking 'Run Training Loop' executes 100 gradient descent steps: loss strictly decreases monotonically to $0.000000$, and model prediction converges to the target value of $2.0$."*
- **Visual Cue:**  
  Click "Verify Analytical Gradients" (shows green checkmarks). Click "Run 100-Step Training" (watch animated loss descent and monotone status banner).

### Scene 4: CAD STL & ANSYS Cloud Visualization (02:05 - 02:55)
- **Narration:**  
  *"Under the CAD & ANSYS Inspection tab, our C++ backend parses ASCII STL meshes and ANSYS node temperature CSV exports. Loading `data/sample_rod.stl`, our custom parser validates ASCII headers, normal vectors, and facet loop structures, rendering the solid in an offline vendored Three.js canvas. Next, we load `data/ansys_thermal_synthetic.csv`. The backend streams 100 nodes, maps temperatures from $100^\circ\text{C}$ to $188^\circ\text{C}$ onto an HSL heatmap, and registers 3D hit targets. Clicking any node instantly displays its physical coordinates and temperature in the HUD."*
- **Visual Cue:**  
  Click "Load Sample Rod STL" -> 3D wireframe/solid appears. Click "Load ANSYS Cloud" -> Heatmap colored points appear along the rod. Hover/click node #50 to display coordinates $(0.5, 0.02, 0.0)$ and $T = 162.5^\circ\text{C}$.

### Scene 5: Multicore Batch Parallelism & Scaling (02:55 - 03:50)
- **Narration:**  
  *"In View 4, we evaluate multicore scaling. Our C++ backend features a bounded `std::thread` pool that pulls tasks using lock-free atomic index increments. We evaluate 2,000 thermal FEA cases across 1, 2, and 4 worker threads. An active progress bar updates every 100 cases. Results show 1 thread at 7.4 ms, 2 threads at 5.3 ms (1.39x speedup), and 4 threads at 4.3 ms (1.72x speedup). Crucially, we verify that serial and parallel results match identically with max temperature difference $0.000^\circ\text{C}$. Our analysis honestly documents thread coordination latency and memory bus contention rather than claiming synthetic linear scaling."*
- **Visual Cue:**  
  Select 2000 cases. Click "Run Multicore Benchmark". Watch live progress bar reach 100%. Bar chart displays scaling curve with 0 diff verification badge.

### Scene 6: Hardware Mocks & Safe Shutdown (03:50 - 04:45)
- **Narration:**  
  *"Finally, the Embedded Integration tab demonstrates our `IDevice` mock framework for MCU and FPGA telemetry. All data is clearly labelled with simulated markers. We observe realistic temperature and voltage jitter. When we simulate a communication fault, the status changes to red with an actionable error code. Clicking 'Clear Fault & Retry' issues a hardware reset sequence and restores normal telemetry. To conclude, clicking 'Shutdown' stops the background server and terminates all worker threads with zero orphaned processes."*
- **Visual Cue:**  
  Click "Connect MCU Mock". Observe live voltage/temp updates. Click "Inject Timeout Fault" (red banner). Click "Clear & Retry" (restores green status). Click "Shutdown Server" to end session cleanly.
