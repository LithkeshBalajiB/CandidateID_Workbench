#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "server/workbench_server.hpp"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    auto t_start = std::chrono::high_resolution_clock::now();

    // Determine base directory relative to executable
    char exe_path_buf[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path_buf, MAX_PATH);
    std::filesystem::path exe_dir = std::filesystem::path(exe_path_buf).parent_path();
    
    // Set current directory to executable directory or parent if in bin/
    if (exe_dir.filename() == "bin") {
        std::filesystem::current_path(exe_dir.parent_path());
    } else {
        std::filesystem::current_path(exe_dir);
    }

    workbench::server::ServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 8080;
    cfg.web_root = "web";

    workbench::server::WorkbenchServer server(cfg);
    bool started = server.start(true); // Run server asynchronously on worker thread

    auto t_ready = std::chrono::high_resolution_clock::now();
    double launch_to_ready_ms = std::chrono::duration<double, std::milli>(t_ready - t_start).count();

    // Record launch-to-ready timing to results/launch_timing.txt
    try {
        std::filesystem::create_directories("results");
        std::ofstream ofs("results/launch_timing.txt");
        ofs << "Launch-to-Ready Time: " << launch_to_ready_ms << " ms\n";
        ofs << "Target Port: http://127.0.0.1:" << cfg.port << "\n";
        ofs << "Server Status: " << (started ? "SUCCESS" : "FAILED") << "\n";
    } catch (...) {}

    // Open default browser automatically without any terminal window
    std::string url = "http://127.0.0.1:" + std::to_string(cfg.port);
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    // Keep server process alive until stopped
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
