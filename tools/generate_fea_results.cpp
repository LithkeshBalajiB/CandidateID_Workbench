#include "core/thermal_fea.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <filesystem>

int main() {
    using namespace workbench::core;

    std::cout << "=========================================================\n";
    std::cout << "1D Thermal FEA Benchmark & Convergence Generation Tool\n";
    std::cout << "=========================================================\n";

    Thermal1DParams p;
    p.L = 1.0;
    p.A = 1.0e-4;
    p.k = 10.0;
    p.q = 10000.0;
    p.T0 = 100.0;
    p.TL = 20.0;

    std::vector<int> N_values = {10, 20, 40, 80};
    std::vector<Thermal1DResult> results;
    results.reserve(N_values.size());

    for (int n : N_values) {
        Thermal1DParams pn = p;
        pn.N = n;
        results.push_back(solve_thermal_1d(pn));
    }

    auto convergence = run_convergence_study(p, N_values);

    std::cout << "\nConvergence Study Results (Uniform 1D Rod, Linear Elements):\n";
    std::cout << "------------------------------------------------------------------------------------\n";
    std::cout << std::setw(6) << "N"
              << std::setw(12) << "h (m)"
              << std::setw(20) << "Max Node Err (C)"
              << std::setw(22) << "Max Midpoint Err (C)"
              << std::setw(18) << "Observed Order\n";
    std::cout << "------------------------------------------------------------------------------------\n";

    for (const auto& row : convergence) {
        std::cout << std::setw(6) << row.N
                  << std::setw(12) << std::fixed << std::setprecision(4) << row.h
                  << std::setw(20) << std::scientific << std::setprecision(4) << row.max_node_error
                  << std::setw(22) << std::fixed << std::setprecision(6) << row.max_midpoint_error
                  << std::setw(18) << std::setprecision(4) << (row.observed_order > 0.0 ? std::to_string(row.observed_order) : "-")
                  << "\n";
    }
    std::cout << "------------------------------------------------------------------------------------\n";

    // Ensure results directory exists
    std::filesystem::create_directories("results");

    std::string profiles_csv = export_profiles_csv(results);
    std::string convergence_csv = export_convergence_csv(convergence);

    if (save_string_to_file("results/fea_profiles.csv", profiles_csv)) {
        std::cout << "[SUCCESS] Wrote profiles to results/fea_profiles.csv\n";
    } else {
        std::cerr << "[ERROR] Failed to write results/fea_profiles.csv\n";
    }

    if (save_string_to_file("results/fea_convergence.csv", convergence_csv)) {
        std::cout << "[SUCCESS] Wrote convergence table to results/fea_convergence.csv\n";
    } else {
        std::cerr << "[ERROR] Failed to write results/fea_convergence.csv\n";
    }

    return 0;
}
