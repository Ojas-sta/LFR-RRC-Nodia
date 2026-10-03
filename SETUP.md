# Complete Setup & Hardware Wiring Guide

← [Back to README](README.md)

---

# Quick Upload

For an already assembled robot:

```bash
git clone https://github.com/Ojas-sta/LFR-RRC-Nodia.git
cd LFR-RRC-Nodia
```

Install PlatformIO if it is not already installed:
```bash
pip install -U platformio
```

Connect your ESP32 via USB and run:

### Windows
Depending on your terminal:
- **Command Prompt (`cmd.exe`)**:
  ```cmd
  upload.bat
  ```
- **PowerShell** *(Default in Windows & VS Code)*:
  ```powershell
  .\upload.bat
  ```
  *(Or `.\upload.ps1`)*
- **Git Bash / WSL**:
  ```bash
  ./upload.sh
  ```
  *(Note: Include the slash `./`, not `.upload.sh`)*

### macOS / Linux
```bash
./upload.sh
```

### Universal PlatformIO Command (Any OS / Terminal)
```bash
pio run -t upload
```

*If you already have an assembled robot, you can stop reading here! For new builds, full pinouts, electrical safety warnings, calibration, and tuning instructions, continue reading below.*

---

This document provides the complete hardware, electrical, wiring, software installation, calibration, and troubleshooting guide for the **LFR-RRC-Nodia** dual-core line-following robot.

---

## 1. Hardware Requirements

### Core Components
| Component | Specification / Recommendation | Notes |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-WROOM-32 (30-pin or 38-pin DevKit) | Dual-core 240 MHz Xtensa LX6 |
| **Sensor Board** | 16-channel analog IR reflectance sensor array | Multiplexed via CD74HC4067 |
| **Motor Driver** | TB6612FNG Dual H-bridge module | High-efficiency MOSFET driver (1.2A continuous / 3.2A peak) |
| **Motors** | 2× Micro Metal Gear DC N20 motors | 6V, 500 to 1000 RPM recommended |
| **Chassis & Wheels** | 2-wheel differential drive with front castor / ball | Lightweight carbon fiber or PCB chassis |
| **Battery Power** | 2S LiPo battery (7.4V nominal) | High discharge rate (e.g., 300–800 mAh, 25C+) |
| **Voltage Regulation** | 5V step-down (buck converter or linear regulator) | Powers ESP32 5V/VIN pin cleanly |
| **USB Cable** | Micro-USB or USB-C data cable | **Must support data transfer**, not charge-only |

### Software & Toolchain Prerequisites
- [VS Code (Visual Studio Code)](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode) (or [PlatformIO Core CLI](https://platformio.org/install/cli))
- USB-to-UART Bridge Drivers (typically CP210x, CH340, or FTDI depending on your ESP32 board)

---

## 2. Hardware Wiring & Pinout

The firmware uses the verified **ESP32 v8 Pinout** defined in [`include/config.h`](include/config.h). All connections must strictly follow this table:

### Complete GPIO Connection Table

| Peripheral Module | Module Pin | ESP32 Pin | GPIO # | Signal Type | Electrical Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CD74HC4067 Multiplexer** | `S0` | D32 | **GPIO 32** | Digital Output | Channel select bit 0 (LSB) |
| | `S1` | D33 | **GPIO 33** | Digital Output | Channel select bit 1 |
| | `S2` | D25 | **GPIO 25** | Digital Output | Channel select bit 2 |
| | `S3` | D26 | **GPIO 26** | Digital Output | Channel select bit 3 (MSB) |
| | `SIG` (Analog Out) | D34 | **GPIO 34** | Analog Input | **ADC1 CH6** (0 to 3.3V range only!) |
| | `EN` (Enable) | D27 | **GPIO 27** | Digital Output | Active LOW (Firmware pulls LOW to enable) |
| | `VCC` | 3V3 | — | Power (3.3V) | Power from ESP32 3.3V rail |
| | `GND` | GND | — | Ground | Common system ground |
| **TB6612FNG Driver** | `AIN1` | D15 | **GPIO 15** | Digital Output | Left motor direction A |
| | `AIN2` | D14 | **GPIO 14** | Digital Output | Left motor direction B |
| | `PWMA` | D13 | **GPIO 13** | PWM Output | Left motor speed (20 kHz LEDC PWM) |
| | `BIN1` | D19 | **GPIO 19** | Digital Output | Right motor direction A |
| | `BIN2` | D21 | **GPIO 21** | Digital Output | Right motor direction B |
| | `PWMB` | D18 | **GPIO 18** | PWM Output | Right motor speed (20 kHz LEDC PWM) |
| | `STBY` | D23 | **GPIO 23** | Digital Output | Active HIGH standby (Firmware drives HIGH) |
| | `VM` | Battery (+) | — | Motor Power | Connect to battery positive (e.g. 7.4V) |
| | `VCC` | 3V3 | — | Logic Power | Connect to ESP32 3.3V logic rail |
| | `GND` (both pins) | GND | — | Ground | Common system ground |
| | `AO1` / `AO2` | Motor 1 | — | Power Output | Connect to Left DC Motor |
| | `BO1` / `BO2` | Motor 2 | — | Power Output | Connect to Right DC Motor |
| **User Interface** | `CAL Button` | D4 | **GPIO 4** | Digital Input | Active LOW with internal pull-up |
| | `START Button` | D5 | **GPIO 5** | Digital Input | Active LOW with internal pull-up |
| | `Status LED` | D2 | **GPIO 2** | Digital Output | Active HIGH on-board status LED |

---

## 3. Critical Electrical Warnings

> [!CAUTION]
> **1. Common Ground is Mandatory**: All grounds (Battery negative, ESP32 GND, TB6612 GND, Sensor Board GND, and 5V regulator GND) **must be tied together**. Missing common ground causes floating ADC readings, erratic motor PWM, and logic lockups.

> [!WARNING]
> **2. Motor Power Isolation (VM vs. VCC)**:
> - `VM` on the TB6612FNG must connect to battery positive ($7.4\text{V}$).
> - `VCC` on the TB6612FNG and the sensor array must connect to **ESP32 3.3V**.
> - **NEVER** connect motor battery voltage ($7.4\text{V}$) to the ESP32 3.3V pin or to any GPIO pin. Doing so will permanently destroy the ESP32 SoC.

> [!WARNING]
> **3. Sensor Signal Voltage Limits**:
> - GPIO 34 (`MUX_SIG`) is connected to the internal ADC1 converter.
> - The maximum allowable voltage on GPIO 34 is **$3.3\text{V}$**.
> - Ensure the sensor board is powered from $3.3\text{V}$ so its analog output cannot exceed this limit.

> [!NOTE]
> **4. Standby Pin (STBY)**:
> - The TB6612FNG H-bridge outputs will remain completely high-impedance (disconnected) if `STBY` is floating or grounded.
> - The firmware explicitly drives GPIO 23 HIGH during initialization. Ensure GPIO 23 is physically wired to the `STBY` header.

> [!TIP]
> **5. Motor Polarity Verification**:
> - Motor 1 represents the **Left wheel**; Motor 2 represents the **Right wheel**.
> - Forward drive sets `AIN1=HIGH, AIN2=LOW` and `BIN1=HIGH, BIN2=LOW`.
> - If either motor spins in reverse during forward motion, simply swap the two motor wires connected to that channel's terminal block.

---

## 4. Software Setup & Installation

### Step 1: Install Development Environment
1. Download and install [Visual Studio Code](https://code.visualstudio.com/).
2. Open VS Code, navigate to the **Extensions** view (`Ctrl+Shift+X` / `Cmd+Shift+X`), search for **PlatformIO IDE**, and click **Install**.
3. Restart VS Code when prompted to complete PlatformIO initialization.

### Step 2: Clone the Repository
Open a terminal and clone the firmware repository:
```bash
git clone https://github.com/Ojas-sta/LFR-RRC-Nodia.git
cd LFR-RRC-Nodia
```

### Step 3: Open in PlatformIO
1. In VS Code, go to **File $\rightarrow$ Open Folder...** and select the cloned `LFR-RRC-Nodia` folder.
2. PlatformIO will automatically read [`platformio.ini`](platformio.ini) and install the required compiler toolchain and framework packages (`framework-arduinoespressif32 @ 3.0.7`).

### Step 4: Connect ESP32
1. Plug the ESP32 into your computer using a data-capable USB cable.
2. Confirm the operating system detects the serial device:
   - **Linux**: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - **macOS**: `/dev/cu.usbserial-XXXX` or `/dev/cu.wchusbserialXXXX`
   - **Windows**: `COM3`, `COM4`, etc. in Device Manager

---

## 5. Firmware Upload

### Method A: One-Click Script (Recommended)
From the root of the repository:

- **Linux / macOS**:
  ```bash
  ./upload.sh
  ```
- **Windows**:
  Double-click `upload.bat` (or run `upload.bat` in Command Prompt).

The script compiles the project and flashes the firmware to the automatically detected serial port.

### Method B: PlatformIO IDE GUI
1. Click the **PlatformIO Alien icon** in the left sidebar.
2. Under **Project Tasks $\rightarrow$ esp32dev**, click **Build** to compile.
3. Click **Upload** to flash the ESP32.

### Method C: PlatformIO Core CLI
```bash
# Build firmware
pio run

# Upload firmware (auto-detect port)
pio run -t upload

# Upload to specific port
pio run -t upload --upload-port /dev/cu.usbserial-0001
```

---

## 6. First Boot & Verification Sequence

Follow this exact sequence after flashing:

```text
       Power ON Robot
             ↓
    Motors Remain Stopped (Fail-Safe)
    LED Blinks at 2 Hz (Uncalibrated IDLE)
             ↓
    Connect to Wi-Fi SoftAP:
    SSID     : LineFollower_AP
    Password : linefollow123
             ↓
    Open Browser to: http://192.168.4.1/
             ↓
    Place Sensor Array Over Track Line
             ↓
    Click "Calibrate" (or press GPIO 4 Button)
             ↓
    Robot Spins Left & Right for 5 Seconds
             ↓
    Calibration Evaluated:
    - Min 8 valid sensors (contrast >= 150)
    - Auto-polarity detected (HIGH or LOW)
             ↓
    Status: READY (LED Solid On / Blue Badge)
             ↓
    Click "Start" (or press GPIO 5 Button)
             ↓
    Active Line-Following at 1250 Hz (RUNNING)
```

---

## 7. Calibration Procedure

Calibration maps the physical reflectance range of every individual phototransistor on your track surface and determines line polarity.

### Execution
1. Position the robot so the center of the 16-sensor array rests directly over the track line.
2. Press the **physical Calibrate button** (GPIO 4) or click **Calibrate** on the web dashboard.
3. The robot executes the calibrated 2-phase spin routine:
   - **1-second pause** for operator safety.
   - **Phase 1 (2.5s)**: Spins counter-clockwise at PWM 90 (`motor1run(90), motor2run(-90)`).
   - **150 ms pause**: Short-brakes both motors.
   - **Phase 2 (2.5s)**: Spins clockwise at PWM 90 (`motor1run(-90), motor2run(90)`).
   - Returns approximately to the initial starting angle.
4. **Noise Filtering**: All raw ADC readings during the spin are filtered through a 3-sample median filter (`med3`) to reject electrical motor noise and optical spikes.
5. **Polarity Voting**: The algorithm evaluates the mean vs. min/max range across all valid sensors to determine whether the track has a black line on white surface or white line on black surface.

### Success vs. Failure Criteria
- **SUCCESS (`READY` state)**: At least 8 sensors measure a contrast span ($max - min$) $\ge 150$ ADC units. The web dashboard turns blue (`READY`), and the status message reads: `Calibration OK: line reads HIGH (or LOW).`
- **FAILURE (`IDLE` state)**: Fewer than 8 sensors detect sufficient contrast. Motors remain disabled, status message explains the failure, and the LED continues blinking.

---

## 8. Web Dashboard & Live Tuning

Connect to the robot's local Wi-Fi Access Point:
- **SSID**: `LineFollower_AP`
- **Password**: `linefollow123`
- **URL**: `http://192.168.4.1/`

### Dashboard Interface
- **State Badge**: Shows current state (`IDLE`, `CALIBRATING`, `READY`, `RUNNING`).
- **Live 16-Sensor Bar Display**: Visualizes normalized reflectance ($0$ to $1000$) across channels 0 to 15 in real time (sampled at 20 Hz). Green bars indicate sensors actively detecting the line.
- **PID Tuning Sliders**:
  - `Kp` (0.00 to 5.00, step 0.01) — Factory default: **0.10**
  - `Ki` (0.0000 to 1.0000, step 0.0005) — Factory default: **0.0000**
  - `Kd` (0.00 to 20.00, step 0.05) — Factory default: **1.00**
  - `Speed` (0 to 255) — Top straightaway speed. Default: **230**
  - `Turn` (0 to 255) — Cornering speed on sharp turns. Default: **80**
  - `Start` (0 to 255) — Initial launch speed. Default: **180**
- **Action Buttons**:
  - **Start / Stop**: Toggles between stationary `READY` and active `RUNNING`.
  - **Calibrate**: Initiates 5-second calibration spin.
  - **Save Settings**: Persists current RAM tuning and calibration tables into **NVS flash memory**.
  - **Load Defaults**: Restores compile-time factory tuning values into RAM.
  - **Reset Calibration**: Clears calibration tables from RAM and flash, returning robot to `IDLE`.

---

## 9. First Physical Track Run

1. **Bench Lift Test**:
   - Elevate the robot chassis so wheels spin freely in the air.
   - Power on, calibrate, and press Start.
   - Move a piece of line track under the sensor array:
     - Shift line left $\rightarrow$ Left wheel slows down, right wheel accelerates.
     - Shift line right $\rightarrow$ Right wheel slows down, left wheel accelerates.
     - Center line $\rightarrow$ Both wheels spin at equal speed.
2. **Low-Speed Track Validation**:
   - Place robot on physical track.
   - On the web UI, reduce `Speed` to `140`, `Turn` to `70`, and `Start` to `120`.
   - Press Start. Verify the robot smoothly follows curves without jumping off track.
3. **Full Competition Tuning**:
   - Gradually increase `Speed` towards `230`.
   - If robot oscillates on straight lines, reduce `Kp` slightly or increase `Kd`.
   - If robot misses sharp 90-degree corners, lower `Turn` speed.
   - Once tuned, click **Save Settings** to write parameters to non-volatile flash.

---

## 10. Comprehensive Troubleshooting Guide

### 1. PlatformIO Upload Fails
- **Symptom**: `Failed to connect to ESP32: Timed out waiting for packet header`
- **Solution**:
  - Hold down the **BOOT** button on the ESP32 while the upload terminal displays `Connecting...`, then release when flashing begins.
  - Verify USB cable supports data transfer (test with a smartphone or flash drive).
  - Ensure correct drivers are installed (Silicon Labs CP210x or WCH CH340).
  - On Linux, grant dialout group permissions: `sudo usermod -a -G dialout $USER`.

### 2. Robot Powers On But Does Not Move
- **Symptom**: Web dashboard shows `RUNNING`, but motors do not spin.
- **Solution**:
  - Check motor battery voltage on TB6612 `VM` terminal ($> 6.5\text{V}$ required).
  - Check common ground between battery, driver, and ESP32 GND.
  - Verify GPIO 23 is wired to TB6612 `STBY`.
  - Check that the robot was calibrated (`READY` state) before starting.

### 3. Robot Spins in Place or Drives Backwards
- **Symptom**: Wheels spin in reverse or robot immediately spins out of control.
- **Solution**:
  - If a motor runs backwards when commanded forward, reverse the two wire connections for that motor on the TB6612 terminal block.
  - Confirm Motor 1 is Left wheel and Motor 2 is Right wheel.

### 4. Calibration Fails ("only X/16 sensors saw the line")
- **Symptom**: Robot spins during calibration, but reverts to `IDLE` with failure message.
- **Solution**:
  - Adjust sensor ground clearance: optimal height is **3 mm to 6 mm** above track.
  - Verify multiplexer select lines S0..S3 (GPIO 32, 33, 25, 26) and signal line (GPIO 34).
  - Confirm track line contrast (black tape on white board or white tape on black board).
  - If motors stall during calibration spin, increase `CAL_SPEED` in [`include/config.h`](include/config.h).

### 5. Wi-Fi Access Point Unavailable
- **Symptom**: `LineFollower_AP` does not appear in phone Wi-Fi list.
- **Solution**:
  - Verify ESP32 power supply: Wi-Fi peak current draws up to 250 mA. If powered solely from a weak USB port, voltage dip may cause Wi-Fi brownout.
  - Open serial monitor (`pio device monitor -b 115200`) to confirm Core 0 task boot messages.

---

← [Back to README](README.md)
