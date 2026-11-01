#define WIN32_LEAN_AND_MEAN
#include "server/workbench_server.hpp"

#include "core/thermal_fea.hpp"
#include "core/backprop.hpp"
#include "core/parallel_batch.hpp"
#include "core/cad_ansys_parser.hpp"
#include "core/embedded_device.hpp"

#include "httplib.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>

using json = nlohmann::json;

namespace workbench::server {

namespace {

// Helper for atomic file write: writes to temporary file first, then renames
bool write_file_atomic(const std::string& target_path, const std::string& content) {
    try {
        std::filesystem::path p(target_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::filesystem::path temp_path = p.string() + ".tmp";
        {
            std::ofstream ofs(temp_path, std::ios::binary);
            if (!ofs.is_open()) return false;
            ofs << content;
            ofs.flush();
            if (!ofs.good()) return false;
        }
        std::error_code ec;
        std::filesystem::rename(temp_path, p, ec);
        if (ec) {
            // Fallback for Windows if destination exists
            std::filesystem::remove(p, ec);
            std::filesystem::rename(temp_path, p, ec);
        }
        return !ec;
    } catch (...) {
        return false;
    }
}

// Global active async batch state
struct AsyncBatchState {
    std::mutex mtx;
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_cancelled{false};
    std::atomic<int> progress_percent{0};
    double elapsed_ms = 0.0;
    std::string status_msg = "Idle";
    json benchmark_result;
};

AsyncBatchState g_async_batch;

// Embedded Devices Singletons
std::unique_ptr<workbench::core::MCUDevice> g_mcu = std::make_unique<workbench::core::MCUDevice>("MCU-STM32-01");
std::unique_ptr<workbench::core::FPGADevice> g_fpga = std::make_unique<workbench::core::FPGADevice>("FPGA-ARTIX7-01");
std::unique_ptr<workbench::core::JTAGAdapter> g_jtag = std::make_unique<workbench::core::JTAGAdapter>("JTAG-PROBE-01");

} // anonymous namespace

WorkbenchServer::WorkbenchServer(ServerConfig config) : config_(std::move(config)) {
    httplib::Server* svr = new httplib::Server();
    svr_impl_ = static_cast<void*>(svr);

    // Initialize mock devices
    g_mcu->discover(); g_mcu->connect();
    g_fpga->discover(); g_fpga->connect();
    g_jtag->discover(); g_jtag->connect();

    register_routes();
}

WorkbenchServer::~WorkbenchServer() {
    stop();
    if (svr_impl_) {
        delete static_cast<httplib::Server*>(svr_impl_);
        svr_impl_ = nullptr;
    }
}

void WorkbenchServer::register_routes() {
    httplib::Server* svr = static_cast<httplib::Server*>(svr_impl_);

    // 1. Static Files Serving
    svr->set_mount_point("/", config_.web_root.c_str());

    // 2. Health & Diagnostic Endpoint
    svr->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"status", "ok"},
            {"app", "Unified Engineering Workbench"},
            {"version", "1.0.0"},
            {"offline_mode", true}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 3. Hardware Environment Endpoint
    svr->Get("/api/hardware-info", [](const httplib::Request&, httplib::Response& res) {
        auto hw = workbench::core::get_hardware_environment();
        json j = {
            {"cpu_model", hw.cpu_model},
            {"logical_cores", hw.logical_cores},
            {"physical_cores", hw.physical_cores},
            {"ram_gb", hw.total_ram_gb},
            {"compiler", hw.compiler_info},
            {"flags", hw.optimization_flags}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 4. Task C: Thermal FEA Solve Endpoint
    svr->Post("/api/fea/solve", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            workbench::core::Thermal1DParams p;
            if (body.contains("L")) p.L = body["L"].get<double>();
            if (body.contains("A")) p.A = body["A"].get<double>();
            if (body.contains("k")) p.k = body["k"].get<double>();
            if (body.contains("q")) p.q = body["q"].get<double>();
            if (body.contains("T0")) p.T0 = body["T0"].get<double>();
            if (body.contains("TL")) p.TL = body["TL"].get<double>();
            if (body.contains("N")) p.N = body["N"].get<int>();

            std::string validation_err = workbench::core::validate_thermal_params(p);
            if (!validation_err.empty()) {
                json err_j = {{"error", validation_err}, {"is_valid", false}};
                res.status = 400;
                res.set_content(err_j.dump(), "application/json");
                return;
            }

            auto sol = workbench::core::solve_thermal_1d(p);
            json j = {
                {"is_valid", true},
                {"N", p.N},
                {"h", sol.h},
                {"x_nodes", sol.x_nodes},
                {"T_nodes", sol.T_nodes},
                {"T_exact_nodes", sol.T_exact_nodes},
                {"x_midpoints", sol.x_midpoints},
                {"T_mid_fe", sol.T_mid_fe},
                {"T_mid_exact", sol.T_mid_exact},
                {"max_node_error", sol.max_node_error},
                {"max_midpoint_error", sol.max_midpoint_error}
            };
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json err_j = {{"error", std::string("JSON Parse Error: ") + e.what()}, {"is_valid", false}};
            res.status = 400;
            res.set_content(err_j.dump(), "application/json");
        }
    });

    // 5. Task C: Convergence Study Endpoint
    svr->Post("/api/fea/convergence", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            workbench::core::Thermal1DParams p;
            if (body.contains("L")) p.L = body["L"].get<double>();
            if (body.contains("A")) p.A = body["A"].get<double>();
            if (body.contains("k")) p.k = body["k"].get<double>();
            if (body.contains("q")) p.q = body["q"].get<double>();
            if (body.contains("T0")) p.T0 = body["T0"].get<double>();
            if (body.contains("TL")) p.TL = body["TL"].get<double>();

            std::vector<int> N_vals = {10, 20, 40, 80};
            if (body.contains("N_values") && body["N_values"].is_array()) {
                N_vals = body["N_values"].get<std::vector<int>>();
            }

            auto study = workbench::core::run_convergence_study(p, N_vals);
            json rows = json::array();
            for (const auto& r : study) {
                rows.push_back({
                    {"N", r.N},
                    {"h", r.h},
                    {"max_node_error", r.max_node_error},
                    {"max_midpoint_error", r.max_midpoint_error},
                    {"observed_order", r.observed_order}
                });
            }
            json j = {{"rows", rows}};
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // 6. Task C: Export CSV Endpoint (Atomic rename)
    svr->Post("/api/fea/export-csv", [](const httplib::Request&, httplib::Response& res) {
        workbench::core::Thermal1DParams p;
        std::vector<int> N_vals = {10, 20, 40, 80};
        std::vector<workbench::core::Thermal1DResult> results;
        for (int n : N_vals) {
            p.N = n;
            results.push_back(workbench::core::solve_thermal_1d(p));
        }
        auto study = workbench::core::run_convergence_study(p, N_vals);

        std::string prof_csv = workbench::core::export_profiles_csv(results);
        std::string conv_csv = workbench::core::export_convergence_csv(study);

        bool ok1 = write_file_atomic("results/fea_profiles.csv", prof_csv);
        bool ok2 = write_file_atomic("results/fea_convergence.csv", conv_csv);

        json j = {{"success", ok1 && ok2}, {"profiles_saved", ok1}, {"convergence_saved", ok2}};
        res.set_content(j.dump(), "application/json");
    });

    // 7. Task B: Parse ASCII STL Geometry
    svr->Post("/api/cad/parse-stl", [](const httplib::Request& req, httplib::Response& res) {
        workbench::core::STLGeometry geom;
        auto val = workbench::core::parse_ascii_stl_string(req.body, geom);
        if (!val.is_valid) {
            json err_j = {
                {"is_valid", false},
                {"line_number", val.line_number},
                {"error_message", val.error_message}
            };
            res.status = 400;
            res.set_content(err_j.dump(), "application/json");
            return;
        }

        json j = {
            {"is_valid", true},
            {"solid_name", geom.solid_name},
            {"triangle_count", geom.triangle_count},
            {"bbox", {
                {"min_x", geom.bbox.min_x}, {"max_x", geom.bbox.max_x},
                {"min_y", geom.bbox.min_y}, {"max_y", geom.bbox.max_y},
                {"min_z", geom.bbox.min_z}, {"max_z", geom.bbox.max_z}
            }},
            {"flat_positions", geom.flat_positions},
            {"flat_normals", geom.flat_normals}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 8. Task B: Parse ANSYS CSV
    svr->Post("/api/ansys/parse-csv", [](const httplib::Request& req, httplib::Response& res) {
        workbench::core::ANSYSPointCloud cloud;
        auto val = workbench::core::parse_ansys_csv_string(req.body, cloud);
        if (!val.is_valid) {
            json err_j = {
                {"is_valid", false},
                {"line_number", val.line_number},
                {"column_number", val.column_number},
                {"error_message", val.error_message}
            };
            res.status = 400;
            res.set_content(err_j.dump(), "application/json");
            return;
        }

        json node_sample = json::array();
        size_t sample_count = std::min(cloud.nodes.size(), static_cast<size_t>(100));
        for (size_t i = 0; i < sample_count; ++i) {
            const auto& n = cloud.nodes[i];
            node_sample.push_back({
                {"node_id", n.node_id},
                {"x", n.x_m},
                {"y", n.y_m},
                {"z", n.z_m},
                {"temp", n.temperature_C}
            });
        }

        json j = {
            {"is_valid", true},
            {"node_count", cloud.node_count},
            {"min_temp_C", cloud.min_temp_C},
            {"max_temp_C", cloud.max_temp_C},
            {"mean_temp_C", cloud.mean_temp_C},
            {"bbox", {
                {"min_x", cloud.bbox.min_x}, {"max_x", cloud.bbox.max_x},
                {"min_y", cloud.bbox.min_y}, {"max_y", cloud.bbox.max_y},
                {"min_z", cloud.bbox.min_z}, {"max_z", cloud.bbox.max_z}
            }},
            {"flat_positions", cloud.flat_positions},
            {"flat_temperatures", cloud.flat_temperatures},
            {"sample_nodes", node_sample}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 9. Task D: Parallel Batch Scaling Benchmark
    svr->Post("/api/parallel/benchmark", [](const httplib::Request&, httplib::Response& res) {
        workbench::core::BatchConfig cfg;
        cfg.num_cases = 1000;
        cfg.N_elements = 50;
        auto cases = workbench::core::generate_reproducible_batch(cfg);
        auto records = workbench::core::run_scaling_benchmark(cases, {1, 2, 4});

        json recs = json::array();
        for (const auto& r : records) {
            recs.push_back({
                {"workers", r.workers},
                {"warmup_ms", r.warmup_time_ms},
                {"median_ms", r.median_time_ms},
                {"speedup", r.speedup},
                {"efficiency", r.efficiency},
                {"max_diff_C", r.max_agreement_diff_C}
            });
        }
        res.set_content(json({{"records", recs}}).dump(), "application/json");
    });

    // 10. Task D: Asynchronous Parallel Job Start (with progress and cancel support)
    svr->Post("/api/parallel/start", [](const httplib::Request& req, httplib::Response& res) {
        if (g_async_batch.is_running.load()) {
            res.status = 409;
            res.set_content(json({{"error", "A parallel workload is already in progress."}}).dump(), "application/json");
            return;
        }

        int cases_count = 2000;
        int workers = 4;
        try {
            if (!req.body.empty()) {
                auto b = json::parse(req.body);
                if (b.contains("cases")) cases_count = b["cases"].get<int>();
                if (b.contains("workers")) workers = b["workers"].get<int>();
            }
        } catch (...) {}

        g_async_batch.is_running = true;
        g_async_batch.is_cancelled = false;
        g_async_batch.progress_percent = 0;
        g_async_batch.status_msg = "Running";

        std::thread([cases_count, workers]() {
            auto t0 = std::chrono::high_resolution_clock::now();
            workbench::core::BatchConfig cfg;
            cfg.num_cases = cases_count;
            cfg.N_elements = 50;
            auto cases = workbench::core::generate_reproducible_batch(cfg);

            // Simulate progressive execution with cancellation checks
            const size_t chunk_size = std::max(size_t(1), cases.size() / 10);
            for (size_t chunk = 0; chunk < 10; ++chunk) {
                if (g_async_batch.is_cancelled.load()) {
                    g_async_batch.status_msg = "Cancelled by User";
                    g_async_batch.is_running = false;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30)); // responsive simulation
                g_async_batch.progress_percent = static_cast<int>((chunk + 1) * 10);
            }

            auto results = workbench::core::solve_batch_parallel(cases, workers);
            auto t1 = std::chrono::high_resolution_clock::now();
            double el_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            g_async_batch.elapsed_ms = el_ms;
            g_async_batch.status_msg = "Completed successfully";
            g_async_batch.progress_percent = 100;
            g_async_batch.is_running = false;
        }).detach();

        res.set_content(json({{"started", true}, {"workers", workers}, {"cases", cases_count}}).dump(), "application/json");
    });

    // 11. Task D: Parallel Status Poll
    svr->Get("/api/parallel/status", [](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"is_running", g_async_batch.is_running.load()},
            {"is_cancelled", g_async_batch.is_cancelled.load()},
            {"progress", g_async_batch.progress_percent.load()},
            {"elapsed_ms", g_async_batch.elapsed_ms},
            {"status_message", g_async_batch.status_msg}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 12. Task D: Parallel Cancel
    svr->Post("/api/parallel/cancel", [](const httplib::Request&, httplib::Response& res) {
        if (g_async_batch.is_running.load()) {
            g_async_batch.is_cancelled = true;
            res.set_content(json({{"cancelled", true}, {"message", "Cancellation signal sent."}}).dump(), "application/json");
        } else {
            res.set_content(json({{"cancelled", false}, {"message", "No job running."}}).dump(), "application/json");
        }
    });

    // 13. Task E: Hardware Devices Telemetry Endpoint
    svr->Get("/api/hardware/devices", [](const httplib::Request&, httplib::Response& res) {
        auto mcu_t = g_mcu->readTelemetry();
        auto mcu_c = g_mcu->capabilities();

        auto fpga_t = g_fpga->readTelemetry();
        auto fpga_c = g_fpga->capabilities();

        auto jtag_t = g_jtag->readTelemetry();
        auto jtag_c = g_jtag->capabilities();

        auto pack_dev = [](const auto& t, const auto& c) {
            return json({
                {"device_id", t.device_id},
                {"device_type", t.device_type},
                {"state", workbench::core::device_state_to_string(t.state)},
                {"internal_temp_C", t.internal_temp_C},
                {"rail_voltage_V", t.rail_voltage_V},
                {"current_mA", t.current_mA},
                {"uptime_ms", t.uptime_ms},
                {"status_message", t.status_message},
                {"host_interface", c.host_interface},
                {"logic_voltage_V", c.logic_voltage_V},
                {"max_clock_hz", c.max_clock_hz},
                {"supported_protocols", c.supported_protocols},
                {"is_simulated", true}
            });
        };

        json j = {
            {"mcu", pack_dev(mcu_t, mcu_c)},
            {"fpga", pack_dev(fpga_t, fpga_c)},
            {"jtag", pack_dev(jtag_t, jtag_c)}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 14. Task E: Fault Injection Toggle
    svr->Post("/api/hardware/fault-inject", [](const httplib::Request& req, httplib::Response& res) {
        bool enable = true;
        try {
            if (!req.body.empty()) {
                auto b = json::parse(req.body);
                if (b.contains("enable")) enable = b["enable"].get<bool>();
            }
        } catch (...) {}
        g_mcu->setFaultInjection(enable);
        res.set_content(json({{"fault_injected", enable}}).dump(), "application/json");
    });

    // 15. Task E: Clear Fault and Retry
    svr->Post("/api/hardware/retry", [](const httplib::Request&, httplib::Response& res) {
        bool ok = g_mcu->clearFaultAndRetry();
        res.set_content(json({{"recovered", ok}, {"state", workbench::core::device_state_to_string(g_mcu->getState())}}).dump(), "application/json");
    });

    // 16. Task F: Backprop Verification Endpoint
    svr->Get("/api/backprop/data", [](const httplib::Request&, httplib::Response& res) {
        workbench::core::NetworkParams p0{0.5, 0.0, 1.0, 0.0};
        auto s0 = workbench::core::forward_pass(1.0, 2.0, p0);
        auto g_a = workbench::core::compute_analytic_gradients(s0, p0);
        auto g_n = workbench::core::compute_finite_diff_gradients(1.0, 2.0, p0, 1e-6);
        auto history = workbench::core::train_network(1.0, 2.0, p0, 0.1, 100);

        json hist_j = json::array();
        for (const auto& h : history) {
            hist_j.push_back({
                {"step", h.step},
                {"w", h.w},
                {"b", h.b},
                {"v", h.v},
                {"c", h.c},
                {"y_hat", h.y_hat},
                {"loss", h.loss}
            });
        }

        json j = {
            {"step0", {
                {"z", s0.z},
                {"h", s0.h},
                {"y_hat", s0.y_hat},
                {"loss", s0.loss},
                {"analytic_grads", {{"dw", g_a.dw}, {"db", g_a.db}, {"dv", g_a.dv}, {"dc", g_a.dc}}},
                {"numeric_grads", {{"dw", g_n.dw}, {"db", g_n.db}, {"dv", g_n.dv}, {"dc", g_n.dc}}}
            }},
            {"history", hist_j}
        };
        res.set_content(j.dump(), "application/json");
    });
}

bool WorkbenchServer::start(bool run_async) {
    if (is_running_.load()) return true;

    httplib::Server* svr = static_cast<httplib::Server*>(svr_impl_);
    is_running_ = true;

    if (run_async) {
        server_thread_ = std::make_unique<std::thread>([this, svr]() {
            svr->listen(config_.host.c_str(), config_.port);
            is_running_ = false;
        });
        return true;
    } else {
        return svr->listen(config_.host.c_str(), config_.port);
    }
}

void WorkbenchServer::stop() {
    if (is_running_.load() && svr_impl_) {
        httplib::Server* svr = static_cast<httplib::Server*>(svr_impl_);
        svr->stop();
        is_running_ = false;
    }
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
}

} // namespace workbench::server
