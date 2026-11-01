# Statement of Ownership and Reused Work

**Candidate ID:** [FILL: Candidate ID]  
**Project:** Unified Engineering Workbench  
**Submission Date:** [FILL: Date]  

---

## 1. Original Work
Unless explicitly designated in Section 2 below, all implementation code, analytical derivations, test suites, architecture designs, and technical documentation in this repository represent the original work of the candidate.

Specifically original work includes:
- **Task C (Thermal FEA Solver):** 1D Finite Element assembly, stiffness matrix generation, boundary condition imposition via row/column elimination, tridiagonal solver implementation, midpoint interpolation, and analytical convergence analysis.
- **Task F (Backpropagation):** Handwritten mathematical derivations, manual forward/backward propagation implementation without automatic differentiation, numerical finite-difference gradient verification harness, and training step iterations.
- **Task D (Multicore Workload):** Deterministic parallel batch solve engine, bounded worker thread pool, benchmark harness, and timing statistical analysis.
- **Task B (CAD & ANSYS Parsers):** C++ ASCII STL and CSV parsers, geometric data validation, boundary and malformed data error reporting.
- **Task E (Embedded Subsystem Mocks):** C++ mock interface architecture, telemetry generation, recoverable state machine transitions, and integration design documentation.
- **Task A (Workbench Server & Interface):** UI layout, data visualization coordination, cancellation/atomic file writing, and packaging architecture.

---

## 2. Reused Code and Third-Party Dependencies
The following open-source libraries and resources have been incorporated with explicit credit and adherence to their respective licences:
- **`doctest` (v2.4.11):** MIT Licence. Used strictly as an offline header-only unit testing harness (`third_party/doctest/doctest.h`).
- **`nlohmann/json` (v3.11.3):** MIT Licence. Used for JSON serialization across the HTTP transport layer (`third_party/nlohmann/json.hpp`).
- **`cpp-httplib` (v0.18.3):** MIT Licence. Used for localhost HTTP server implementation (`third_party/httplib.h`).
- **`Three.js` (r128):** MIT Licence. Vendored minified script used for client-side WebGL 3D rendering of STL triangles and point clouds (`web/js/three.min.js`).

Full licence texts, repository links, and version lock records are documented in [THIRD_PARTY.md](THIRD_PARTY.md).
