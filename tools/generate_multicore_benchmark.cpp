#include "core/parallel_batch.hpp"
#include "core/thermal_fea.hpp"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>

int main() {
    using namespace workbench::core;

    std::cout << "=========================================================\n";
    std::cout << "Task D: Multicore / Multithreaded Execution Benchmark\n";
    std::cout << "=========================================================\n";

    HardwareInfo hw = get_hardware_environment();
    std::cout << "\n1. HARDWARE & ENVIRONMENT REPORT:\n";
    std::cout << "   CPU Model         : " << hw.cpu_model << "\n";
    std::cout << "   Logical Processors: " << hw.logical_cores << "\n";
    std::cout << "   Physical Cores    : " << hw.physical_cores << "\n";
    std::cout << "   System RAM        : " << std::fixed << std::setprecision(2) << hw.total_ram_gb << " GB\n";
    std::cout << "   Compiler          : " << hw.compiler_info << "\n";
    std::cout << "   Optimization Flags: " << hw.optimization_flags << "\n";

    // Workload setup: 2000 independent thermal solves, N = 100 elements
    BatchConfig cfg;
    cfg.seed = 20261031;
    cfg.num_cases = 2000;
    cfg.N_elements = 100;

    std::cout << "\n2. WORKLOAD CONFIGURATION:\n";
    std::cout << "   Independent Solves: " << cfg.num_cases << "\n";
    std::cout << "   Mesh Size per Rod : N = " << cfg.N_elements << " elements\n";
    std::cout << "   Random Seed       : " << cfg.seed << "\n";
    std::cout << "   Conductivity (k)  : [" << cfg.k_min << ", " << cfg.k_max << "] W/(m*K)\n";
    std::cout << "   Heat Gen (q)      : [" << cfg.q_min << ", " << cfg.q_max << "] W/m^3\n";

    std::cout << "\n3. GENERATING BATCH WORKLOAD (Excluded from compute timing)...\n";
    auto cases = generate_reproducible_batch(cfg);
    std::cout << "   Generated " << cases.size() << " cases.\n";

    // Test workers: 1, 2, and 4 (capped at logical cores)
    std::vector<int> workers = {1, 2, 4};
    int max_avail = hw.logical_cores > 0 ? hw.logical_cores : 4;
    for (auto it = workers.begin(); it != workers.end(); ) {
        if (*it > max_avail) {
            it = workers.erase(it);
        } else {
            ++it;
        }
    }
    if (workers.empty()) {
        workers.push_back(1);
    }

    std::cout << "\n4. RUNNING BENCHMARK PROTOCOL:\n";
    std::cout << "   Procedure: 1 Warm-up run, 5 Measured repetitions, Median reported.\n";
    std::cout << "   Exclude input generation, memory allocation, and disk I/O.\n\n";

    auto records = run_scaling_benchmark(cases, workers);

    std::cout << "----------------------------------------------------------------------------------------------------\n";
    std::cout << std::setw(8) << "Workers"
              << std::setw(12) << "Warmup (ms)"
              << std::setw(14) << "Median (ms)"
              << std::setw(12) << "Speedup"
              << std::setw(14) << "Efficiency"
              << std::setw(20) << "Max |T_s - T_p| (C)"
              << std::setw(16) << "Agreement\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    for (const auto& r : records) {
        std::cout << std::setw(8) << r.workers
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.warmup_time_ms
                  << std::setw(14) << r.median_time_ms
                  << std::setw(12) << std::setprecision(2) << r.speedup << "x"
                  << std::setw(14) << std::setprecision(1) << (r.efficiency * 100.0) << "%"
                  << std::setw(20) << std::scientific << std::setprecision(3) << r.max_agreement_diff_C
                  << std::setw(16) << (r.max_agreement_diff_C <= 1.0e-8 ? "PASS (<=1e-8)" : "FAIL")
                  << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    std::filesystem::create_directories("results");
    std::string csv_data = export_benchmark_csv(hw, cfg, records);
    if (save_string_to_file("results/performance_benchmark.csv", csv_data)) {
        std::cout << "\n[SUCCESS] Exported raw benchmark measurements to results/performance_benchmark.csv\n";
    } else {
        std::cerr << "\n[ERROR] Failed to save results/performance_benchmark.csv\n";
    }

    return 0;
}
