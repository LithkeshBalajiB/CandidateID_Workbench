#include "core/cad_ansys_parser.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

namespace {

size_t get_peak_rss_kb() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize / 1024;
    }
#endif
    return 0;
}

} // anonymous namespace

int main() {
    using namespace workbench::core;

    std::cout << "=========================================================\n";
    std::cout << "Task B: File-Size Limits & Parser Stress Test Harness\n";
    std::cout << "=========================================================\n";

    std::filesystem::create_directories("scratch");

    // 1. Stress Test CSV Parser with large nodal files: 10K, 100K, 500K nodes
    std::cout << "\n1. STRESS TESTING ANSYS CSV PARSER:\n";
    std::cout << "------------------------------------------------------------------------------------\n";
    std::cout << std::setw(12) << "Node Count"
              << std::setw(16) << "File Size (MB)"
              << std::setw(18) << "Parse Time (ms)"
              << std::setw(18) << "Throughput (MB/s)"
              << std::setw(16) << "Peak RSS (MB)\n";
    std::cout << "------------------------------------------------------------------------------------\n";

    std::vector<size_t> csv_counts = {10000, 100000, 500000};
    for (size_t count : csv_counts) {
        std::string filename = "scratch/temp_stress_" + std::to_string(count) + ".csv";
        {
            std::ofstream ofs(filename);
            ofs << "node_id,x_m,y_m,z_m,temperature_C\n";
            for (size_t i = 1; i <= count; ++i) {
                ofs << i << ",0.123,0.456,0.789,125.50\n";
            }
        }

        uintmax_t file_bytes = std::filesystem::file_size(filename);
        double file_mb = static_cast<double>(file_bytes) / (1024.0 * 1024.0);

        ANSYSPointCloud cloud;
        auto t0 = std::chrono::high_resolution_clock::now();
        auto res = parse_ansys_csv_file(filename, cloud);
        auto t1 = std::chrono::high_resolution_clock::now();

        double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double throughput_mb_s = (elapsed_ms > 0.0) ? (file_mb / (elapsed_ms / 1000.0)) : 0.0;
        double peak_mb = static_cast<double>(get_peak_rss_kb()) / 1024.0;

        if (res.is_valid && cloud.node_count == count) {
            std::cout << std::setw(12) << count
                      << std::setw(16) << std::fixed << std::setprecision(2) << file_mb
                      << std::setw(18) << std::setprecision(2) << elapsed_ms
                      << std::setw(18) << std::setprecision(1) << throughput_mb_s
                      << std::setw(16) << std::setprecision(1) << peak_mb
                      << "\n";
        } else {
            std::cout << "[ERROR] Validation failed for " << count << " nodes: " << res.error_message << "\n";
        }

        std::filesystem::remove(filename);
    }
    std::cout << "------------------------------------------------------------------------------------\n";

    // 2. Stress Test ASCII STL Parser: 10K, 50K facets
    std::cout << "\n2. STRESS TESTING ASCII STL PARSER:\n";
    std::cout << "------------------------------------------------------------------------------------\n";
    std::cout << std::setw(14) << "Facet Count"
              << std::setw(16) << "File Size (MB)"
              << std::setw(18) << "Parse Time (ms)"
              << std::setw(18) << "Throughput (MB/s)"
              << std::setw(16) << "Peak RSS (MB)\n";
    std::cout << "------------------------------------------------------------------------------------\n";

    std::vector<size_t> stl_counts = {10000, 50000};
    for (size_t count : stl_counts) {
        std::string filename = "scratch/temp_stress_" + std::to_string(count) + ".stl";
        {
            std::ofstream ofs(filename);
            ofs << "solid StressSolid\n";
            for (size_t i = 0; i < count; ++i) {
                ofs << "  facet normal 0.0 0.0 1.0\n"
                    << "    outer loop\n"
                    << "      vertex 0.0 0.0 0.0\n"
                    << "      vertex 1.0 0.0 0.0\n"
                    << "      vertex 0.5 0.5 0.0\n"
                    << "    endloop\n"
                    << "  endfacet\n";
            }
            ofs << "endsolid StressSolid\n";
        }

        uintmax_t file_bytes = std::filesystem::file_size(filename);
        double file_mb = static_cast<double>(file_bytes) / (1024.0 * 1024.0);

        STLGeometry geom;
        auto t0 = std::chrono::high_resolution_clock::now();
        auto res = parse_ascii_stl_file(filename, geom);
        auto t1 = std::chrono::high_resolution_clock::now();

        double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double throughput_mb_s = (elapsed_ms > 0.0) ? (file_mb / (elapsed_ms / 1000.0)) : 0.0;
        double peak_mb = static_cast<double>(get_peak_rss_kb()) / 1024.0;

        if (res.is_valid && geom.triangle_count == count) {
            std::cout << std::setw(14) << count
                      << std::setw(16) << std::fixed << std::setprecision(2) << file_mb
                      << std::setw(18) << std::setprecision(2) << elapsed_ms
                      << std::setw(18) << std::setprecision(1) << throughput_mb_s
                      << std::setw(16) << std::setprecision(1) << peak_mb
                      << "\n";
        } else {
            std::cout << "[ERROR] Validation failed for " << count << " facets: " << res.error_message << "\n";
        }

        std::filesystem::remove(filename);
    }
    std::cout << "------------------------------------------------------------------------------------\n";

    return 0;
}
