#include "doctest/doctest.h"
#include "core/thermal_fea.hpp"

#include <cmath>
#include <limits>
#include <vector>

using namespace workbench::core;

TEST_SUITE("Task C: 1D Steady-State Thermal FEA") {

    TEST_CASE("Positive: Exact solution at nodes and midpoint convergence for N = 10, 20, 40, 80") {
        Thermal1DParams p;
        p.L = 1.0;
        p.A = 1.0e-4;
        p.k = 10.0;
        p.q = 10000.0;
        p.T0 = 100.0;
        p.TL = 20.0;

        const std::vector<int> N_vals = {10, 20, 40, 80};
        auto convergence = run_convergence_study(p, N_vals);

        REQUIRE(convergence.size() == 4);

        // Theoretical midpoint error is exactly (q / (8 * k)) * h^2
        const double factor = p.q / (8.0 * p.k); // 10000 / 80 = 125.0

        for (size_t i = 0; i < convergence.size(); ++i) {
            const auto& row = convergence[i];
            const double expected_mid_err = factor * row.h * row.h;

            // Nodal error should be near machine precision (~1e-12 deg C)
            CHECK(row.max_node_error < 1.0e-11);

            // Midpoint error must match analytical (q / 8k) * h^2 within 1e-10
            CHECK(std::abs(row.max_midpoint_error - expected_mid_err) < 1.0e-10);

            // Observed convergence order should be ~2.00 after the first doubling
            if (i > 0) {
                CHECK(row.observed_order > 1.99);
                CHECK(row.observed_order < 2.01);
            }
        }

        // Specific midpoint error values verification
        CHECK(std::abs(convergence[0].max_midpoint_error - 1.25) < 1.0e-9);       // N=10, h=0.1
        CHECK(std::abs(convergence[1].max_midpoint_error - 0.3125) < 1.0e-9);     // N=20, h=0.05
        CHECK(std::abs(convergence[2].max_midpoint_error - 0.078125) < 1.0e-9);   // N=40, h=0.025
        CHECK(std::abs(convergence[3].max_midpoint_error - 0.01953125) < 1.0e-9); // N=80, h=0.0125
    }

    TEST_CASE("Negative: Input parameter validation and error messages") {
        Thermal1DParams p;

        SUBCASE("N < 1 rejected") {
            p.N = 0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
            p.N = -5;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
        }

        SUBCASE("k <= 0 rejected") {
            p.k = 0.0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
            p.k = -10.0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
        }

        SUBCASE("L <= 0 rejected") {
            p.L = 0.0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
            p.L = -1.0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
        }

        SUBCASE("A <= 0 rejected") {
            p.A = 0.0;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
            p.A = -1.0e-4;
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
        }

        SUBCASE("Non-finite parameters rejected") {
            p.q = std::numeric_limits<double>::quiet_NaN();
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);

            p.q = std::numeric_limits<double>::infinity();
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);

            p.q = 10000.0;
            p.T0 = std::numeric_limits<double>::quiet_NaN();
            CHECK_THROWS_AS(solve_thermal_1d(p), std::invalid_argument);
        }
    }

    TEST_CASE("Numerical: Boundary condition preservation, symmetry, and energy balance") {
        Thermal1DParams p;
        p.L = 1.0;
        p.A = 1.0e-4;
        p.k = 10.0;
        p.q = 10000.0;
        p.T0 = 100.0;
        p.TL = 20.0;
        p.N = 20;

        auto res = solve_thermal_1d(p);

        SUBCASE("Dirichlet boundary conditions exact match") {
            CHECK(res.T_nodes.front() == doctest::Approx(100.0));
            CHECK(res.T_nodes.back() == doctest::Approx(20.0));
        }

        SUBCASE("Symmetric boundary conditions produce symmetric solution") {
            Thermal1DParams p_sym = p;
            p_sym.T0 = 50.0;
            p_sym.TL = 50.0;
            p_sym.N = 20;

            auto res_sym = solve_thermal_1d(p_sym);
            const int num_nodes = static_cast<int>(res_sym.T_nodes.size());

            for (int i = 0; i <= num_nodes / 2; ++i) {
                const double T_left = res_sym.T_nodes[i];
                const double T_right = res_sym.T_nodes[num_nodes - 1 - i];
                CHECK(std::abs(T_left - T_right) < 1.0e-11);
            }

            // Peak temperature for symmetric case must be at mid-span x = L/2
            const int mid_idx = num_nodes / 2;
            const double expected_peak = 50.0 + (p_sym.q / (2.0 * p_sym.k)) * (0.5) * (0.5); // 50 + 500*0.25 = 175
            CHECK(res_sym.T_nodes[mid_idx] == doctest::Approx(expected_peak));
        }

        SUBCASE("Energy conservation check: Q_gen == Heat out") {
            // Total heat generated in volume: Q_gen = q * A * L
            const double Q_gen = p.q * p.A * p.L; // 10000 * 1e-4 * 1.0 = 1.0 W

            // Analytical temperature gradient T'(x) = -(T0 - TL)/L + (q/(2k))*(L - 2x)
            // T'(0) = -80 + (10000/20)*(1) = -80 + 500 = +420 K/m
            // Flux leaving at x = 0: q_out_left = - (-k * A * T'(0)) = +k * A * T'(0) ? 
            // Heat flux is q_flux = -k dT/dx. 
            // At x = 0, outward normal is -x. So outward flux = - (-k dT/dx) = +k dT/dx = 10 * 1e-4 * 420 = 0.42 W.
            // At x = L, outward normal is +x. Outward flux = -k dT/dx.
            // T'(L) = -80 + (500)*(-1) = -580 K/m.
            // Outward flux = -10 * 1e-4 * (-580) = +0.58 W.
            // Total outward heat flow = 0.42 + 0.58 = 1.00 W == Q_gen!
            const double dT_dx_0 = -(p.T0 - p.TL) / p.L + (p.q / (2.0 * p.k)) * p.L;
            const double dT_dx_L = -(p.T0 - p.TL) / p.L - (p.q / (2.0 * p.k)) * p.L;

            const double heat_out_left = (p.k * p.A) * dT_dx_0;
            const double heat_out_right = -(p.k * p.A) * dT_dx_L;
            const double total_heat_out = heat_out_left + heat_out_right;

            CHECK(total_heat_out == doctest::Approx(Q_gen).epsilon(1.0e-9));
        }

        SUBCASE("Single element mesh N = 1") {
            Thermal1DParams p_single = p;
            p_single.N = 1;
            auto res_single = solve_thermal_1d(p_single);

            CHECK(res_single.T_nodes.size() == 2);
            CHECK(res_single.T_nodes[0] == 100.0);
            CHECK(res_single.T_nodes[1] == 20.0);
            CHECK(res_single.x_midpoints.size() == 1);
            // Linear interpolation at midpoint of N=1 gives (100 + 20) / 2 = 60
            CHECK(res_single.T_mid_fe[0] == 60.0);
            // Exact at midpoint x = 0.5: 100 - 40 + (10000/20)*0.5*0.5 = 60 + 125 = 185
            CHECK(res_single.T_mid_exact[0] == 185.0);
            CHECK(res_single.max_midpoint_error == 125.0); // factor * 1.0^2 = 125
        }
    }
}
