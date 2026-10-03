#include "lfr_core.h"
#include "sync.h"

namespace LFRCore {

    // Active configuration consumed exclusively by Core 1
    static LFRConfig activeConfig;

    // Control-loop state variables (identical to reference implementation)
    static RobotState currentState = RobotState::IDLE;
    static int        currentCalPct = 0;

    static int        raw[NUM_SENSORS];
    static int        sensorValue[NUM_SENSORS];
    static int        sensorArray[NUM_SENSORS];
    static int        threshold[NUM_SENSORS];

    static int        P = 0, I = 0, D = 0;
    static int        previousError = 0;
    static int        PIDvalue = 0;
    static double     error = 0.0;
    static int        lsp = 0, rsp = 0;
    static int        currentSpeed = DEFAULT_START_SPEED;
    static int        lastTurnDir = 0; // 1 = left side (0/1), -1 = right side (12/13)
    static int        activeSensors = 0;
    static int        onLine = 1;

    static uint32_t   lastStepUs = 0;
    static uint32_t   lastTelemTime = 0;

    // ---------------- Hardware Initialization ----------------
    void initHardware() {
        // Multiplexer control pins
        pinMode(MUX_S0, OUTPUT);
        pinMode(MUX_S1, OUTPUT);
        pinMode(MUX_S2, OUTPUT);
        pinMode(MUX_S3, OUTPUT);
        pinMode(MUX_EN, OUTPUT);
        digitalWrite(MUX_EN, LOW); // Active LOW -> enable multiplexer
        pinMode(MUX_SIG, INPUT);

        // TB6612FNG motor driver control pins
        pinMode(AIN1, OUTPUT);
        pinMode(AIN2, OUTPUT);
        pinMode(BIN1, OUTPUT);
        pinMode(BIN2, OUTPUT);
        pinMode(STBY, OUTPUT);
        digitalWrite(STBY, HIGH);  // Active HIGH -> enable H-bridges

        // UI buttons & LED
        pinMode(CAL_BTN_PIN, INPUT_PULLUP);
        pinMode(START_BTN_PIN, INPUT_PULLUP);
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);

        // Attach LEDC PWM channels (Arduino-ESP32 3.x API)
        ledcAttach(PWMA, PWM_FREQ, PWM_RES);
        ledcAttach(PWMB, PWM_FREQ, PWM_RES);
        stopMotors();

        // ADC Configuration (12-bit, 0..3.3V attenuation)
        analogReadResolution(12);
        analogSetPinAttenuation(MUX_SIG, ADC_11db);

        // Pull initial configuration from shared state
        Sync::getConfigSnapshot(activeConfig);
        if (activeConfig.calibrated) {
            currentState = RobotState::READY;
            for (int i = 0; i < NUM_SENSORS; i++) {
                threshold[i] = (activeConfig.minValues[i] + activeConfig.maxValues[i]) / 2;
            }
        } else {
            currentState = RobotState::IDLE;
        }
    }

    // ---------------- Motor Primitives (Core 1 only) ----------------
    void motor1run(int motorSpeed) { // Motor 1 = Left
        motorSpeed = constrain(motorSpeed, -255, 255);
        if (motorSpeed > 0) {
            digitalWrite(AIN1, HIGH);
            digitalWrite(AIN2, LOW);
            ledcWrite(PWMA, motorSpeed);
        } else if (motorSpeed < 0) {
            digitalWrite(AIN1, LOW);
            digitalWrite(AIN2, HIGH);
            ledcWrite(PWMA, -motorSpeed);
        } else {
            digitalWrite(AIN1, HIGH);
            digitalWrite(AIN2, HIGH);
            ledcWrite(PWMA, 0);
        }
    }

    void motor2run(int motorSpeed) { // Motor 2 = Right
        motorSpeed = constrain(motorSpeed, -255, 255);
        if (motorSpeed > 0) {
            digitalWrite(BIN1, HIGH);
            digitalWrite(BIN2, LOW);
            ledcWrite(PWMB, motorSpeed);
        } else if (motorSpeed < 0) {
            digitalWrite(BIN1, LOW);
            digitalWrite(BIN2, HIGH);
            ledcWrite(PWMB, -motorSpeed);
        } else {
            digitalWrite(BIN1, HIGH);
            digitalWrite(BIN2, HIGH);
            ledcWrite(PWMB, 0);
        }
    }

    void stopMotors() {
        motor1run(0);
        motor2run(0);
    }

    // ---------------- Sensor Acquisition ----------------
    int sensorRead(int channel) {
        digitalWrite(MUX_S0, (channel >> 0) & 1);
        digitalWrite(MUX_S1, (channel >> 1) & 1);
        digitalWrite(MUX_S2, (channel >> 2) & 1);
        digitalWrite(MUX_S3, (channel >> 3) & 1);
        delayMicroseconds(MUX_SETTLE_US);

        int sum = 0;
        for (int k = 0; k < ADC_SAMPLES; k++) {
            sum += analogRead(MUX_SIG);
        }
        return sum / ADC_SAMPLES;
    }

    void readAllRaw(int rawOut[NUM_SENSORS]) {
        for (int i = 0; i < NUM_SENSORS; i++) {
            rawOut[i] = sensorRead(i);
        }
    }

    void readLine() {
        readAllRaw(raw);
        onLine = 0;
        for (int i = 0; i < NUM_SENSORS; i++) {
            if (!activeConfig.valid[i]) {
                sensorValue[i] = 0;
                sensorArray[i] = 0;
                continue;
            }
            int span = activeConfig.maxValues[i] - activeConfig.minValues[i];
            if (span <= 0) span = 1;
            long v = (long)(raw[i] - activeConfig.minValues[i]) * 1000L / span;
            if (!activeConfig.lineHigh) {
                v = 1000 - v;
            }
            sensorValue[i] = constrain((int)v, 0, 1000);
            sensorArray[i] = (sensorValue[i] > SEEN_THRESHOLD) ? 1 : 0;
            if (sensorArray[i]) {
                onLine = 1;
            }
        }
    }

    // ---------------- Calibration Filter & Routine ----------------
    static inline int med3(int a, int b, int c) {
        if (a > b) { int t = a; a = b; b = t; }
        int m = (b < c) ? b : c;
        return (a > m) ? a : m;
    }

    void calibrate() {
        currentState = RobotState::CALIBRATING;
        currentCalPct = 0;
        Sync::setStatusMessage("Calibrating... robot will spin left and right over the line.");

        int p1[NUM_SENSORS], p2[NUM_SENSORS];
        uint32_t sum[NUM_SENSORS];
        uint32_t n = 0;

        readAllRaw(raw);
        for (int i = 0; i < NUM_SENSORS; i++) {
            p1[i] = p2[i] = raw[i];
            activeConfig.minValues[i] = 4095;
            activeConfig.maxValues[i] = 0;
            sum[i] = 0;
        }

        // Two phases: spin one way, then opposite way (returns roughly over line)
        for (int phase = 0; phase < 2; phase++) {
            int dir = (phase == 0) ? 1 : -1;
            uint32_t t0 = millis();
            while (millis() - t0 < (CAL_TIME_MS / 2)) {
                motor1run(dir * CAL_SPEED);
                motor2run(-dir * CAL_SPEED);

                readAllRaw(raw);
                for (int i = 0; i < NUM_SENSORS; i++) {
                    int m = med3(p2[i], p1[i], raw[i]); // Reject single-sample spikes
                    p2[i] = p1[i];
                    p1[i] = raw[i];
                    if (m < activeConfig.minValues[i]) activeConfig.minValues[i] = m;
                    if (m > activeConfig.maxValues[i]) activeConfig.maxValues[i] = m;
                    sum[i] += m;
                }
                n++;
                currentCalPct = (int)((phase * (CAL_TIME_MS / 2) + (millis() - t0)) * 100 / CAL_TIME_MS);
                digitalWrite(LED_PIN, (millis() / 100) & 1);

                // Publish telemetry so Web UI updates spin percentage
                if (millis() - lastTelemTime > 50) {
                    lastTelemTime = millis();
                    LFRTelemetry telem;
                    telem.state = currentState;
                    telem.calPct = currentCalPct;
                    Sync::publishTelemetry(telem);
                }
            }
            stopMotors();
            delay(150);
        }

        stopMotors();
        digitalWrite(LED_PIN, LOW);

        // Evaluate sensor quality & detect line polarity
        int ok = 0, votesHigh = 0, votesLow = 0, best = 0, clip = 0;
        for (int i = 0; i < NUM_SENSORS; i++) {
            int range = activeConfig.maxValues[i] - activeConfig.minValues[i];
            if (range > best) best = range;
            if (activeConfig.maxValues[i] >= 4050) clip++;
            activeConfig.valid[i] = (range >= MIN_RANGE);
            if (activeConfig.valid[i]) {
                ok++;
                float mean = (float)sum[i] / (float)n;
                // Background is seen most of the time: if mean near min, background is LOW -> line reads HIGH
                if ((mean - activeConfig.minValues[i]) / (float)range < 0.5f) {
                    votesHigh++;
                } else {
                    votesLow++;
                }
            }
            threshold[i] = (activeConfig.minValues[i] + activeConfig.maxValues[i]) / 2;
        }

        activeConfig.lineHigh = AUTO_POLARITY ? (votesHigh >= votesLow) : (bool)IS_BLACK_LINE;

        // Print calibration report to serial
        Serial.print("min: "); for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(activeConfig.minValues[i]); Serial.print(" "); } Serial.println();
        Serial.print("max: "); for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(activeConfig.maxValues[i]); Serial.print(" "); } Serial.println();
        Serial.print("thr: "); for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(threshold[i]); Serial.print(" "); } Serial.println();
        Serial.print("ok : "); for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(activeConfig.valid[i] ? "y " : "- "); } Serial.println();

        char m[160];
        if (ok < MIN_VALID) {
            activeConfig.calibrated = false;
            currentState = RobotState::IDLE;
            snprintf(m, sizeof(m),
                     "Calibration FAILED: only %d/%d sensors saw the line (best range %d). "
                     "Put the line under the array; check the robot actually spins.", ok, NUM_SENSORS, best);
            Sync::setStatusMessage(m);
            Sync::publishCalibrationResult(activeConfig.minValues, activeConfig.maxValues, activeConfig.valid, activeConfig.lineHigh, false);
            return;
        }

        activeConfig.calibrated = true;
        currentState = RobotState::READY;
        currentCalPct = 100;
        snprintf(m, sizeof(m), "Calibration OK: %d/%d sensors, line reads %s. Place robot on line and press Start.%s",
                 ok, NUM_SENSORS, activeConfig.lineHigh ? "HIGH" : "LOW",
                 clip > 4 ? " WARNING: sensors clipping at ADC max (SIG > 3.1 V?)" : "");
        Sync::setStatusMessage(m);
        Sync::publishCalibrationResult(activeConfig.minValues, activeConfig.maxValues, activeConfig.valid, activeConfig.lineHigh, true);
    }

    // ---------------- PID Line Follow (Behavioral Reference) ----------------
    void linefollow() {
        error = 0;
        activeSensors = 0;

        for (int i = 0; i < NUM_SENSORS; i++) {
            if (sensorArray[i]) {
                error += SENSOR_WEIGHTS[i] * sensorArray[i] * sensorValue[i];
            }
            activeSensors += sensorArray[i];
        }
        error = error / activeSensors; // activeSensors >= 1 guaranteed when onLine == 1

        P = (int)error;
        I = I + (int)error;
        D = (int)error - previousError;

        PIDvalue = (int)((activeConfig.Kp * P) + (activeConfig.Ki * I) + (activeConfig.Kd * D));
        previousError = (int)error;

        lsp = currentSpeed - PIDvalue;
        rsp = currentSpeed + PIDvalue;

        lsp = constrain(lsp, PWM_MIN, PWM_MAX);
        rsp = constrain(rsp, PWM_MIN, PWM_MAX);

        motor1run(lsp);
        motor2run(rsp);
    }

    // ---------------- Single Iteration of ~1250 Hz Loop ----------------
    void runStep() {
        // Deterministic microsecond pacing: busy-wait until 800 µs elapsed
        while ((uint32_t)(micros() - lastStepUs) < LOOP_PERIOD_US) {}
        lastStepUs = micros();

        readLine();

        // Remember which side the line was last detected on
        if (sensorArray[0] || sensorArray[1]) {
            lastTurnDir = 1;
        } else if (sensorArray[12] || sensorArray[13]) {
            lastTurnDir = -1;
        }

        // Outer sensors see the line = sharp turn, so reduce speed
        if (sensorArray[0] || sensorArray[1] || sensorArray[12] || sensorArray[13]) {
            currentSpeed = activeConfig.turnSpeed;
        } else if (currentSpeed < activeConfig.lfSpeed) {
            currentSpeed++; // Ramp back up to full line-follow speed
        }

        if (onLine == 1) {
            linefollow();
            digitalWrite(LED_PIN, HIGH);
        } else {
            // Line lost: spin toward last known side
            digitalWrite(LED_PIN, LOW);
            currentSpeed = activeConfig.turnSpeed;

            if (lastTurnDir == 1) {
                motor1run(LOST_REV);
                motor2run(LOST_FWD);
            } else if (lastTurnDir == -1) {
                motor1run(LOST_FWD);
                motor2run(LOST_REV);
            } else {
                if (error >= 0) {
                    motor1run(LOST_REV);
                    motor2run(LOST_FWD);
                } else {
                    motor1run(LOST_FWD);
                    motor2run(LOST_REV);
                }
            }
        }
    }

    // ---------------- Start / Stop State Transitions ----------------
    void startRun() {
        if (currentState != RobotState::READY || !activeConfig.calibrated) {
            Sync::setStatusMessage("Calibrate first.");
            return;
        }
        P = I = D = previousError = PIDvalue = 0;
        error = 0.0;
        lsp = rsp = 0;
        lastTurnDir = 0;
        onLine = 1;
        currentSpeed = activeConfig.startSpeed;
        lastStepUs = micros();
        currentState = RobotState::RUNNING;
        Sync::setStatusMessage("Running.");
    }

    void stopRun() {
        stopMotors();
        digitalWrite(LED_PIN, LOW);
        currentState = RobotState::READY;
        Sync::setStatusMessage("Stopped. Press Start to run again.");
    }

    // ---------------- Physical Button Handling ----------------
    void handleButtons() {
        static bool lastCalBtn = HIGH;
        static bool lastStartBtn = HIGH;
        static uint32_t lastCalTime = 0;
        static uint32_t lastStartTime = 0;

        bool calBtn = digitalRead(CAL_BTN_PIN);
        bool startBtn = digitalRead(START_BTN_PIN);
        uint32_t now = millis();

        if (calBtn == LOW && lastCalBtn == HIGH && (now - lastCalTime > 300)) {
            lastCalTime = now;
            Sync::requestCalibrate();
        }
        lastCalBtn = calBtn;

        if (startBtn == LOW && lastStartBtn == HIGH && (now - lastStartTime > 300)) {
            lastStartTime = now;
            Sync::requestStartStopToggle();
        }
        lastStartBtn = startBtn;
    }

    // ---------------- Telemetry Aggregation ----------------
    static void publishCurrentTelemetry() {
        LFRTelemetry telem;
        telem.state        = currentState;
        telem.calPct       = currentCalPct;
        telem.Kp           = activeConfig.Kp;
        telem.Ki           = activeConfig.Ki;
        telem.Kd           = activeConfig.Kd;
        telem.lfSpeed      = activeConfig.lfSpeed;
        telem.turnSpeed    = activeConfig.turnSpeed;
        telem.startSpeed   = activeConfig.startSpeed;
        telem.lsp          = lsp;
        telem.rsp          = rsp;
        telem.currentSpeed = currentSpeed;
        telem.error        = error;
        telem.PIDvalue     = PIDvalue;
        telem.onLine       = onLine;

        for (int i = 0; i < NUM_SENSORS; i++) {
            telem.sensorValue[i] = sensorValue[i];
            telem.valid[i]       = activeConfig.valid[i];
        }
        Sync::publishTelemetry(telem);
    }

    // ---------------- Core 1 Main Task Loop ----------------
    void taskEntry(void* parameter) {
        initHardware();
        Serial.println("[Core 1] Real-time LFR task started on Core 1.");

        for (;;) {
            // 1. Physical Buttons
            handleButtons();

            // 2. Check for Live Configuration Updates from Core 0
            Sync::checkConfigUpdate(activeConfig);

            // 3. Process Inter-Core Requests
            if (Sync::consumeResetCalRequest()) {
                if (currentState == RobotState::RUNNING) {
                    stopRun();
                }
                activeConfig.resetCalibration();
                currentState = RobotState::IDLE;
                Sync::setStatusMessage("Calibration reset. Please calibrate before starting.");
            }

            if (Sync::consumeCalRequest()) {
                if (currentState == RobotState::RUNNING) {
                    Sync::setStatusMessage("Stop first to calibrate.");
                } else {
                    delay(1000); // 1-second safety pause before spin
                    calibrate();
                }
            }

            if (Sync::consumeStartRequest()) {
                if (currentState != RobotState::RUNNING) {
                    startRun();
                }
            }

            if (Sync::consumeStopRequest()) {
                if (currentState == RobotState::RUNNING) {
                    stopRun();
                }
            }

            if (Sync::consumeStartStopToggleRequest()) {
                if (currentState == RobotState::RUNNING) {
                    stopRun();
                } else {
                    startRun();
                }
            }

            // 4. Execution State Machine
            if (currentState == RobotState::RUNNING) {
                runStep();
            } else {
                // Not running: idle/ready behavior
                if (currentState == RobotState::IDLE) {
                    digitalWrite(LED_PIN, (millis() / 250) & 1); // 250ms blink = uncalibrated
                }

                // Sample sensors at 20 Hz while stationary for live Web UI bars
                static uint32_t lastSensorScan = 0;
                if (activeConfig.calibrated && (millis() - lastSensorScan > 50)) {
                    lastSensorScan = millis();
                    readLine();
                }

                // Yield to allow idle task servicing on Core 1 when not running
                delay(1);
            }

            // 5. Periodic Telemetry Publish (~20 Hz)
            if (millis() - lastTelemTime > 50) {
                lastTelemTime = millis();
                publishCurrentTelemetry();
            }
        }
    }

} // namespace LFRCore
