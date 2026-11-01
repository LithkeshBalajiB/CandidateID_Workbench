#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace workbench::core {

/**
 * @brief Device connection state machine.
 */
enum class DeviceState {
    Disconnected,
    Connected,
    FaultTimeout
};

std::string device_state_to_string(DeviceState state);

/**
 * @brief Static hardware capabilities reported by a device.
 */
struct DeviceCapabilities {
    std::string device_type;
    std::string host_interface;
    double logic_voltage_V = 3.3;
    uint32_t max_clock_hz = 0;
    std::vector<std::string> supported_protocols;
    bool is_simulated = true;
};

/**
 * @brief Dynamic telemetry message retrieved from device.
 */
struct DeviceTelemetry {
    std::string device_id;
    std::string device_type;
    DeviceState state = DeviceState::Disconnected;
    double internal_temp_C = 0.0;
    double rail_voltage_V = 0.0;
    double current_mA = 0.0;
    uint64_t uptime_ms = 0;
    std::string status_message;
    bool is_simulated = true;
};

/**
 * @brief Abstract interface for host-connected workbench devices.
 */
class IDevice {
public:
    virtual ~IDevice() = default;

    virtual std::string getDeviceId() const = 0;
    virtual std::string getDeviceType() const = 0;
    virtual DeviceState getState() const = 0;

    virtual bool discover() = 0;
    virtual DeviceCapabilities capabilities() const = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual DeviceTelemetry readTelemetry() = 0;

    /// Recovers from an active fault state and attempts reconnect
    virtual bool clearFaultAndRetry() = 0;

    /// Test injection hook for simulating hardware timeout
    virtual void setFaultInjection(bool enable_timeout) = 0;
};

/**
 * @brief Simulated Microcontroller Device (ARM Cortex-M4 / STM32F401, 3.3V).
 */
class MCUDevice : public IDevice {
public:
    explicit MCUDevice(std::string id = "MCU-STM32-01");

    std::string getDeviceId() const override { return id_; }
    std::string getDeviceType() const override { return "Microcontroller (STM32F401)"; }
    DeviceState getState() const override { return state_; }

    bool discover() override;
    DeviceCapabilities capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    DeviceTelemetry readTelemetry() override;
    bool clearFaultAndRetry() override;
    void setFaultInjection(bool enable_timeout) override { inject_fault_ = enable_timeout; }

private:
    std::string id_;
    DeviceState state_ = DeviceState::Disconnected;
    bool inject_fault_ = false;
    uint64_t simulated_uptime_ms_ = 0;
};

/**
 * @brief Simulated FPGA Device (Xilinx Artix-7, 1.0V core / 3.3V I/O).
 */
class FPGADevice : public IDevice {
public:
    explicit FPGADevice(std::string id = "FPGA-ARTIX7-01");

    std::string getDeviceId() const override { return id_; }
    std::string getDeviceType() const override { return "FPGA (Xilinx Artix-7)"; }
    DeviceState getState() const override { return state_; }

    bool discover() override;
    DeviceCapabilities capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    DeviceTelemetry readTelemetry() override;
    bool clearFaultAndRetry() override;
    void setFaultInjection(bool enable_timeout) override { inject_fault_ = enable_timeout; }

private:
    std::string id_;
    DeviceState state_ = DeviceState::Disconnected;
    bool inject_fault_ = false;
    uint64_t simulated_uptime_ms_ = 0;
};

/**
 * @brief Simulated JTAG Debug Adapter (SEGGER J-Link / FTDI FT2232H TAP Controller).
 */
class JTAGAdapter : public IDevice {
public:
    explicit JTAGAdapter(std::string id = "JTAG-PROBE-01");

    std::string getDeviceId() const override { return id_; }
    std::string getDeviceType() const override { return "JTAG Debug Probe (FT2232H / J-Link)"; }
    DeviceState getState() const override { return state_; }

    bool discover() override;
    DeviceCapabilities capabilities() const override;
    bool connect() override;
    bool disconnect() override;
    DeviceTelemetry readTelemetry() override;
    bool clearFaultAndRetry() override;
    void setFaultInjection(bool enable_timeout) override { inject_fault_ = enable_timeout; }

private:
    std::string id_;
    DeviceState state_ = DeviceState::Disconnected;
    bool inject_fault_ = false;
    uint64_t simulated_uptime_ms_ = 0;
};

} // namespace workbench::core
