#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

namespace workbench::core {

/**
 * @brief Input parameters for the 1D steady-state thermal FEA solver.
 * Governing equation: -d/dx (k*A*dT/dx) = q*A
 */
struct Thermal1DParams {
    double L = 1.0;          ///< Rod length in meters (m)
    double A = 1.0e-4;       ///< Cross-sectional area in square meters (m^2)
    double k = 10.0;         ///< Thermal conductivity in W/(m*K)
    double q = 10000.0;      ///< Uniform volumetric heat generation in W/m^3
    double T0 = 100.0;       ///< Prescribed temperature at x = 0 (deg C)
    double TL = 20.0;        ///< Prescribed temperature at x = L (deg C)
    int N = 10;              ///< Number of uniform two-node linear elements
};

/**
 * @brief Results obtained from solving the 1D steady-state thermal FEA problem.
 */
struct Thermal1DResult {
    Thermal1DParams params;
    double h = 0.0;                          ///< Element length (m)
    std::vector<double> x_nodes;             ///< Node coordinates (m)
    std::vector<double> T_nodes;             ///< FE nodal temperatures (deg C)
    std::vector<double> T_exact_nodes;       ///< Analytical exact temperatures at nodes (deg C)
    std::vector<double> x_midpoints;         ///< Element midpoint coordinates (m)
    std::vector<double> T_mid_fe;            ///< FE interpolated temperatures at midpoints (deg C)
    std::vector<double> T_mid_exact;         ///< Analytical exact temperatures at midpoints (deg C)
    double max_node_error = 0.0;             ///< Max absolute nodal error |T_fe - T_exact| (deg C)
    double max_midpoint_error = 0.0;         ///< Max absolute midpoint error (deg C)
};

/**
 * @brief Convergence metrics for a given element resolution.
 */
struct ConvergenceRow {
    int N = 0;                     ///< Number of elements
    double h = 0.0;                ///< Element size (m)
    double max_node_error = 0.0;    ///< Max nodal error (deg C)
    double max_midpoint_error = 0.0;///< Max midpoint error (deg C)
    double observed_order = 0.0;   ///< Numerical rate of convergence p
};

/**
 * @brief Validates input parameters. Returns empty string if valid, or actionable error string.
 */
std::string validate_thermal_params(const Thermal1DParams& p);

/**
 * @brief Computes the analytical exact temperature at coordinate x.
 * T_exact(x) = T0 - (T0 - TL)*x/L + [q/(2k)]*x*(L - x)
 */
double exact_temperature_1d(double x, const Thermal1DParams& p);

/**
 * @brief Solves the steady-state 1D heat conduction equation using two-node linear FE.
 * Assembles element stiffness (kA/h)[1 -1; -1 1] and load (qAh/2)[1; 1].
 * Imposes boundary conditions via row/column elimination and solves the tridiagonal system.
 */
Thermal1DResult solve_thermal_1d(const Thermal1DParams& p);

/**
 * @brief Runs a mesh refinement study across a sequence of element counts (e.g. 10, 20, 40, 80).
 */
std::vector<ConvergenceRow> run_convergence_study(
    const Thermal1DParams& base_params, 
    const std::vector<int>& N_values
);

/**
 * @brief Exports temperature profiles (x, T_fe, T_exact, N) as CSV text.
 */
std::string export_profiles_csv(const std::vector<Thermal1DResult>& results);

/**
 * @brief Exports the convergence study table (N, h, node_error, midpoint_error, observed_order) as CSV.
 */
std::string export_convergence_csv(const std::vector<ConvergenceRow>& rows);

/**
 * @brief Utility to save string content to a specified file path.
 */
bool save_string_to_file(const std::string& filepath, const std::string& content);

} // namespace workbench::core
