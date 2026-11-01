#include "core/embedded_device.hpp"

#include <stdexcept>

namespace workbench::core {

std::string device_state_to_string(DeviceState state) {
    switch (state) {
        case DeviceState::Disconnected: return "DISCONNECTED";
        case DeviceState::Connected: return "CONNECTED";
        case DeviceState::FaultTimeout: return "FAULT_TIMEOUT";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// MCU Device Implementation
// ============================================================================

MCUDevice::MCUDevice(std::string id) : id_(std::move(id)) {}

bool MCUDevice::discover() {
    return true; // Virtual simulated probe always discovers mocked hardware
}

DeviceCapabilities MCUDevice::capabilities() const {
    DeviceCapabilities caps;
    caps.device_type = getDeviceType();
    caps.host_interface = "USB-CDC / Virtual COM Port";
    caps.logic_voltage_V = 3.3;
    caps.max_clock_hz = 84000000; // 84 MHz Cortex-M4
    caps.supported_protocols = {"UART", "I2C", "SPI", "ADC_Telemetry"};
    caps.is_simulated = true;
    return caps;
}

bool MCUDevice::connect() {
    if (inject_fault_) {
        state_ = DeviceState::FaultTimeout;
        return false;
    }
    state_ = DeviceState::Connected;
    simulated_uptime_ms_ = 100;
    return true;
}

bool MCUDevice::disconnect() {
    state_ = DeviceState::Disconnected;
    return true;
}

DeviceTelemetry MCUDevice::readTelemetry() {
    DeviceTelemetry t;
    t.device_id = id_;
    t.device_type = getDeviceType();
    t.state = state_;
    t.is_simulated = true;

    if (state_ == DeviceState::Disconnected) {
        t.status_message = "[SIMULATED] Error: Cannot read telemetry from disconnected MCU device.";
        return t;
    }

    if (inject_fault_ || state_ == DeviceState::FaultTimeout) {
        state_ = DeviceState::FaultTimeout;
        t.state = DeviceState::FaultTimeout;
        t.status_message = "[SIMULATED] Warning: UART communication timeout after 500ms. Response frame missing.";
        return t;
    }

    simulated_uptime_ms_ += 250;
    t.uptime_ms = simulated_uptime_ms_;
    t.internal_temp_C = 37.8;
    t.rail_voltage_V = 3.295;
    t.current_mA = 42.6;
    t.status_message = "[SIMULATED] OK: MCU core running normally at 84 MHz. ADC telemetry healthy.";
    return t;
}

bool MCUDevice::clearFaultAndRetry() {
    inject_fault_ = false; // Reset injected fault
    return connect();      // Re-establish connection
}

// ============================================================================
// FPGA Device Implementation
// ============================================================================

FPGADevice::FPGADevice(std::string id) : id_(std::move(id)) {}

bool FPGADevice::discover() {
    return true;
}

DeviceCapabilities FPGADevice::capabilities() const {
    DeviceCapabilities caps;
    caps.device_type = getDeviceType();
    caps.host_interface = "High-Speed FTDI USB FIFO (FT232H / SPI)";
    caps.logic_voltage_V = 3.3; // I/O bank voltage (core 1.0V internal)
    caps.max_clock_hz = 100000000; // 100 MHz oscillator
    caps.supported_protocols = {"SPI_Slave", "AXI_Stream", "Raw_FIFO"};
    caps.is_simulated = true;
    return caps;
}

bool FPGADevice::connect() {
    if (inject_fault_) {
        state_ = DeviceState::FaultTimeout;
        return false;
    }
    state_ = DeviceState::Connected;
    simulated_uptime_ms_ = 150;
    return true;
}

bool FPGADevice::disconnect() {
    state_ = DeviceState::Disconnected;
    return true;
}

DeviceTelemetry FPGADevice::readTelemetry() {
    DeviceTelemetry t;
    t.device_id = id_;
    t.device_type = getDeviceType();
    t.state = state_;
    t.is_simulated = true;

    if (state_ == DeviceState::Disconnected) {
        t.status_message = "[SIMULATED] Error: Cannot read telemetry from disconnected FPGA device.";
        return t;
    }

    if (inject_fault_ || state_ == DeviceState::FaultTimeout) {
        state_ = DeviceState::FaultTimeout;
        t.state = DeviceState::FaultTimeout;
        t.status_message = "[SIMULATED] Warning: SPI FIFO response timeout. FPGA core unresponsive.";
        return t;
    }

    simulated_uptime_ms_ += 250;
    t.uptime_ms = simulated_uptime_ms_;
    t.internal_temp_C = 46.4;
    t.rail_voltage_V = 3.302;
    t.current_mA = 185.3;
    t.status_message = "[SIMULATED] OK: Bitstream verified. VCCINT=1.0V, VCCO=3.3V, logic cells active.";
    return t;
}

bool FPGADevice::clearFaultAndRetry() {
    inject_fault_ = false;
    return connect();
}

// ============================================================================
// JTAG Debug Adapter Implementation
// ============================================================================

JTAGAdapter::JTAGAdapter(std::string id) : id_(std::move(id)) {}

bool JTAGAdapter::discover() {
    return true;
}

DeviceCapabilities JTAGAdapter::capabilities() const {
    DeviceCapabilities caps;
    caps.device_type = getDeviceType();
    caps.host_interface = "USB 2.0 High-Speed Debug Channel";
    caps.logic_voltage_V = 3.3; // Sensed target reference VREF
    caps.max_clock_hz = 15000000; // 15 MHz TCK clock
    caps.supported_protocols = {"IEEE_1149.1_JTAG", "ARM_SWD", "Boundary_Scan"};
    caps.is_simulated = true;
    return caps;
}

bool JTAGAdapter::connect() {
    if (inject_fault_) {
        state_ = DeviceState::FaultTimeout;
        return false;
    }
    state_ = DeviceState::Connected;
    simulated_uptime_ms_ = 200;
    return true;
}

bool JTAGAdapter::disconnect() {
    state_ = DeviceState::Disconnected;
    return true;
}

DeviceTelemetry JTAGAdapter::readTelemetry() {
    DeviceTelemetry t;
    t.device_id = id_;
    t.device_type = getDeviceType();
    t.state = state_;
    t.is_simulated = true;

    if (state_ == DeviceState::Disconnected) {
        t.status_message = "[SIMULATED] Error: Cannot read telemetry from disconnected JTAG probe.";
        return t;
    }

    if (inject_fault_ || state_ == DeviceState::FaultTimeout) {
        state_ = DeviceState::FaultTimeout;
        t.state = DeviceState::FaultTimeout;
        t.status_message = "[SIMULATED] Warning: JTAG TAP scan timeout. Target device did not assert TDO acknowledge.";
        return t;
    }

    simulated_uptime_ms_ += 250;
    t.uptime_ms = simulated_uptime_ms_;
    t.internal_temp_C = 34.2;
    t.rail_voltage_V = 3.298; // Target sensed VREF
    t.current_mA = 24.1;     // Probe internal USB draw
    t.status_message = "[SIMULATED] OK: Target VREF=3.3V detected. TAP state: Run-Test/Idle. IDCODE scan verified.";
    return t;
}

bool JTAGAdapter::clearFaultAndRetry() {
    inject_fault_ = false;
    return connect();
}

} // namespace workbench::core
