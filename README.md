# LFR-RRC-Nodia: High-Speed Dual-Core ESP32 Line Follower

A production-grade, competition-tuned line-following robot (LFR) firmware for the ESP32 microcontroller, engineered for deterministic 1250 Hz control loop execution and real-time parameter tuning over Wi-Fi.

---

## 1. Hardware Architecture

- **Microcontroller**: ESP32 Dual-Core Tensilica Xtensa LX6 (240 MHz)
- **Sensor Array**: **RoboJunkies 14-channel analog infrared reflectance array** multiplexed via CD74HC4067 multiplexer
- **Motor Driver**: TB6612FNG Dual H-Bridge (20 kHz ultrasonic PWM)
- **Actuators**: 2× Micro Metal Gear DC N20 Motors
- **Indicators & Controls**:
  - GPIO 2: Status LED
  - GPIO 4: Physical Calibrate Button
  - GPIO 5: Physical Start/Stop Button

---

## 2. Pinout Configuration

| Peripheral | Signal | ESP32 GPIO | Description |
| :--- | :--- | :--- | :--- |
| **Multiplexer** | `MUX_S0` | **32** | Channel Select Bit 0 (LSB) |
| | `MUX_S1` | **33** | Channel Select Bit 1 |
| | `MUX_S2` | **25** | Channel Select Bit 2 |
| | `MUX_S3` | **26** | Channel Select Bit 3 (MSB) |
| | `MUX_SIG` | **34** | ADC1 Channel 6 Analog Input |
| | `MUX_EN` | **27** | Multiplexer Enable (Active LOW) |
| **TB6612FNG** | `AIN1` | **15** | Motor 1 (Left) Direction A |
| | `AIN2` | **14** | Motor 1 (Left) Direction B |
| | `PWMA` | **13** | Motor 1 (Left) Speed PWM |
| | `BIN1` | **19** | Motor 2 (Right) Direction A |
| | `BIN2` | **21** | Motor 2 (Right) Direction B |
| | `PWMB` | **18** | Motor 2 (Right) Speed PWM |
| | `STBY` | **23** | Driver Standby (Active HIGH) |
| **User Interface** | `CAL_BTN` | **4** | Calibrate Button (INPUT_PULLUP) |
| | `START_BTN` | **5** | Start/Stop Button (INPUT_PULLUP) |
| | `LED_PIN` | **2** | Status Indicator LED |

---

## 3. Dual-Core Architecture

The firmware enforces strict hardware and task isolation between FreeRTOS cores to guarantee zero jitter in the line-following algorithm:

```
                      +---------------------------------------+
                      |         ESP32 Dual-Core SoC           |
                      +-------------------+-------------------+
                                          |
                   CORE 0                 |                CORE 1
          (Wi-Fi / Web / Storage)         |         (Real-Time LFR Loop)
  +-------------------------------------+ | +-------------------------------------+
  | - SoftAP: "LineFollower_AP"         | | | - 800 µs (~1250 Hz) Deterministic   |
  | - WebServer (Port 80)               | | |   Control Loop                      |
  | - HTTP Handlers (/, /status, /pid,  | | | - 14-ch MUX Acquisition (ADC1)     |
  |   /start, /cal, /save, /defaults)   | | | - Error & PID Calculation           |
  | - Live Tuning RAM updates           | | | - Speed Ramping & Sharp Turns       |
  | - Persistent NVS (Preferences)      | | | - Lost-Line Recovery                |
  |   Save / Load / Factory Reset       | | | - TB6612FNG Motor Control (LEDC)    |
  | - Calibration & Start Dispatch      | | | - Physical Buttons & Status LED     |
  | - Telemetry JSON Serialization      | | | - Calibration Spin Routine          |
  +-------------------------------------+ | +-------------------------------------+
                     ^                    |                    ^
                     |     Lock-Free Inter-Core Mailbox        |
                     +--------------------+--------------------+
                                          |
                  +-----------------------+-----------------------+
                  |  Shared State / Synchronization Layer         |
                  |  - Atomic Command Flags (Start/Stop/Cal)      |
                  |  - Double-Buffered Config (Lock-Free Seqlock) |
                  |  - Low-Rate Telemetry Snapshot (20 Hz)        |
                  |  - Critical Section for Status Message        |
                  +-----------------------------------------------+
```

### Determinism & Performance
- **Core 1 Isolation**: Core 1 never executes `String` allocations, never performs flash operations, and never blocks on Wi-Fi or WebServer locks.
- **Zero-Mutex Hot Path**: Configuration updates from Core 0 use an atomic dirty flag and staged buffers, allowing Core 1 to check for updates with a single memory load.
- **Microsecond Precision**: Timing is governed by `micros()` pacing at `LOOP_PERIOD_US = 800` (~1250 Hz).

---

## 4. Installation & Build

### Prerequisites
Install [PlatformIO Core](https://platformio.org/install/cli):
```bash
pip install -U platformio
```

### Clone and Compile
```bash
git clone https://github.com/Ojas-sta/LFR-RRC-Nodia.git
cd LFR-RRC-Nodia

# Build firmware
pio run
```

---

## 5. Uploading to ESP32

### One-Command Upload Script
Use the provided script:
```bash
# Auto-detects serial port
./scripts/upload.sh

# Or specify your port explicitly:
./scripts/upload.sh /dev/cu.usbserial-0001
```

### Raw PlatformIO Commands
```bash
# Compile and upload
pio run -t upload

# Upload to specific port
pio run -t upload --upload-port /dev/cu.usbserial-0001

# Serial monitor
pio device monitor -b 115200
```

---

## 6. Wi-Fi AP & Web Dashboard

When the robot boots, it creates a local Wi-Fi Access Point:

- **SSID**: `LineFollower_AP`
- **Password**: `linefollow123`
- **IP Address**: `http://192.168.4.1/`

Connect any smartphone or laptop to `LineFollower_AP` and open `http://192.168.4.1/` in your browser.

The dashboard displays:
- **Robot State Badge**: `IDLE`, `CALIBRATING`, `READY`, or `RUNNING`
- **Live Sensor Bar Array**: 14 real-time bars showing reflectance values (0..1000) and detection state
- **Status Messages**: Feedback from calibration, boot, and motor commands
- **Live Sliders**: `Kp`, `Ki`, `Kd`, `Speed`, `Turn`, `Start`
- **Primary Buttons**: **Calibrate**, **Start / Stop**
- **Persistence Buttons**: **Save Settings**, **Load Defaults**, **Reset Calibration**

---

## 7. Calibration Procedure

Before running, the robot must be calibrated:

1. Place the sensor array directly over the line on your track surface.
2. In the Web UI, click **Calibrate** (or press the physical button on GPIO 4).
3. The robot will pause for 1 second, then spin left and right over the line for 5 seconds:
   - Measures maximum, minimum, and noise floor per sensor channel.
   - Filters single-sample spikes using a 3-sample median filter (`med3`).
   - Automatically detects whether the track has a black line on white surface or white line on black surface.
4. If at least 7 sensors register a minimum contrast span (`MIN_RANGE = 150`), the robot enters **`READY`** state.
5. If calibration fails, the robot enters **`IDLE`** with an explanatory message.

---

## 8. Live PID & Speed Tuning

All parameters can be tuned live over Wi-Fi while stationary or during a run:

- **`Kp` (Proportional)**: Primary steering response. Increases responsiveness to deviation.
- **`Ki` (Integral)**: Steady-state error accumulator. Keep low (0.000 to 0.005) for high-speed tracks to prevent windup oscillation.
- **`Kd` (Derivative)**: Dampens high-frequency wobble and counteracts overshoot.
- **`Speed`**: Maximum speed (`lfSpeed`) achieved on straightaways (0..255).
- **`Turn`**: Reduced speed (`turnSpeed`) applied during sharp corners when outer sensors trigger.
- **`Start`**: Launch speed (`startSpeed`) used when leaving the starting line.

> Slider movements update the robot's RAM **instantly** without rebooting or stopping motors. Slider network requests are throttled to 100 ms to protect Wi-Fi bandwidth.

---

## 9. Persistent Storage (RAM vs. NVS Flash)

| Memory Type | What It Holds | Persistence |
| :--- | :--- | :--- |
| **Live RAM** | Current `Kp`, `Ki`, `Kd`, speeds, sensor readings | Active immediately; lost on power-off |
| **NVS Flash** | Saved tuning parameters and calibration tables | Permanent; restored automatically on boot |

### How to Save
- Moving a slider changes **RAM only**. It does **not** write to flash.
- To permanently retain settings across reboots, click **Save Settings** in the Web UI.
- On reboot, the ESP32 validates the stored version header (`CONFIG_MAGIC = "LFR1"`). If valid, it loads your settings and enters **`READY`** immediately without requiring recalibration.

---

## 10. Restoring Defaults & Resetting

- **Load Defaults**: Restores factory-tuned PID (`Kp=0.1, Kd=1.0, Ki=0`) and speeds (`230 / 80 / 180`) in live RAM.
- **Reset Calibration**: Erases stored calibration arrays and reverts the robot to **`IDLE`** (uncalibrated) state.

---

## 11. Troubleshooting

### Robot does not start (Button disabled)
- The robot is in **`IDLE`** state because it has not been calibrated or stored calibration was cleared. Run calibration first.

### Upload fails / Serial port not found
- Ensure ESP32 is connected with a data-capable USB cable.
- Run `ls /dev/cu.*` or `ls /dev/ttyUSB*` to find the device.
- Upload explicitly: `./scripts/upload.sh /dev/cu.usbserial-XXXX`.

### Calibration fails ("only X/16 sensors saw the line")
- Ensure the robot physically spins during the 5-second calibration window. If motors stall, increase `CAL_SPEED` in `include/config.h`.
- Check sensor height: reflectance sensors should be 3 mm to 6 mm above the track.

### Web UI sliders not updating robot
- Verify connection to `LineFollower_AP` (IP `192.168.4.1`).
- Disable mobile cellular data if your phone tries to bypass local Wi-Fi.

### Settings lost after power cycle
- Ensure you clicked **Save Settings** in the Web UI after adjusting parameters.
