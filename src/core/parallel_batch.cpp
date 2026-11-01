#include "core/parallel_batch.hpp"

#include <random>
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace workbench::core {

std::vector<Thermal1DParams> generate_reproducible_batch(const BatchConfig& config) {
    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<double> dist_k(config.k_min, config.k_max);
    std::uniform_real_distribution<double> dist_q(config.q_min, config.q_max);

    std::vector<Thermal1DParams> cases;
    cases.reserve(config.num_cases);

    for (int i = 0; i < config.num_cases; ++i) {
        Thermal1DParams p;
        p.L = config.L;
        p.A = config.A;
        p.k = dist_k(rng);
        p.q = dist_q(rng);
        p.T0 = config.T0;
        p.TL = config.TL;
        p.N = config.N_elements;
        cases.push_back(p);
    }
    return cases;
}

std::vector<Thermal1DResult> solve_batch_serial(const std::vector<Thermal1DParams>& cases) {
    std::vector<Thermal1DResult> results;
    results.reserve(cases.size());

    for (const auto& c : cases) {
        results.push_back(solve_thermal_1d(c));
    }
    return results;
}

std::vector<Thermal1DResult> solve_batch_parallel(const std::vector<Thermal1DParams>& cases, int num_workers) {
    if (cases.empty()) {
        return {};
    }
    if (num_workers <= 1) {
        return solve_batch_serial(cases);
    }

    const size_t total_cases = cases.size();
    num_workers = std::min(num_workers, static_cast<int>(total_cases));

    std::vector<Thermal1DResult> results(total_cases);
    std::atomic<size_t> next_index{0};

    auto worker_task = [&]() {
        while (true) {
            size_t idx = next_index.fetch_add(1, std::memory_order_relaxed);
            if (idx >= total_cases) {
                break;
            }
            results[idx] = solve_thermal_1d(cases[idx]);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(worker_task);
    }

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    return results;
}

double compute_max_agreement_diff(const std::vector<Thermal1DResult>& r1, const std::vector<Thermal1DResult>& r2) {
    if (r1.size() != r2.size()) {
        return -1.0;
    }

    double max_diff = 0.0;
    for (size_t i = 0; i < r1.size(); ++i) {
        const auto& nodes1 = r1[i].T_nodes;
        const auto& nodes2 = r2[i].T_nodes;
        if (nodes1.size() != nodes2.size()) {
            return -1.0;
        }
        for (size_t j = 0; j < nodes1.size(); ++j) {
            const double diff = std::abs(nodes1[j] - nodes2[j]);
            if (diff > max_diff) {
                max_diff = diff;
            }
        }
    }
    return max_diff;
}

HardwareInfo get_hardware_environment() {
    HardwareInfo hw;
    hw.logical_cores = static_cast<int>(std::thread::hardware_concurrency());
    hw.physical_cores = hw.logical_cores / 2; // conservative fallback

#if defined(_WIN32)
    // CPU Brand / Name via Windows Registry or CPUID
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD size = sizeof(buffer);
        if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS) {
            hw.cpu_model = std::string(buffer);
        }
        RegCloseKey(hKey);
    }
    if (hw.cpu_model.empty()) {
        hw.cpu_model = "x86_64 Processor";
    }

    // Total System RAM
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        hw.total_ram_gb = static_cast<double>(memStatus.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
    }
#else
    hw.cpu_model = "Generic x86_64";
    hw.total_ram_gb = 16.0;
#endif

    // Compiler information
#if defined(__GNUC__)
    std::ostringstream comp;
    comp << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
    hw.compiler_info = comp.str();
#elif defined(_MSC_VER)
    std::ostringstream comp;
    comp << "MSVC " << _MSC_VER;
    hw.compiler_info = comp.str();
#else
    hw.compiler_info = "Standard C++17 Compiler";
#endif

    hw.optimization_flags = "-O3 -static -static-libgcc -static-libstdc++";
    return hw;
}

std::vector<BenchmarkRecord> run_scaling_benchmark(
    const std::vector<Thermal1DParams>& cases,
    const std::vector<int>& worker_counts
) {
    std::vector<BenchmarkRecord> records;
    records.reserve(worker_counts.size());

    // Generate baseline serial result for agreement verification
    auto serial_baseline = solve_batch_serial(cases);

    double T1_median = 0.0;

    for (int p : worker_counts) {
        BenchmarkRecord rec;
        rec.workers = p;

        // 1. One Warm-up run (excluded from statistics)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            auto res_warmup = solve_batch_parallel(cases, p);
            auto t1 = std::chrono::high_resolution_clock::now();
            rec.warmup_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        // 2. FIVE measured repetitions
        std::vector<Thermal1DResult> latest_results;
        for (int rep = 0; rep < 5; ++rep) {
            auto t0 = std::chrono::high_resolution_clock::now();
            latest_results = solve_batch_parallel(cases, p);
            auto t1 = std::chrono::high_resolution_clock::now();
            double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            rec.repetition_times_ms.push_back(elapsed_ms);
        }

        // 3. Compute MEDIAN time
        std::vector<double> sorted_times = rec.repetition_times_ms;
        std::sort(sorted_times.begin(), sorted_times.end());
        rec.median_time_ms = sorted_times[2]; // Median of 5 elements is at index 2

        // Record T1 for speedup calculations
        if (p == 1) {
            T1_median = rec.median_time_ms;
        }

        rec.speedup = (T1_median > 0.0 && rec.median_time_ms > 0.0) ? (T1_median / rec.median_time_ms) : 1.0;
        rec.efficiency = (p > 0) ? (rec.speedup / static_cast<double>(p)) : 1.0;

        // Agreement verification vs serial baseline
        rec.max_agreement_diff_C = compute_max_agreement_diff(serial_baseline, latest_results);

        records.push_back(rec);
    }

    return records;
}

std::string export_benchmark_csv(
    const HardwareInfo& hw,
    const BatchConfig& cfg,
    const std::vector<BenchmarkRecord>& records
) {
    std::ostringstream oss;
    oss << "# Hardware: " << hw.cpu_model << " | " << hw.logical_cores << " Logical Processors | "
        << std::fixed << std::setprecision(1) << hw.total_ram_gb << " GB RAM\n";
    oss << "# Compiler: " << hw.compiler_info << " | Flags: " << hw.optimization_flags << "\n";
    oss << "# Workload: cases=" << cfg.num_cases << ", mesh_N=" << cfg.N_elements << ", seed=" << cfg.seed << "\n";
    oss << "workers,warmup_ms,rep1_ms,rep2_ms,rep3_ms,rep4_ms,rep5_ms,median_ms,speedup,efficiency,max_diff_C\n";
    oss << std::fixed << std::setprecision(4);

    for (const auto& r : records) {
        oss << r.workers << ","
            << r.warmup_time_ms << ",";
        for (double t : r.repetition_times_ms) {
            oss << t << ",";
        }
        oss << r.median_time_ms << ","
            << r.speedup << ","
            << r.efficiency << ","
            << std::scientific << std::setprecision(6) << r.max_agreement_diff_C << "\n";
        oss << std::fixed << std::setprecision(4);
    }
    return oss.str();
}

} // namespace workbench::core
