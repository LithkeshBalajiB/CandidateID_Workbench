#pragma once

#include "core/thermal_fea.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

namespace workbench::core {

/**
 * @brief Configuration for generating a deterministic, reproducible batch of independent thermal solves.
 */
struct BatchConfig {
    uint32_t seed = 42;             ///< Fixed random seed for reproducibility
    int num_cases = 1000;           ///< Number of independent thermal solve cases
    int N_elements = 100;          ///< Mesh size per rod
    double L = 1.0;                 ///< Rod length (m)
    double A = 1.0e-4;              ///< Cross-sectional area (m^2)
    double k_min = 1.0;             ///< Minimum thermal conductivity W/(m*K)
    double k_max = 50.0;            ///< Maximum thermal conductivity W/(m*K)
    double q_min = 1000.0;          ///< Minimum heat generation W/m^3
    double q_max = 50000.0;         ///< Maximum heat generation W/m^3
    double T0 = 100.0;              ///< Boundary temp at x = 0 (deg C)
    double TL = 20.0;               ///< Boundary temp at x = L (deg C)
};

/**
 * @brief Benchmark timing metrics for a given worker thread count.
 */
struct BenchmarkRecord {
    int workers = 1;
    double warmup_time_ms = 0.0;
    std::vector<double> repetition_times_ms;
    double median_time_ms = 0.0;
    double speedup = 1.0;            ///< T1 / Tp
    double efficiency = 1.0;         ///< (T1 / Tp) / p
    double max_agreement_diff_C = 0.0; ///< Max abs diff vs serial solution
};

/**
 * @brief System hardware environment summary.
 */
struct HardwareInfo {
    std::string cpu_model;
    int physical_cores = 0;
    int logical_cores = 0;
    double total_ram_gb = 0.0;
    std::string compiler_info;
    std::string optimization_flags;
};

/**
 * @brief Generates reproducible batch of cases with strictly positive k and q.
 */
std::vector<Thermal1DParams> generate_reproducible_batch(const BatchConfig& config);

/**
 * @brief Executes batch of solves serially on a single thread.
 */
std::vector<Thermal1DResult> solve_batch_serial(const std::vector<Thermal1DParams>& cases);

/**
 * @brief Executes batch of solves using a bounded worker pool of std::thread.
 * Results are stored in deterministic input order.
 */
std::vector<Thermal1DResult> solve_batch_parallel(const std::vector<Thermal1DParams>& cases, int num_workers);

/**
 * @brief Compares two sets of batch results and returns the maximum absolute temperature difference.
 */
double compute_max_agreement_diff(const std::vector<Thermal1DResult>& r1, const std::vector<Thermal1DResult>& r2);

/**
 * @brief Detects hardware information on Windows platform.
 */
HardwareInfo get_hardware_environment();

/**
 * @brief Runs standard benchmark protocol: 1 warmup, 5 repetitions, median time, for p in {1, 2, 4}.
 */
std::vector<BenchmarkRecord> run_scaling_benchmark(
    const std::vector<Thermal1DParams>& cases,
    const std::vector<int>& worker_counts
);

/**
 * @brief Exports benchmark results to CSV format.
 */
std::string export_benchmark_csv(
    const HardwareInfo& hw,
    const BatchConfig& cfg,
    const std::vector<BenchmarkRecord>& records
);

} // namespace workbench::core
