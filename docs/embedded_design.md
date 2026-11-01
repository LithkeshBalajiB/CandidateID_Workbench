# Task E: Modular Host-Connected Workbench — Embedded System Integration Design

**Candidate ID:** [FILL: Candidate ID]  
**Architecture:** Host PC connected to One Microcontroller Board, One FPGA Board, and One JTAG Debug Adapter.  
**Simulation Status:** All physical hardware interactions in the workbench are **SIMULATED** mocks; no physical soldering, PCB, or firmware flashing is required.

---

## 1. System Block Diagram

The modular workbench interfaces three distinct hardware roles to a host workstation:

```mermaid
graph TD
    Host["Host Workstation (Windows 11 x64 / Workbench App)"]

    subgraph USB_Bus["Host Communication Layer (USB 2.0 / USB 3.0)"]
        USB_MCU["Virtual COM Port (USB-CDC)"]
        USB_FPGA["FTDI FIFO / SPI Bridge (FT232H)"]
        USB_JTAG["High-Speed JTAG/SWD Debug Channel"]
    end

    subgraph Target_Hardware["Target Embedded System Under Test"]
        MCU["Microcontroller Board\n(ARM Cortex-M4 / STM32F401)\nDomain: 3.3V Logic"]
        FPGA["FPGA Board\n(Xilinx Artix-7 XC7A35T)\nDomains: 1.0V Core, 1.8V Aux, 3.3V I/O"]
        LevelShifter["Bidirectional Level Shifter\n(TXS0108E / SN74LVC8T245)\nVoltage Translation & Isolation"]
        CommBus["Inter-Chip Bus\n(High-Speed SPI / UART telemetry)"]
    end

    subgraph Debug_Layer["Hardware Debug & Boundary-Scan Interface"]
        JTAG["JTAG Debug Adapter\n(SEGGER J-Link / FTDI FT2232H)\nRole: TAP State Machine Controller"]
    end

    %% Connections
    Host <==>|USB Cable| USB_MCU
    Host <==>|USB Cable| USB_FPGA
    Host <==>|USB Cable| USB_JTAG

    USB_MCU <==>|UART TX/RX| MCU
    USB_FPGA <==>|8-bit FIFO / SPI| FPGA
    USB_JTAG <==> JTAG

    MCU <==>|3.3V Signal| LevelShifter
    LevelShifter <==>|1.8V / 3.3V Signal| FPGA

    JTAG -.->|VREF Sensed Voltage| MCU
    JTAG ==>|TCK, TMS, TDI (Driven by Adapter)| MCU
    MCU ==>|TDO (Driven by Target)| JTAG
    JTAG ==>|TCK, TMS, TDI (Chained / Muxed)| FPGA
    FPGA ==>|TDO (Return Path)| JTAG

    GND_BUS["COMMON STAR GROUND (Earth-Referenced Return Path)"]
    Host --- GND_BUS
    MCU --- GND_BUS
    FPGA --- GND_BUS
    JTAG --- GND_BUS

    style Host fill:#2b5c8f,stroke:#1a365d,stroke-width:2px,color:#fff
    style GND_BUS fill:#333,stroke:#111,stroke-width:3px,color:#fff
    style JTAG fill:#9c4221,stroke:#652b19,stroke-width:2px,color:#fff
    style LevelShifter fill:#4a5568,stroke:#2d3748,stroke-width:1px,color:#fff
```

---

## 2. Interface, Voltage Domains, and Pinout Assumptions

### 2.1 Interface & Voltage Domain Table

| Subsystem / Board | Voltage Domains | Connector Type | Host Interface | Primary Bus Role | Logic Voltage | Power Source |
| :--- | :--- | :--- | :--- | :--- | :---: | :--- |
| **Microcontroller (MCU)** | 3.3V (VDD/VDDA) | Micro USB / USB-C & 10-pin 1.27mm Cortex Debug | USB-CDC (Virtual COM) | Application Controller / Telemetry Source | 3.3V LVCMOS | 5V USB regulated via on-board LDO to 3.3V (500mA max) |
| **FPGA Board** | 1.0V (VCCINT), 1.8V (VCCAUX), 3.3V (VCCO) | USB-C & 14-pin 2.0mm JTAG header & 0.1" PMODs | FTDI FT2232H USB-FIFO | Hardware Accelerator / High-Speed DSP | 1.8V / 3.3V Banks | External 5V DC Jack regulated via switching PMIC (3A max) |
| **JTAG Debug Probe** | Senses 1.2V – 5.0V (VREF) | 20-pin 2.54mm / 10-pin 1.27mm Ribbon Cable | High-Speed USB 2.0 Debug Endpoint | TAP Bus Master (Test Access Port) | Target Sensed (VREF) | Bus-powered from Host USB (does NOT supply VREF) |
| **Inter-Chip Level Shifter** | Port A: 3.3V, Port B: 1.8V / 3.3V | Surface-mount / Breakout Board | None (Hardware pass-through) | Bidirectional Voltage Translation | Dual Supply | Powered from MCU VDD (Port A) and FPGA VCCO (Port B) |

### 2.2 Device-Specific Pinout Assumptions *(Labelled Engineering Assumptions)*
- **Assumption 1 (MCU Debug Header):** Assumes standard ARM Cortex 10-pin (1.27mm pitch) SWD/JTAG pinout:
  - Pin 1: VREF (3.3V), Pin 2: SWDIO/TMS, Pin 3: GND, Pin 4: SWCLK/TCK, Pin 5: GND, Pin 6: SWO/TDO, Pin 7: NC, Pin 8: TDI, Pin 9: GND, Pin 10: nRESET.
- **Assumption 2 (FPGA JTAG Header):** Assumes standard Xilinx 14-pin (2.0mm pitch) JTAG pinout:
  - Pin 1: GND, Pin 2: VREF (3.3V), Pin 3: GND, Pin 4: TMS, Pin 5: GND, Pin 6: TCK, Pin 7: GND, Pin 8: TDO, Pin 9: GND, Pin 10: TDI, Pin 11/12: NC, Pin 13/14: NC/GND.
- **Assumption 3 (Inter-Chip Bus):** Assumes 4-wire SPI connection (SCK, MOSI, MISO, CS#) where MCU acts as SPI Master (3.3V) and FPGA acts as SPI Slave (1.8V bank), routed through an active level shifter (TXS0108E).
- **Assumption 4 (Grounding):** Assumes a star-point common ground bus across host, MCU, FPGA, and debug adapter to eliminate ground bounce.

---

## 3. JTAG Signal Ownership & Architectural Distinction

### 3.1 Signal Ownership Matrix

| Signal Name | Full Name | Driver (Source) | Receiver (Sink) | Direction | Electrical Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **TCK** | Test Clock | **JTAG Adapter** | Target (MCU / FPGA) | Output from Probe | Free-running synchronous clock (up to 15 MHz). Dedicated clock tree on target. |
| **TMS** | Test Mode Select | **JTAG Adapter** | Target (MCU / FPGA) | Output from Probe | Steers the 16-state TAP state machine on rising edge of TCK. |
| **TDI** | Test Data In | **JTAG Adapter** | Target (MCU / FPGA) | Output from Probe | Serial data shifted into Instruction (IR) or Data (DR) registers. |
| **TDO** | Test Data Out | **Target Device** | JTAG Adapter | Input to Probe | Serial data shifted out of target on falling edge of TCK. High-impedance (tri-state) when not shifting. |
| **VREF** | Voltage Reference | **Target Board** | JTAG Adapter | Input to Probe | **Input only** to adapter level shifters. Senses target I/O voltage (e.g. 3.3V or 1.8V) to drive TCK/TMS/TDI at matching thresholds. |
| **nSRST** | System Reset | **JTAG Adapter** | Target (MCU) | Open-Drain Bidirectional | Active-low open-drain line pulled up on target. Allows debugger to reset core into halt. |
| **nTRST** | TAP Reset (Optional) | **JTAG Adapter** | Target (FPGA) | Output from Probe | Asynchronous reset of TAP controller logic (optional in IEEE 1149.1). |
| **GND** | Ground Reference | **Common** | **Common** | Bidirectional Reference | Essential 0V reference; multiple ground pins interleaved in ribbon cable to prevent crosstalk. |

### 3.2 Why a JTAG Adapter is a Debug Interface — Not a Compute Board
In distributed workbench systems, candidates and junior engineers often mistakenly treat a JTAG adapter as just another "board on the USB bus." This architectural misunderstanding causes hardware damage or communication failures:

1. **Role Asymmetry (Controller vs. Peripheral):**
   A microcontroller or FPGA runs an autonomous application program with internal ALU, memories, and independent task loops. A JTAG adapter possesses no application program; it is a **hardware-level bus master** whose sole function is to manipulate the IEEE 1149.1 Test Access Port (TAP) state machine to inspect registers, set hardware instruction breakpoints, program flash bitstreams, and sample boundary-scan pins.
2. **Signal Ownership and Tri-Stating:**
   Unlike symmetrical peer-to-peer networks (Ethernet, CAN), JTAG is strictly master-slave. The JTAG adapter unilaterally drives TCK and TMS. The target device is completely passive and only drives TDO when its TAP controller is in the `Shift-IR` or `Shift-DR` states. Connecting two JTAG adapters together or treating an adapter as a target causes drive contention and smoke.
3. **Target Voltage Sensing (VREF) vs. Power Delivery:**
   A professional JTAG adapter does **NOT** power the target board. It receives target rail voltage via VREF and uses high-speed internal level buffers to match target logic levels. Forcing power backwards from the debugger into an unpowered FPGA core will back-power the silicon through internal ESD protection diodes, destroying the device.
4. **Protocol Incompatibility:**
   Do not assume all boards share a connector or voltage. The MCU uses 3.3V SWD on a 10-pin 1.27mm header; the FPGA uses 1.8V/3.3V JTAG on a 14-pin 2.0mm header. They cannot share a single unbuffered cable without voltage level translation and chain routing.

---

## 4. Safe Integration & Bring-Up Plan

To prevent silicon damage from ground loops, reverse polarity, level mismatches, or power sequencing faults, physical workbench integration must follow this disciplined 6-stage bring-up procedure:

### Stage 1: Power & Common Ground Connection (Pre-Power Check)
- Connect earth ground and DC return lines (GND) across all boards **first** before attaching any USB or signal cables.
- Measure resistance between MCU ground, FPGA ground, and JTAG ground with a DMM: must be $< 0.5 \, \Omega$.

### Stage 2: Independent Bench Power Supply Limits
- Configure current-limiting on external bench power supplies:
  - FPGA 5V rail: Current limit set to $1.5\text{ A}$ (nominal draw $\approx 0.5\text{ A}$).
  - MCU 3.3V rail: Current limit set to $100\text{ mA}$ (nominal draw $\approx 45\text{ mA}$).
- Ensure all supply outputs are toggled OFF prior to cable insertion.

### Stage 3: Voltage Domain & Level-Shifter Verification
- Power up boards individually without interconnecting signal lines.
- Measure rail voltages with an oscilloscope / DMM:
  - MCU VDD: verify $3.3\text{ V} \pm 5\%$.
  - FPGA VCCINT: verify $1.0\text{ V} \pm 3\%$; VCCO: verify $3.3\text{ V} \pm 5\%$.
- Verify that level shifters (TXS0108E) translate 3.3V MCU signals to 1.8V/3.3V FPGA levels without overshoots $> 0.3\text{ V}$ above rail.

### Stage 4: JTAG Debugger Connection (No Hot-Plugging)
- **STRICT RULE:** Never hot-plug JTAG ribbon cables while target power is applied. Hot-plugging induces capacitive transient spikes that blow out unbuffered TDO/TDI pins.
- Attach JTAG ribbon cable while power is OFF.
- Turn on target board power $\rightarrow$ verify adapter senses VREF ($3.3\text{ V}$) $\rightarrow$ plug in JTAG USB to host PC.
- Query target IDCODE over JTAG at low clock speed ($100\text{ kHz}$) before elevating to $10\text{ MHz}$.

### Stage 5: Inter-Chip Bus Bring-Up & Telemetry Handshake
- Connect SPI / UART lines through verified level shifters.
- Transmit low-rate heartbeat telemetry frames from MCU to host.
- Verify that FPGA responds to SPI poll commands without bus timeouts.

### Stage 6: Recoverable Fault Handling
- Test recoverable timeout fault: simulate a missing telemetry response frame (timeout after $500\text{ ms}$).
- Verify workbench software flags `FAULT_TIMEOUT`, logs an actionable diagnostic, and executes `clearFaultAndRetry()` to restore normal communication without crashing the UI.
