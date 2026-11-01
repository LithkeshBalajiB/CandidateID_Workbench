# Data Provenance and Schema Documentation

This document records the provenance, units, coordinate systems, and schema specifications for all sample datasets bundled with the Unified Engineering Workbench.

---

## 1. ASCII STL Geometry File

### File: `data/sample_rod.stl`
- **Format:** Standard ASCII stereolithography (`.stl`).
- **Provenance / Origin:** Created synthetically by candidate as an openly licensed test model for solid structural visualization.
- **Licence:** Creative Commons Zero (CC0) / Public Domain.
- **Dimensions & Bounding Box:**
  - $X \in [0.0, 1.0]\text{ m}$ (length)
  - $Y \in [0.0, 0.05]\text{ m}$ (width, 50 mm)
  - $Z \in [0.0, 0.05]\text{ m}$ (height, 50 mm)
- **Topology:** 12 triangular facets (6 rectangular faces $\times$ 2 triangles per face), 36 vertices.
- **Units:** SI Meters ($\text{m}$).
- **Validation Notes:** Validated to ensure non-degenerate normals, triangular loop closure (`outer loop` $\rightarrow$ 3 `vertex` $\rightarrow$ `endloop`), and absence of binary null headers.

---

## 2. ANSYS Nodal Temperature File

### File: `data/ansys_thermal_synthetic.csv`
- **Format:** Comma-Separated Values (`.csv`).
- **Provenance / Status:** **SYNTHETIC DATASET**.
  > **Explicit Disclosure:** This CSV file was generated synthetically by the candidate using the 1D/3D thermal diffusion equation and is **NOT** an actual proprietary export from ANSYS Mechanical software.
- **Licence:** Creative Commons Zero (CC0) / Public Domain.
- **Required Column Schema (Exact Header):**
  ```csv
  node_id,x_m,y_m,z_m,temperature_C
  ```

### Field Definitions & Units

| Column | Data Type | Physical Meaning | Standard Units | Valid Range |
| :--- | :--- | :--- | :---: | :--- |
| `node_id` | Integer (`int64`) | Unique FE mesh node identifier | Dimensionless | Strictly unique, $\ge 1$ |
| `x_m` | Floating Point | Node X-coordinate | Meters ($\text{m}$) | Finite real number |
| `y_m` | Floating Point | Node Y-coordinate | Meters ($\text{m}$) | Finite real number |
| `z_m` | Floating Point | Node Z-coordinate | Meters ($\text{m}$) | Finite real number |
| `temperature_C` | Floating Point | Nodal steady-state temperature | Degrees Celsius ($^\circ\text{C}$) | Finite real number |

- **Sample Statistics:**
  - Total Nodes: $99$
  - Temperature Range: $[19.83^\circ\text{C}, 185.00^\circ\text{C}]$
  - Mean Temperature: $118.78^\circ\text{C}$

---

## 3. Out-of-Scope Formats
In accordance with assignment boundaries:
- Proprietary binary ANSYS database formats (`.db`, `.rst`, `.wbpj`) are explicitly out of scope.
- Proprietary binary CAD B-Rep formats (`.step`, `.iges`, `.sldprt`, native solids) are explicitly out of scope.
