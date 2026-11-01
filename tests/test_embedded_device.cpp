#include "doctest/doctest.h"
#include "core/embedded_device.hpp"

#include <memory>
#include <vector>

using namespace workbench::core;

TEST_SUITE("Task E: Embedded Mock Subsystem & Recoverable Failure") {

    TEST_CASE("Device Discovery and Capability Inspection (MCU, FPGA, JTAG)") {
        std::vector<std::unique_ptr<IDevice>> devices;
        devices.push_back(std::make_unique<MCUDevice>("MCU-TEST-01"));
        devices.push_back(std::make_unique<FPGADevice>("FPGA-TEST-01"));
        devices.push_back(std::make_unique<JTAGAdapter>("JTAG-TEST-01"));

        for (const auto& dev : devices) {
            CHECK(dev->discover());
            auto caps = dev->capabilities();
            CHECK(caps.is_simulated);
            CHECK(caps.logic_voltage_V > 0.0);
            CHECK(caps.max_clock_hz > 0);
            CHECK_FALSE(caps.supported_protocols.empty());
            CHECK_FALSE(caps.host_interface.empty());
        }
    }

    TEST_CASE("State Transitions: Connect and Disconnect") {
        MCUDevice mcu("MCU-TEST-02");

        CHECK(mcu.getState() == DeviceState::Disconnected);

        // Connect
        CHECK(mcu.connect());
        CHECK(mcu.getState() == DeviceState::Connected);

        // Disconnect
        CHECK(mcu.disconnect());
        CHECK(mcu.getState() == DeviceState::Disconnected);
    }

    TEST_CASE("Telemetry in Valid Connected State") {
        FPGADevice fpga("FPGA-TEST-02");
        fpga.connect();

        auto telemetry = fpga.readTelemetry();
        CHECK(telemetry.is_simulated);
        CHECK(telemetry.state == DeviceState::Connected);
        CHECK(telemetry.rail_voltage_V == doctest::Approx(3.3).epsilon(0.05));
        CHECK(telemetry.internal_temp_C > 20.0);
        CHECK(telemetry.current_mA > 0.0);
        CHECK(telemetry.status_message.find("[SIMULATED]") != std::string::npos);
        CHECK(telemetry.status_message.find("OK") != std::string::npos);
    }

    TEST_CASE("Invalid-State Error: Reading telemetry when disconnected") {
        JTAGAdapter jtag("JTAG-TEST-02");
        CHECK(jtag.getState() == DeviceState::Disconnected);

        auto telemetry = jtag.readTelemetry();
        CHECK(telemetry.state == DeviceState::Disconnected);
        CHECK(telemetry.status_message.find("Error") != std::string::npos);
    }

    TEST_CASE("Recoverable Failure: Timeout simulation, detection, and successful retry recovery") {
        MCUDevice mcu("MCU-FAULT-01");

        // Normal connection
        CHECK(mcu.connect());
        CHECK(mcu.getState() == DeviceState::Connected);

        // Inject simulated timeout
        mcu.setFaultInjection(true);

        // Reading telemetry triggers timeout fault
        auto fault_telemetry = mcu.readTelemetry();
        CHECK(mcu.getState() == DeviceState::FaultTimeout);
        CHECK(fault_telemetry.state == DeviceState::FaultTimeout);
        CHECK(fault_telemetry.status_message.find("timeout") != std::string::npos);
        CHECK(fault_telemetry.status_message.find("[SIMULATED]") != std::string::npos);

        // Subsequent telemetry stays in fault state until cleared
        auto still_fault = mcu.readTelemetry();
        CHECK(still_fault.state == DeviceState::FaultTimeout);

        // Recover: Clear fault and retry
        bool recovered = mcu.clearFaultAndRetry();
        CHECK(recovered);
        CHECK(mcu.getState() == DeviceState::Connected);

        // Reading telemetry after recovery is healthy again
        auto healthy_telemetry = mcu.readTelemetry();
        CHECK(healthy_telemetry.state == DeviceState::Connected);
        CHECK(healthy_telemetry.status_message.find("OK") != std::string::npos);
    }
}
