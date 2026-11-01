# Task C: 1D Thermal FEA Formulation & Interdisciplinary Adoption Note

**Candidate ID:** [FILL: Candidate ID]  
**Background:** Computer Science (Big Data Analytics) Undergraduate  
**Topic:** 1D Steady-State Heat Conduction with Internal Heat Generation  

---

## 1. Problem Definition & Mathematical Formulation

### 1.1 Physical Setup and Governing Equation
We model steady-state heat conduction along a uniform cylindrical or prismatic rod of length $L$, cross-sectional area $A$, and thermal conductivity $k$, with insulated lateral surfaces and uniform volumetric heat generation $q$.

$$\frac{d}{dx} \left( -k A \frac{dT}{dx} \right) = q A, \quad \text{for } x \in (0, L)$$

Since material properties ($k$) and geometry ($A$) are constant:
$$-k \frac{d^2 T}{dx^2} = q \implies \frac{d^2 T}{dx^2} = -\frac{q}{k}$$

### 1.2 Boundary Conditions & Units Table

| Parameter | Symbol | Numerical Value | Standard SI Units | Description |
| :--- | :---: | :---: | :---: | :--- |
| Rod Length | $L$ | $1.0$ | $\text{m}$ | Length of physical domain |
| Cross-Sectional Area | $A$ | $1.0 \times 10^{-4}$ | $\text{m}^2$ | Constant cross-section ($1\text{ cm} \times 1\text{ cm}$) |
| Thermal Conductivity | $k$ | $10.0$ | $\text{W}/(\text{m}\cdot\text{K})$ | Material heat conduction property |
| Volumetric Heat Gen. | $q$ | $10,000.0$ | $\text{W}/\text{m}^3$ | Uniform distributed heat source |
| Left Boundary Temp | $T(0)$ | $100.0$ | $^\circ\text{C}$ | Essential (Dirichlet) boundary condition |
| Right Boundary Temp | $T(L)$ | $20.0$ | $^\circ\text{C}$ | Essential (Dirichlet) boundary condition |

### 1.3 Analytical Exact Solution
Integrating $\frac{d^2 T}{dx^2} = -\frac{q}{k}$ twice:
$$\frac{dT}{dx} = -\frac{q}{k}x + C_1$$
$$T(x) = -\frac{q}{2k}x^2 + C_1 x + C_2$$

Applying boundary conditions:
1. At $x = 0$: $T(0) = C_2 = 100^\circ\text{C}$
2. At $x = L$: $T(L) = -\frac{q}{2k}L^2 + C_1 L + 100 = 20 \implies C_1 = -\frac{80}{L} + \frac{q L}{2k}$

Yielding the closed-form exact temperature profile:
$$T_{\text{exact}}(x) = 100 - 80\frac{x}{L} + \frac{q}{2k} x (L - x)$$

---

## 2. Finite Element Method (Galerkin Weak Form)

### 2.1 Element Formulation (2-Node Linear Lagrange Elements)
For a uniform element $e$ of length $h = L/N$ spanning $[x_1, x_2]$:
$$T^e(x) = N_1(x) T_1^e + N_2(x) T_2^e$$
$$N_1(x) = \frac{x_2 - x}{h}, \quad N_2(x) = \frac{x - x_1}{h}$$

The element stiffness matrix $K^e$ and element load vector $F^e$:
$$K^e = \int_{x_1}^{x_2} k A \frac{dN^T}{dx} \frac{dN}{dx} dx = \frac{k A}{h} \begin{bmatrix} 1 & -1 \\ -1 & 1 \end{bmatrix}$$
$$F^e = \int_{x_1}^{x_2} q A N^T dx = \frac{q A h}{2} \begin{bmatrix} 1 \\ 1 \end{bmatrix}$$

### 2.2 Boundary Condition Imposition (Row/Column Elimination)
For global assembly $K T = F$:
- Node $0$ is fixed to $T_0 = 100^\circ\text{C}$
- Node $N$ is fixed to $T_N = 20^\circ\text{C}$
Row/column elimination removes equations $0$ and $N$, modifying the interior load vector:
$$\tilde{F}_1 = F_1 - K_{1,0} T_0 = q A h + \frac{k A}{h} T_0$$
$$\tilde{F}_{N-1} = F_{N-1} - K_{N-1,N} T_N = q A h + \frac{k A}{h} T_N$$
The reduced tridiagonal system is symmetric and strictly diagonally dominant, solved in $O(N)$ operations via the Thomas algorithm.

---

## 3. Real Debugging Insight: The Nodal vs. Midpoint Paradox

### 3.1 The Observation
During initial verification, inspecting error solely at the mesh nodes ($x_i = i \cdot h$) gave:
$$|T_{\text{fe}}(x_i) - T_{\text{exact}}(x_i)| < 10^{-12} \, ^\circ\text{C} \quad (\approx 0 \text{ within floating-point epsilon})$$
This created a deceptive illusion that the FE solution was 100% exact everywhere. However, evaluating at **element midpoints** $x_{\text{mid}} = x_i + h/2$ using FE linear interpolation revealed a distinct, nonzero error:
- $N=10$: $e_{\text{mid}} = 1.2500^\circ\text{C}$
- $N=20$: $e_{\text{mid}} = 0.3125^\circ\text{C}$ ($4\times$ reduction)
- $N=40$: $e_{\text{mid}} = 0.0781^\circ\text{C}$ ($4\times$ reduction)
- $N=80$: $e_{\text{mid}} = 0.0195^\circ\text{C}$ ($4\times$ reduction)

### 3.2 The Mathematical Reason
In 1D steady-state diffusion, the finite element weak form minimizes energy norm error over the linear trial function space. A foundational property of the 1D Galerkin method with piecewise linear elements is that the finite element Green's function coincides exactly with the continuous Green's function evaluated at the nodes. Therefore, **nodal values are exact**.

However, the continuous solution $T_{\text{exact}}(x)$ is quadratic (curvature $T''(x) = -q/k = -1000^\circ\text{C}/\text{m}^2$), whereas the FE approximation inside each element is a linear chord (secant line) connecting the two exact nodal endpoints. The difference between a parabola and its secant chord over an interval of length $h$ reaches its maximum at the midpoint:
$$e_{\text{mid}} = \left| \frac{T(x_i) + T(x_{i+1})}{2} - T\left(x_i + \frac{h}{2}\right) \right| = \frac{q}{8k} h^2$$
For our parameters:
$$e_{\text{mid}} = \frac{10000}{8 \times 10} h^2 = 125 \, h^2$$
- $h = 0.1 \implies 125 \times 0.01 = 1.2500^\circ\text{C}$
- $h = 0.05 \implies 125 \times 0.0025 = 0.3125^\circ\text{C}$
- Observed convergence rate: $p = \frac{\ln(1.25 / 0.3125)}{\ln(0.1 / 0.05)} = \frac{\ln(4)}{\ln(2)} = 2.000$ (Second-order convergence).

This taught me that in FEA verification, **evaluating error only at nodes can completely mask interpolation errors**, and testing interior points or element midpoints is essential.

---

## 4. Interdisciplinary Adoption Note (CS Undergraduate Perspective)

*(Personal reflection template: candidate should review and customize during interview preparation).*

### 4.1 Transitioning from Discrete Data to Continuous Physics
Coming from a Computer Science background focused on Big Data Analytics (matrices, algorithms, and distributed processing), physics-based modeling initially felt unfamiliar because mechanical problems deal with continuous differential operators rather than discrete datasets.

- **Initial Mental Barrier:** How do continuous boundary-value problems translate into linear algebra?
- **The Breakthrough:** Understanding the Galerkin weak form as an orthogonal projection. Just like linear regression in data science minimizes the sum of squared residuals $(y - X\beta)^T(y - X\beta)$, the finite element method projects the differential equation onto a subspace of piecewise-linear basis functions by minimizing the energy functional $\Pi(T) = \int [\frac{1}{2} k (\nabla T)^2 - q T] dx$.
- **Mapping to CS Concepts:**
  - Mesh generation $\rightarrow$ 1D discretized array partitioning.
  - Stiffness matrix assembly $\rightarrow$ Map-Reduce accumulation of local element contributions into a global sparse matrix.
  - Boundary condition imposition $\rightarrow$ Subspace constraint elimination.
  - Tridiagonal solver $\rightarrow$ Dynamic programming / Thomas Gaussian elimination ($O(N)$ linear time).

### 4.2 Candidate Reflection Prompts (Fill with Your Experience)
- **Prompt 1: What was the most difficult concept to grasp initially?**
  *[FILL: e.g., Deriving why the load vector is divided evenly (qAh/2) between the two nodes rather than placed at the centroid, which stems from integrating the linear basis functions N_1(x) and N_2(x) over the element domain].*
- **Prompt 2: How did you verify correctness independently?**
  *[FILL: e.g., Cross-checking with hand calculations for N=1 and N=2, writing unit tests with boundary condition symmetry, checking global energy conservation (heat generated = heat flux out at boundaries), and plotting analytical vs FE solutions].*
- **Prompt 3: If asked to modify this in an interview, what is one extension you understand?**
  *[FILL: e.g., Handling a convective boundary condition (Robin BC) at x = L: -k dT/dx = h_conv (T - T_inf), which adds h_conv to K(N,N) and h_conv*T_inf to F(N)].*
