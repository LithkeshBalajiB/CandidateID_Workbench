#include "server/workbench_server.hpp"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> g_stop_requested{false};

void signal_handler(int) {
    g_stop_requested = true;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string web_root = "web";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--web-root" && i + 1 < argc) {
            web_root = argv[++i];
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=========================================================\n";
    std::cout << "Unified Engineering Workbench - Localhost Backend Server\n";
    std::cout << "=========================================================\n";
    std::cout << "[INFO] Serving static Web UI from: " << web_root << "\n";
    std::cout << "[INFO] Listening at: http://127.0.0.1:" << port << "\n";
    std::cout << "[INFO] Press Ctrl+C to terminate cleanly.\n";

    workbench::server::ServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = port;
    cfg.web_root = web_root;

    workbench::server::WorkbenchServer server(cfg);
    server.start(false); // Blocking listen

    std::cout << "[INFO] Server shut down cleanly.\n";
    return 0;
}
