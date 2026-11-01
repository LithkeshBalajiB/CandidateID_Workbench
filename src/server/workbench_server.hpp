#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <thread>

namespace workbench::server {

/**
 * @brief Server configuration and state controller.
 */
struct ServerConfig {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string web_root = "web";
};

/**
 * @brief Manages the HTTP server, asynchronous tasks, cancellation tokens, and API endpoints.
 */
class WorkbenchServer {
public:
    explicit WorkbenchServer(ServerConfig config);
    ~WorkbenchServer();

    /// Starts server (blocking or non-blocking)
    bool start(bool run_async = false);

    /// Stops server
    void stop();

    bool is_running() const { return is_running_.load(); }
    int get_port() const { return config_.port; }

private:
    void register_routes();

    ServerConfig config_;
    std::atomic<bool> is_running_{false};
    std::unique_ptr<std::thread> server_thread_;
    void* svr_impl_ = nullptr; // Opaque pointer to httplib::Server
};

} // namespace workbench::server
