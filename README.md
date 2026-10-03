# LFR-RRC-Nodia

A high-speed, dual-core line-following robot (LFR) firmware for the ESP32 microcontroller, engineered for competition racing with deterministic 1250 Hz control execution and wireless telemetry and tuning.

The firmware combines a **low-level Raw-C real-time control engine** running on **Core 1** with an asynchronous **Wi-Fi Web Server and NVS configuration subsystem** running on **Core 0**.

---

## Current Version

**v1.0.0**

- **Architecture**: Dual-Core ESP32 (Core 1 is dedicated to the LFR control path while Core 0 handles Wi-Fi, WebServer, and NVS configuration).
- **Framework**: PlatformIO on Arduino-ESP32 core 3.x (`framework-arduinoespressif32 @ 3.0.7`).
- **Core 1 Real-Time Engine**: Pure C modules (`lfr_bsp.c`, `lfr_sensor.c`, `lfr_motor.c`, `lfr_pid.c`, `lfr_core.c`) with zero heap allocation and zero dynamic memory.
- **Control Features**: Exact discrete PID mathematics without premature truncation, persistent NVS flash storage for PID and calibration, live Web UI sliders, 2-phase median-filtered calibration, deterministic 800 µs loop pacing.

---

> [!IMPORTANT]
> **16-Channel Sensor Array Required**: This firmware is designed and strictly tuned for a **16-channel analog sensor array** multiplexed via a CD74HC4067 (channels 0 through 15). It is **not** directly interchangeable with 14-channel sensor boards without adjusting sensor geometric weights in [`include/config.h`](include/config.h). Verify your sensor hardware before flashing.

---

## Setup & Documentation

> **Want to flash quickly?** Use the [Quick Start — One-Click Upload](#quick-start--one-click-upload).  
> **Building or configuring the robot for the first time?** Follow the comprehensive [SETUP.md](SETUP.md).

👉 **[Read the complete Hardware, Wiring, and Setup Guide in SETUP.md](SETUP.md)**

| Method | Intended User | Steps | Best For |
| :--- | :--- | :---: | :--- |
| **One-Click Upload** | Assembled robot, PlatformIO already installed | Minimal | Fast flashing & iterative firmware updates |
| **[SETUP.md](SETUP.md)** | First-time user, new hardware builds | Complete | Full wiring diagram, electrical safety, calibration, and troubleshooting |

- **Use One-Click Upload when**: The robot wiring is verified, PlatformIO is installed, and you simply want to compile and flash the microcontroller.
- **Use [SETUP.md](SETUP.md) when**: You are assembling the chassis, wiring the TB6612FNG or CD74HC4067 multiplexer, setting up VS Code from scratch, or diagnosing sensor/motor issues.

---

## Quick Start — One-Click Upload

### 1. Clone the Repository
```bash
git clone https://github.com/Ojas-sta/LFR-RRC-Nodia.git
cd LFR-RRC-Nodia
```

### 2. Install PlatformIO (if not already installed)
```bash
pip install -U platformio
```

### 3. Connect ESP32 and Flash

#### Windows
In the repository root, double-click **`upload.bat`** (or run `upload.bat` in Command Prompt). The script compiles the project, auto-detects the serial COM port, and flashes the firmware.

#### Linux / macOS
Open a terminal in the repository root and run:
```bash
./upload.sh
```

To specify a serial port explicitly (optional):
```bash
./upload.sh /dev/cu.usbserial-0001
```

#### Direct PlatformIO Fallback
```bash
pio run -t upload
```

---

## Hardware & Pinout

The robot hardware incorporates an ESP32 microcontroller, a 16-channel analog infrared reflectance array multiplexed via a CD74HC4067 IC, a TB6612FNG dual H-bridge motor driver, and dual DC N20 micro metal gearmotors.

### Pinout Configuration (ESP32 v8 Pinout)

| Peripheral Module | Signal Name | ESP32 GPIO | Electrical Function |
| :--- | :--- | :--- | :--- |
| **CD74HC4067 Multiplexer** | `MUX_S0` | **GPIO 32** | Channel Select Bit 0 (LSB) |
| | `MUX_S1` | **GPIO 33** | Channel Select Bit 1 |
| | `MUX_S2` | **GPIO 25** | Channel Select Bit 2 |
| | `MUX_S3` | **GPIO 26** | Channel Select Bit 3 (MSB) |
| | `MUX_SIG` | **GPIO 34** | Analog Out $\rightarrow$ **ADC1 Channel 6** (Input Only) |
| | `MUX_EN` | **GPIO 27** | Multiplexer Enable (Active LOW) |
| **TB6612FNG Motor Driver** | `AIN1` | **GPIO 15** | Motor 1 (Left) Direction Phase A |
| | `AIN2` | **GPIO 14** | Motor 1 (Left) Direction Phase B |
| | `PWMA` | **GPIO 13** | Motor 1 (Left) Speed PWM (20 kHz LEDC) |
| | `BIN1` | **GPIO 19** | Motor 2 (Right) Direction Phase A |
| | `BIN2` | **GPIO 21** | Motor 2 (Right) Direction Phase B |
| | `PWMB` | **GPIO 18** | Motor 2 (Right) Speed PWM (20 kHz LEDC) |
| | `STBY` | **GPIO 23** | Driver Standby Enable (Active HIGH) |
| **User Interface** | `CAL_BTN` | **GPIO 4** | Physical Calibrate Button (Active LOW, internal pull-up) |
| | `START_BTN` | **GPIO 5** | Physical Start/Stop Toggle (Active LOW, internal pull-up) |
| | `LED_PIN` | **GPIO 2** | On-board Status Indicator LED (Active HIGH) |

---

## Control Architecture

The system enforces strict execution boundaries across the ESP32's dual CPU cores:

```text
                    ESP32
                      |
          +-----------+-----------+
          |                       |
       CORE 0                   CORE 1
      C++ / Infra              RAW C / RT
          |                       |
     Wi-Fi / Web             Sensor / ADC
     Web Dashboard            Calibration
     NVS                       Line Error
     Configuration            PID
     Telemetry                Speed Control
          |                   Lost Line
          |                   Motors
          +---------+---------+
                    |
             C-compatible sync
```

### Core 0: High-Level Infrastructure (C++)
- **Wi-Fi SoftAP**: Broadcasts `LineFollower_AP` ($8.5\text{ dBm}$ TX power to eliminate ADC ripple).
- **Asynchronous WebServer**: Serves the mobile-optimized dark-mode dashboard at `http://192.168.4.1/`.
- **Live Tuning**: Parses slider updates for $K_p, K_i, K_d$, and speed parameters into double-buffered RAM.
- **NVS Flash Persistence**: Safely stores configuration and calibration tables to flash memory via explicit user save commands.
- **Telemetry Serializer**: Prepares 20 Hz JSON telemetry snapshots for live client monitoring.
- **Safety Guarantee**: **Core 0 never directly controls the motors.**

### Core 1: Raw-C Real-Time Control Loop
- **Hardware Abstraction**: Low-level GPIO, ADC sampling, and 20 kHz ultrasonic motor PWM.
- **Sensor Pipeline**: Mux channel selection with $10\text{ µs}$ settling delay, median-filtered acquisition, and 0..1000 normalization.
- **Trajectory Decisions**: Discrete centroid line position calculation, outer-sensor sharp turn downshifting, and lost-line recovery spins.
- **Deterministic Loop**: Paced by an $800\text{ µs}$ microsecond busy-wait loop ($\approx 1250\text{ Hz}$).
- **Task Priority**: Pinned at FreeRTOS real-time priority 24 with zero heap allocation, zero `String` usage, and zero flash writes.

---

## Raw-C Subsystem Architecture

The real-time control engine on Core 1 is constructed as modular, low-level C components:

- [`src/lfr_bsp.c`](src/lfr_bsp.c) / [`include/lfr_bsp.h`](include/lfr_bsp.h): Low-level board support package for GPIO configuration, 12-bit ADC reads, 20 kHz LEDC PWM motor generation, and microsecond hardware timers.
- [`src/lfr_sensor.c`](src/lfr_sensor.c) / [`include/lfr_sensor.h`](include/lfr_sensor.h): 16-channel analog multiplexer acquisition, 0..1000 normalization, and 2-phase median-filtered auto-polarity calibration.
- [`src/lfr_motor.c`](src/lfr_motor.c) / [`include/lfr_motor.h`](include/lfr_motor.h): Dual H-bridge motor driver primitives and directional mixing with active motor braking on zero.
- [`src/lfr_pid.c`](src/lfr_pid.c) / [`include/lfr_pid.h`](include/lfr_pid.h): Discrete centroid error calculation, golden-reference PID mathematics without premature truncation, sharp-turn detection, speed ramping, and lost-line recovery.
- [`src/lfr_core.c`](src/lfr_core.c) / [`include/lfr_core.h`](include/lfr_core.h): Robot state machine orchestration, deterministic 800 µs loop pacing, physical button debouncing, and Core 1 FreeRTOS task entry.

---

## First Boot & Operation Sequence

The physical robot follows this deterministic operating sequence:

```text
Power ON
   ↓
Motors remain stopped (Fail-Safe)
LED blinks at 2 Hz (Uncalibrated IDLE)
   ↓
Connect smartphone/laptop to "LineFollower_AP" (password: "linefollow123")
   ↓
Open browser to http://192.168.4.1/
   ↓
Place sensor array center over track line
   ↓
Click "Calibrate" (or press physical button on GPIO 4)
Robot spins left and right for 5 seconds
   ↓
Calibration passes (>= 8 valid sensors):
State transitions to READY (LED solid on / Blue badge)
   ↓
Click "Start" (or press physical button on GPIO 5)
   ↓
Robot enters RUNNING at 1250 Hz control rate
```

---

## Wi-Fi Setup & Web Dashboard

- **SSID**: `LineFollower_AP`
- **Password**: `linefollow123`
- **IP Address**: `http://192.168.4.1/`

The browser dashboard provides:
1. **Live State Badge**: Displays `IDLE`, `CALIBRATING`, `READY`, or `RUNNING`.
2. **16-Channel Reflectance Visualizer**: Real-time bar graph displaying reflectance levels ($0$ to $1000$) across all 16 channels at 20 Hz. Green bars highlight active line detection.
3. **Action Buttons**: **Calibrate** and **Start / Stop**.
4. **Interactive PID & Speed Sliders**: Live adjustments for $K_p$, $K_i$, $K_d$, Top Speed, Turn Speed, and Start Speed.
5. **Flash Storage Controls**:
   - **Save Settings**: Persists current tuning and calibration tables to NVS flash.
   - **Load Defaults**: Restores factory tuning defaults in live RAM.
   - **Reset Calibration**: Clears stored calibration from flash and RAM.

---

## Sensor Calibration

Calibration maps the physical reflectivity range of your track surface and determines track polarity:
1. **Trigger**: Press the physical button on GPIO 4 or click **Calibrate** in the Web UI.
2. **Motion**: Robot pauses for 1 second, then spins counter-clockwise at PWM 90 for 2.5 seconds, pauses 150 ms, and spins clockwise for 2.5 seconds.
3. **Processing**: Raw readings pass through a 3-sample median filter (`med3`) to reject electrical motor noise.
4. **Auto-Polarity Detection**: Compares sensor baseline means to the contrast span. Determines automatically whether the track is dark line on light surface or light line on dark surface.
5. **Validation Threshold**: Requires at least 8 sensors to record a contrast span $\ge 150$ ADC units (`MIN_RANGE = 150`). If passed, robot transitions to `READY`. If fewer than 8 sensors qualify, robot reverts to `IDLE` with a diagnostic message.

---

## Tuning & Control Parameters

### Default Parameters (Factory Defaults)
| Parameter | Default Value | Description |
| :--- | :---: | :--- |
| **`Kp`** | `0.10` | Proportional gain for primary steering response |
| **`Ki`** | `0.00` | Integral gain (keep 0.00 to prevent windup oscillation) |
| **`Kd`** | `1.00` | Derivative gain for oscillation damping and stability |
| **`Speed`** | `230` | Top line-following speed on straightaways (0..255) |
| **`Turn`** | `80` | Reduced speed applied during sharp turns (0..255) |
| **`Start`** | `180` | Initial launch speed when leaving the start line (0..255) |

> **RAM vs. NVS Persistence**: Slider adjustments update **RAM immediately**. To retain parameters across power cycles, click **Save Settings** in the Web UI.

---

## Sensor System & Weighting

The 16 analog channels are evaluated to determine line position:
- **Threshold**: Normalized values $> 500$ are flagged as on-line (`SEEN_THRESHOLD = 500`).
- **Geometric Weights Array**:
  ```text
  Sensor Index:   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
  Weight Value:   7   6   5   4   3   2   1   0   0  -1  -2  -3  -4  -5  -6  -7
  ```
- **Error Calculation**: Discrete centroid average over active sensors:
  $$\text{error} = \frac{\sum_{i=0}^{15} \text{Weight}[i] \times \text{SensorArray}[i] \times \text{SensorValue}[i]}{\text{ActiveSensors}}$$

---

## Control Loop Execution Pipeline

Each control iteration executes deterministically within an **$800\text{ µs}$ budget ($\approx 1250\text{ Hz}$)**:

$$\text{Paced Wait (800 µs)} \longrightarrow \text{Mux Read (16 ch)} \longrightarrow \text{Centroid Error} \longrightarrow \text{Sharp Turn Check} \longrightarrow \text{Speed Ramp} \longrightarrow \text{PID Math} \longrightarrow \text{Motor PWM}$$

1. **Sharp Turn Detection**: If outer sensors (channels 0, 1, 14, or 15) detect the line, `currentSpeed` drops immediately to `turnSpeed` (80) and sets `lastTurnDir`.
2. **Speed Ramping**: When line returns to inner sensors, speed ramps up by $+1$ PWM unit per 800 µs loop tick until reaching `lfSpeed` (230).
3. **Lost-Line Recovery**: When no sensors detect the line (`onLine == 0`), the robot aggressively spins in place towards `lastTurnDir`:
   - Line lost to left (`lastTurnDir == 1`): Left motor reverse at $-100$, right motor forward at $255$.
   - Line lost to right (`lastTurnDir == -1`): Left motor forward at $255$, right motor reverse at $-100$.

---

## Safety Systems

- **Power-On Motor Lockout**: Motors are explicitly driven to 0 PWM with H-bridges short-braked during boot. Motors never spin until an explicit Start command is issued.
- **Actuator Ownership Isolation**: Core 0 (Wi-Fi/Web) has zero motor control primitives. Core 1 is the sole motor owner.
- **Calibration Area Safety**: The robot spins in place during calibration. Ensure track area is clear before clicking Calibrate.
- **Bench Lift Testing**: Always conduct initial motor and direction checks with the robot wheels elevated off the surface.
- **Power Disconnect Warning**: Disconnect battery power before altering motor driver or multiplexer wiring.

---

## Troubleshooting

| Problem | Cause | Recommended Action |
| :--- | :--- | :--- |
| **Upload Fails / Connection Timeout** | ESP32 not in bootloader mode or wrong cable | Hold **BOOT** button while terminal says `Connecting...`. Ensure USB cable supports data transfer. |
| **Robot Does Not Move on Start** | Robot uncalibrated, low battery, or missing STBY | Confirm state is `READY` (not `IDLE`). Check motor battery voltage on TB6612 `VM` terminal ($> 6.5\text{V}$). Check GPIO 23 to `STBY`. |
| **Robot Drives Backwards** | Motor leads reversed | Swap the two motor output wires for that channel on the TB6612 terminal block. |
| **Calibration Fails (`only X/16 sensors`)** | Sensor height incorrect or low surface contrast | Adjust sensor ground clearance to **3 mm to 6 mm**. Verify MUX S0..S3 (GPIO 32, 33, 25, 26) and SIG (GPIO 34) wiring. |
| **Wi-Fi Dashboard Inaccessible** | Phone bypassing local Wi-Fi or weak power | Disable cellular data on phone. Ensure ESP32 is powered from a capable 5V regulator (Wi-Fi peaks at 250 mA). |

---

## Repository Structure

```text
LFR-RRC-Nodia/
├── include/
│   ├── config.h            # Pin definitions, 16-ch weights, and tuned defaults
│   ├── lfr_bsp.h           # Board support package C header (GPIO, ADC, PWM, timer)
│   ├── lfr_core.h          # Core 1 state machine and execution loop C header
│   ├── lfr_motor.h         # Motor control primitives C header
│   ├── lfr_pid.h           # Discrete PID and trajectory dynamics C header
│   ├── lfr_sensor.h        # 16-channel sensor acquisition & calibration C header
│   ├── nvs_manager.h       # NVS flash memory persistence C++ header (Core 0)
│   ├── sync.h              # Inter-core synchronization C/C++ header
│   ├── types.h             # Data structures and robot state definitions
│   └── web_server.h        # Wi-Fi SoftAP and web server C++ header (Core 0)
├── scripts/
│   ├── upload.bat          # Windows one-click build and upload script
│   └── upload.sh           # Linux/macOS one-click build and upload script
├── src/
│   ├── lfr_bsp.c           # Raw-C low-level hardware drivers (Core 1)
│   ├── lfr_core.c          # Raw-C 1250 Hz control loop and state machine (Core 1)
│   ├── lfr_motor.c         # Raw-C TB6612FNG H-bridge motor driver (Core 1)
│   ├── lfr_pid.c           # Raw-C exact discrete PID and line math (Core 1)
│   ├── lfr_sensor.c        # Raw-C 16-channel mux scanner & calibration (Core 1)
│   ├── main.cpp            # System bootstrapper and FreeRTOS task spawner
│   ├── nvs_manager.cpp     # Preferences flash storage implementation (Core 0)
│   ├── sync.cpp            # Lock-free atomic and spinlock synchronization
│   └── web_server.cpp      # Wi-Fi SoftAP and web dashboard implementation (Core 0)
├── platformio.ini          # PlatformIO build configuration (Arduino-ESP32 3.0.7)
├── upload.bat              # Root one-click upload helper (Windows)
├── upload.sh               # Root one-click upload helper (Linux/macOS)
├── README.md               # Main repository documentation and quick start
└── SETUP.md                # Complete hardware, wiring, and software setup guide
```

---

## Setup Guide

For the complete hardware, software, wiring, calibration, and troubleshooting guide:

👉 **[Read the full SETUP.md](SETUP.md)**
