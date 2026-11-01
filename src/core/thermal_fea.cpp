#include "core/thermal_fea.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

namespace workbench::core {

std::string validate_thermal_params(const Thermal1DParams& p) {
    if (!std::isfinite(p.L) || p.L <= 0.0) {
        return "Validation Error: Rod length L must be positive and finite (received: " + std::to_string(p.L) + " m). Fix: Set L > 0.";
    }
    if (!std::isfinite(p.A) || p.A <= 0.0) {
        return "Validation Error: Cross-sectional area A must be positive and finite (received: " + std::to_string(p.A) + " m^2). Fix: Set A > 0.";
    }
    if (!std::isfinite(p.k) || p.k <= 0.0) {
        return "Validation Error: Thermal conductivity k must be positive and finite (received: " + std::to_string(p.k) + " W/(m*K)). Fix: Set k > 0.";
    }
    if (!std::isfinite(p.q)) {
        return "Validation Error: Heat generation rate q must be finite (received non-finite value). Fix: Provide a finite real number.";
    }
    if (!std::isfinite(p.T0) || !std::isfinite(p.TL)) {
        return "Validation Error: Boundary temperatures T0 and TL must be finite real numbers.";
    }
    if (p.N < 1) {
        return "Validation Error: Number of elements N must be >= 1 (received: " + std::to_string(p.N) + "). Fix: Set integer N >= 1.";
    }
    return ""; // Valid
}

double exact_temperature_1d(double x, const Thermal1DParams& p) {
    // T_exact(x) = T0 - (T0 - TL)*(x/L) + [q/(2k)] * x * (L - x)
    const double linear_part = p.T0 - (p.T0 - p.TL) * (x / p.L);
    const double parabolic_part = (p.q / (2.0 * p.k)) * x * (p.L - x);
    return linear_part + parabolic_part;
}

Thermal1DResult solve_thermal_1d(const Thermal1DParams& p) {
    const std::string err = validate_thermal_params(p);
    if (!err.empty()) {
        throw std::invalid_argument(err);
    }

    Thermal1DResult res;
    res.params = p;
    const int N = p.N;
    const double h = p.L / static_cast<double>(N);
    res.h = h;

    const int num_nodes = N + 1;
    res.x_nodes.resize(num_nodes);
    res.T_nodes.resize(num_nodes);
    res.T_exact_nodes.resize(num_nodes);

    for (int i = 0; i < num_nodes; ++i) {
        res.x_nodes[i] = i * h;
        res.T_exact_nodes[i] = exact_temperature_1d(res.x_nodes[i], p);
    }
    // Ensure boundary coordinate precision
    res.x_nodes[N] = p.L;
    res.T_exact_nodes[N] = p.TL;

    // Prescribed boundary conditions
    res.T_nodes[0] = p.T0;
    res.T_nodes[N] = p.TL;

    if (N == 1) {
        // Direct two-node solution (no interior nodes)
        // Midpoint
        res.x_midpoints.push_back(0.5 * p.L);
        res.T_mid_fe.push_back(0.5 * (res.T_nodes[0] + res.T_nodes[1]));
        res.T_mid_exact.push_back(exact_temperature_1d(0.5 * p.L, p));
        res.max_node_error = 0.0;
        res.max_midpoint_error = std::abs(res.T_mid_fe[0] - res.T_mid_exact[0]);
        return res;
    }

    // Number of interior unknown nodes: M = N - 1 (indices 1 to N-1)
    const int M = N - 1;
    const double kA_over_h = (p.k * p.A) / h;
    const double qAh = p.q * p.A * h;

    // Tridiagonal system: A * T = F
    // A is symmetric tridiagonal: sub-diagonal a, main diagonal d, super-diagonal c
    std::vector<double> a(M, -kA_over_h);      // subdiagonal: a[1] to a[M-1] used
    std::vector<double> d(M, 2.0 * kA_over_h); // main diagonal
    std::vector<double> c(M, -kA_over_h);      // superdiagonal: c[0] to c[M-2] used
    std::vector<double> F(M, qAh);             // RHS load vector

    // Boundary condition elimination into RHS:
    // Equation 0 (node 1): -kA/h * T0 + 2kA/h * T1 - kA/h * T2 = qAh
    // => 2kA/h * T1 - kA/h * T2 = qAh + kA/h * T0
    F[0] += kA_over_h * p.T0;

    // Equation M-1 (node N-1): -kA/h * T_{N-2} + 2kA/h * T_{N-1} - kA/h * TL = qAh
    // => -kA/h * T_{N-2} + 2kA/h * T_{N-1} = qAh + kA/h * TL
    F[M - 1] += kA_over_h * p.TL;

    // Thomas algorithm (Tridiagonal matrix solver)
    // Forward elimination
    std::vector<double> c_prime(M, 0.0);
    std::vector<double> d_prime(M, 0.0);

    c_prime[0] = c[0] / d[0];
    d_prime[0] = F[0] / d[0];

    for (int i = 1; i < M; ++i) {
        const double denom = d[i] - a[i] * c_prime[i - 1];
        if (i < M - 1) {
            c_prime[i] = c[i] / denom;
        }
        d_prime[i] = (F[i] - a[i] * d_prime[i - 1]) / denom;
    }

    // Back substitution
    std::vector<double> T_int(M, 0.0);
    T_int[M - 1] = d_prime[M - 1];
    for (int i = M - 2; i >= 0; --i) {
        T_int[i] = d_prime[i] - c_prime[i] * T_int[i + 1];
    }

    // Assign interior temperatures back to full array
    for (int i = 0; i < M; ++i) {
        res.T_nodes[i + 1] = T_int[i];
    }

    // Compute nodal errors
    res.max_node_error = 0.0;
    for (int i = 0; i < num_nodes; ++i) {
        const double err_i = std::abs(res.T_nodes[i] - res.T_exact_nodes[i]);
        if (err_i > res.max_node_error) {
            res.max_node_error = err_i;
        }
    }

    // Compute midpoint evaluations using FE linear interpolation
    res.x_midpoints.resize(N);
    res.T_mid_fe.resize(N);
    res.T_mid_exact.resize(N);
    res.max_midpoint_error = 0.0;

    for (int e = 0; e < N; ++e) {
        const double x_m = 0.5 * (res.x_nodes[e] + res.x_nodes[e + 1]);
        const double T_fe_m = 0.5 * (res.T_nodes[e] + res.T_nodes[e + 1]); // linear element interpolation at midpoint
        const double T_ex_m = exact_temperature_1d(x_m, p);

        res.x_midpoints[e] = x_m;
        res.T_mid_fe[e] = T_fe_m;
        res.T_mid_exact[e] = T_ex_m;

        const double err_m = std::abs(T_fe_m - T_ex_m);
        if (err_m > res.max_midpoint_error) {
            res.max_midpoint_error = err_m;
        }
    }

    return res;
}

std::vector<ConvergenceRow> run_convergence_study(
    const Thermal1DParams& base_params, 
    const std::vector<int>& N_values
) {
    std::vector<ConvergenceRow> rows;
    rows.reserve(N_values.size());

    for (size_t i = 0; i < N_values.size(); ++i) {
        Thermal1DParams p = base_params;
        p.N = N_values[i];
        Thermal1DResult res = solve_thermal_1d(p);

        ConvergenceRow row;
        row.N = p.N;
        row.h = res.h;
        row.max_node_error = res.max_node_error;
        row.max_midpoint_error = res.max_midpoint_error;
        row.observed_order = 0.0;

        if (i > 0) {
            const double prev_h = rows[i - 1].h;
            const double prev_err = rows[i - 1].max_midpoint_error;
            if (prev_err > 1.0e-15 && row.max_midpoint_error > 1.0e-15) {
                // observed order p = log(E_prev / E_curr) / log(h_prev / h_curr)
                row.observed_order = std::log(prev_err / row.max_midpoint_error) / std::log(prev_h / row.h);
            }
        }
        rows.push_back(row);
    }

    return rows;
}

std::string export_profiles_csv(const std::vector<Thermal1DResult>& results) {
    std::ostringstream oss;
    oss << "N,node_index,x_m,T_fe_C,T_exact_C,error_C\n";
    oss << std::setprecision(10) << std::fixed;

    for (const auto& res : results) {
        for (size_t i = 0; i < res.x_nodes.size(); ++i) {
            const double err = std::abs(res.T_nodes[i] - res.T_exact_nodes[i]);
            oss << res.params.N << ","
                << i << ","
                << res.x_nodes[i] << ","
                << res.T_nodes[i] << ","
                << res.T_exact_nodes[i] << ","
                << err << "\n";
        }
    }
    return oss.str();
}

std::string export_convergence_csv(const std::vector<ConvergenceRow>& rows) {
    std::ostringstream oss;
    oss << "N,h_m,max_node_error_C,max_midpoint_error_C,observed_order\n";
    oss << std::setprecision(10) << std::scientific;

    for (const auto& r : rows) {
        oss << r.N << ","
            << r.h << ","
            << r.max_node_error << ","
            << r.max_midpoint_error << ","
            << std::setprecision(4) << std::fixed << r.observed_order << "\n";
    }
    return oss.str();
}

bool save_string_to_file(const std::string& filepath, const std::string& content) {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << content;
    return ofs.good();
}

} // namespace workbench::core
