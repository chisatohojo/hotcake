#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VL53L0X.h>
#include <math.h>
#include <string.h>

#include "config.h"

// SSD1306 otherwise restores 100 kHz after each transfer, including display().
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN,
                        I2C_CLOCK_HZ, I2C_CLOCK_HZ);
Adafruit_VL53L0X tof;

struct IoData {
  float temperatureSetC = TEMP_SET_MIN_C;
  float pressTimeSetS = PRESS_TIME_MIN_S;
  float heightSetMm = HEIGHT_SET_MIN_MM;

  uint16_t pt100Adc = 0;
  float pt100ResistanceOhm = NAN;
  float temperatureActualC = NAN;
  bool pt100Valid = false;

  uint16_t tofDistanceMm = 0;
  uint8_t tofRangeStatus = 255;
  VL53L0X_Error tofApiError = VL53L0X_ERROR_NONE;
  bool tofValid = false;
  bool tofFilterInitialized = false;
  float tofFilteredDistanceMm = NAN;
  unsigned long tofLastValidMs = 0;
  bool tofApiAttempted = false;
  bool tofDiagnosticAvailable = false;
  uint16_t tofRawDistanceMm = 0;
  uint32_t tofSignalRateRaw = 0;
  uint32_t tofAmbientRateRaw = 0;
  uint16_t tofSpadCountRaw = 0;

  bool startPressed = false;
};

IoData ioData;

enum class ActuatorState : uint8_t {
  IDLE, MOVE_UP_INITIAL, MOVE_DOWN, HOLD, MOVE_UP_RETURN
};
ActuatorState actuatorState = ActuatorState::IDLE;
uint8_t motorPwmOutput = 0;
uint8_t motorDirOutput = LOW;
float cycleTargetMm = ACTUATOR_LOWER_MIN_MM;
unsigned long cycleHoldMs = 0;
unsigned long holdStartedMs = 0;


struct TofStatistics {
  uint32_t attempts;
  uint32_t accepted;
  uint32_t apiErrors;
  uint32_t statusCounts[256];
  unsigned long startedAtMs;
};

TofStatistics tofStatistics = {};
uint32_t tofStatisticsWindow = 0;
bool tofStatisticsResetRequested = false;

bool oledReady = false;
bool tofReady = false;
bool tofSettingsChecked = false;
bool tofSettingsVerified = false;
bool tofRitReadbackAvailable = false;
uint8_t tofRitReadback = 0;
bool analogFilterInitialized = false;
bool i2cAddressDetected[128] = {};

float filteredTempAdc = 0.0F;
float filteredTimeAdc = 0.0F;
float filteredHeightAdc = 0.0F;

bool switchLastRawPressed = false;
bool switchStablePressed = false;
unsigned long switchLastChangeMs = 0;

bool buzzerActive = false;
unsigned long buzzerStopMs = 0;

unsigned long lastSensorMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastSerialMs = 0;

float mapAdcToFloat(float adc, float outputMin, float outputMax) {
  return outputMin + (adc / 1023.0F) * (outputMax - outputMin);
}

float updateEma(float previous, float sample) {
  return previous + ANALOG_FILTER_ALPHA * (sample - previous);
}

bool tofControlValid() {
  return ioData.tofFilterInitialized &&
         (millis() - ioData.tofLastValidMs < TOF_FILTER_TIME_CONSTANT_MS);
}

float tofControlDistanceMm() {
  return ioData.tofFilteredDistanceMm;
}

bool timeReached(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

void stopMotor() {
  analogWrite(PIN_MOTOR_PWM, 0);
  motorPwmOutput = 0;
}

void enforceSafeOutputs() {
  digitalWrite(PIN_SSR, LOW);
  digitalWrite(PIN_SSR_HOTPLATE, LOW);
  if (!ACTUATOR_SETUP_CONFIRMED || !tofControlValid() ||
      actuatorState == ActuatorState::IDLE || actuatorState == ActuatorState::HOLD) {
    stopMotor();
  }
}

void driveMotor(bool up) {
  if (!ACTUATOR_SETUP_CONFIRMED) {
    stopMotor();
    return;
  }
  const uint8_t direction = up ? MOTOR_DIR_UP_LEVEL : MOTOR_DIR_DOWN_LEVEL;
  if (motorDirOutput != direction) {
    stopMotor();  // Remove PWM before changing DIR.
    digitalWrite(PIN_MOTOR_DIR, direction);
    motorDirOutput = direction;
  }
  if (motorPwmOutput != MOTOR_PWM_RUN) {
    analogWrite(PIN_MOTOR_PWM, MOTOR_PWM_RUN);
    motorPwmOutput = MOTOR_PWM_RUN;
  }
}

void setBuzzerOutput(bool on) {
  digitalWrite(PIN_BUZZER, on ? BUZZER_ON_LEVEL : BUZZER_OFF_LEVEL);
}

void beep(unsigned long durationMs = START_BEEP_MS) {
  setBuzzerOutput(true);
  buzzerActive = true;
  buzzerStopMs = millis() + durationMs;
}

void updateBuzzer() {
  if (buzzerActive && timeReached(millis(), buzzerStopMs)) {
    setBuzzerOutput(false);
    buzzerActive = false;
  }
}

void scanI2cBus() {
  uint8_t foundCount = 0;
  Serial.println(F("I2C_SCAN_BEGIN"));

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      i2cAddressDetected[address] = true;
      Serial.print(F("I2C_DEVICE=0x"));
      if (address < 0x10) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
      ++foundCount;
    }
  }

  Serial.print(F("I2C_DEVICE_COUNT="));
  Serial.println(foundCount);
  Serial.println(F("I2C_SCAN_END"));
}

bool logTofLimitSetting(uint16_t limitId, const __FlashStringHelper* name,
                        const __FlashStringHelper* unit) {
  // Read value first: the ST signal-limit getter synchronizes its enable cache.
  const FixPoint1616_t value = tof.getLimitCheckValue(limitId);
  const VL53L0X_Error valueApi = tof.Status;
  const uint8_t enabled = tof.getLimitCheckEnable(limitId);
  const VL53L0X_Error enableApi = tof.Status;

  Serial.print(F("TOF_SETTING="));
  Serial.print(name);
  Serial.print(F(",ENABLE="));
  if (enableApi == VL53L0X_ERROR_NONE) {
    Serial.print(enabled);
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(F(",ENABLE_API="));
  Serial.print(static_cast<int>(enableApi));
  Serial.print(F(",VALUE="));
  if (valueApi == VL53L0X_ERROR_NONE) {
    Serial.print(static_cast<float>(value) / TOF_LIMIT_FIXED_SCALE, 6);
    Serial.print(F(",RAW="));
    Serial.print(value);
  } else {
    // Wrappers do not promise a usable return value on failure.
    Serial.print(F("NA,RAW=NA"));
  }
  Serial.print(F(",VALUE_API="));
  Serial.print(static_cast<int>(valueApi));
  Serial.print(F(",UNIT="));
  Serial.println(unit);

  if (limitId == VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD) {
    tofRitReadbackAvailable = (enableApi == VL53L0X_ERROR_NONE);
    if (tofRitReadbackAvailable) {
      tofRitReadback = enabled;
    }
  }
  return valueApi == VL53L0X_ERROR_NONE && enableApi == VL53L0X_ERROR_NONE;
}

bool logTofIntegerSetting(const __FlashStringHelper* name, uint32_t value,
                          VL53L0X_Error api, const __FlashStringHelper* unit) {
  Serial.print(F("TOF_SETTING="));
  Serial.print(name);
  Serial.print(F(",VALUE="));
  if (api == VL53L0X_ERROR_NONE) {
    Serial.print(value);
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(F(",API="));
  Serial.print(static_cast<int>(api));
  Serial.print(F(",UNIT="));
  Serial.println(unit);
  return api == VL53L0X_ERROR_NONE;
}

void configureAndLogTofSettings() {
  Serial.println(F("TOF_PROFILE_REQUEST=DEFAULT"));
  Serial.print(F("TOF_RIT_REQUEST="));
  Serial.println(TOF_RIT_ENABLED ? 1 : 0);
  if (!tofReady) {
    Serial.println(F("TOF_RIT_SET=NOT_RUN,TOF_RIT_SET_API=NOT_RUN"));
    Serial.print(F("TOF_SETTINGS=NOT_RUN,REASON="));
    Serial.println(i2cAddressDetected[TOF_I2C_ADDRESS] ? F("INIT_ERR") : F("NOT_FOUND"));
    return;  // No settings APIs before a successful begin().
  }

  tofSettingsChecked = true;
  // The only setting changed after DEFAULT initialization is the RIT enable bit.
  const bool setOk = tof.setLimitCheckEnable(
      VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, TOF_RIT_ENABLED ? 1 : 0);
  const VL53L0X_Error setApi = tof.Status;  // Capture before any other API call.
  bool allOk = setOk && setApi == VL53L0X_ERROR_NONE;
  Serial.print(F("TOF_RIT_SET="));
  Serial.print(allOk ? F("OK") : F("ERROR"));
  Serial.print(F(",TOF_RIT_SET_RETURN="));
  Serial.print(setOk ? 1 : 0);  // Wrapper returns boolean, not VL53L0X_Error.
  Serial.print(F(",TOF_RIT_SET_API="));
  Serial.println(static_cast<int>(setApi));

  // Evaluate every read even if a preceding setting/read failed.
  allOk &= logTofLimitSetting(VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
                             F("RIT"), F("MCPS/SPAD"));
  allOk &= logTofLimitSetting(VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                             F("SIGNAL_RATE_FINAL_RANGE"), F("MCPS"));
  allOk &= logTofLimitSetting(VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                             F("SIGMA_FINAL_RANGE"), F("mm"));

  uint32_t value = tof.getMeasurementTimingBudgetMicroSeconds();
  VL53L0X_Error api = tof.Status;
  allOk &= logTofIntegerSetting(F("TIMING_BUDGET"),
                                api == VL53L0X_ERROR_NONE ? value : 0, api, F("us"));
  value = tof.getVcselPulsePeriod(VL53L0X_VCSEL_PERIOD_PRE_RANGE);
  api = tof.Status;
  allOk &= logTofIntegerSetting(F("PRE_RANGE_VCSEL"),
                                api == VL53L0X_ERROR_NONE ? value : 0, api, F("PCLK"));
  value = tof.getVcselPulsePeriod(VL53L0X_VCSEL_PERIOD_FINAL_RANGE);
  api = tof.Status;
  allOk &= logTofIntegerSetting(F("FINAL_RANGE_VCSEL"),
                                api == VL53L0X_ERROR_NONE ? value : 0, api, F("PCLK"));

  const bool ritMatches = tofRitReadbackAvailable &&
                         tofRitReadback == (TOF_RIT_ENABLED ? 1 : 0);
  Serial.print(F("TOF_RIT_MATCH="));
  Serial.println(!tofRitReadbackAvailable ? F("UNKNOWN") :
                 (ritMatches ? F("YES") : F("NO")));
  tofSettingsVerified = allOk && ritMatches;
  Serial.print(F("TOF_SETTINGS="));
  Serial.println(tofSettingsVerified ? F("OK") : F("ERROR"));
  // A settings failure stays visible; ranging diagnostics remain available.
}

void initializeI2cDevices() {
  Wire.begin();
  Wire.setClock(I2C_CLOCK_HZ);
  scanI2cBus();

  oledReady = i2cAddressDetected[OLED_I2C_ADDRESS] &&
              display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS, true, false);
  Serial.print(F("OLED_INIT="));
  Serial.println(oledReady ? F("OK") : F("ERR"));

  if (oledReady) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Dorayaki I/O Check"));
    display.print(F("OLED: OK  ToF: ..."));
    display.display();
  }

  tofReady = i2cAddressDetected[TOF_I2C_ADDRESS] && tof.begin();
  // VL53L0X begin() calls Wire.begin(), resetting the AVR bus to 100 kHz.
  // Restore the comparison clock after either successful or failed initialization.
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.print(F("I2C_CLOCK_HZ="));
  Serial.println(I2C_CLOCK_HZ);
  Serial.print(F("TOF_INIT="));
  Serial.println(tofReady ? F("OK") : F("ERR"));
  configureAndLogTofSettings();

  if (oledReady) {
    display.setCursor(0, 24);
    display.print(F("ToF: "));
    display.println(tofReady ? F("OK") : F("ERR"));
    display.display();
  }
}

void readAnalogInputs() {
  const uint16_t tempAdc = analogRead(PIN_POT_TEMP);
  const uint16_t timeAdc = analogRead(PIN_POT_PRESS_TIME);
  const uint16_t heightAdc = analogRead(PIN_POT_HEIGHT);

  if (!analogFilterInitialized) {
    filteredTempAdc = tempAdc;
    filteredTimeAdc = timeAdc;
    filteredHeightAdc = heightAdc;
    analogFilterInitialized = true;
  } else {
    filteredTempAdc = updateEma(filteredTempAdc, tempAdc);
    filteredTimeAdc = updateEma(filteredTimeAdc, timeAdc);
    filteredHeightAdc = updateEma(filteredHeightAdc, heightAdc);
  }

  ioData.temperatureSetC = mapAdcToFloat(filteredTempAdc, TEMP_SET_MIN_C, TEMP_SET_MAX_C);
  ioData.pressTimeSetS = mapAdcToFloat(filteredTimeAdc, PRESS_TIME_MIN_S, PRESS_TIME_MAX_S);
  ioData.heightSetMm = mapAdcToFloat(filteredHeightAdc, HEIGHT_SET_MIN_MM, HEIGHT_SET_MAX_MM);
}

void readPt100() {
  ioData.pt100Adc = analogRead(PIN_PT100);
  ioData.pt100Valid = false;
  ioData.pt100ResistanceOhm = NAN;
  ioData.temperatureActualC = NAN;

  if (ioData.pt100Adc < PT100_ADC_MIN_VALID ||
      ioData.pt100Adc > PT100_ADC_MAX_VALID) {
    return;
  }

  const float denominator = 1023.0F - static_cast<float>(ioData.pt100Adc);
  if (denominator <= 0.0F) {
    return;
  }

  const float resistance = PT100_SERIES_RESISTOR_OHM *
                           static_cast<float>(ioData.pt100Adc) / denominator;
  const float ratio = resistance / PT100_R0_OHM;

  // Inverse Callendar-Van Dusen equation for T >= 0 degC:
  // R/R0 = 1 + A*T + B*T^2
  const float discriminant = PT100_CVD_A * PT100_CVD_A -
                             4.0F * PT100_CVD_B * (1.0F - ratio);
  if (!isfinite(resistance) || discriminant < 0.0F) {
    return;
  }

  float temperature = (-PT100_CVD_A + sqrt(discriminant)) / (2.0F * PT100_CVD_B);
  temperature = temperature * PT100_CAL_GAIN + PT100_CAL_OFFSET_C;

  if (!isfinite(temperature) || temperature < PT100_MIN_C || temperature > PT100_MAX_C) {
    return;
  }

  ioData.pt100ResistanceOhm = resistance;
  ioData.temperatureActualC = temperature;
  ioData.pt100Valid = true;
}

void resetTofStatistics() {
  memset(&tofStatistics, 0, sizeof(tofStatistics));
  tofStatistics.startedAtMs = millis();
  ++tofStatisticsWindow;
  Serial.print(F("TOF_STATS_RESET="));
  Serial.println(tofStatisticsWindow);
}

void readTofDiagnosticCommands() {
  // A bounded amount of input per loop; commands never control any output pin.
  for (uint8_t count = 0; count < TOF_COMMAND_BYTES_PER_LOOP && Serial.available() > 0;
       ++count) {
    const int command = Serial.read();
    if (command == 'r' || command == 'R') {
      tofStatisticsResetRequested = true;
    }
  }
}

void readTof() {
  // Reset immediately before the next sample so a new window cannot log old data.
  if (tofStatisticsResetRequested) {
    resetTofStatistics();
    tofStatisticsResetRequested = false;
  }
  ioData.tofValid = false;
  ioData.tofRangeStatus = 255;
  ioData.tofApiError = VL53L0X_ERROR_NONE;
  ioData.tofApiAttempted = false;
  ioData.tofDiagnosticAvailable = false;
  ioData.tofRawDistanceMm = 0;
  ioData.tofSignalRateRaw = 0;
  ioData.tofAmbientRateRaw = 0;
  ioData.tofSpadCountRaw = 0;
  if (!tofReady) {
    return;
  }

  VL53L0X_RangingMeasurementData_t measurement = {};
  ioData.tofApiAttempted = true;
  ++tofStatistics.attempts;
  ioData.tofApiError = tof.rangingTest(&measurement, false);
  if (ioData.tofApiError != VL53L0X_ERROR_NONE) {
    ++tofStatistics.apiErrors;
    return;  // Failed API calls must not expose stale or incomplete range data.
  }
  ioData.tofRangeStatus = measurement.RangeStatus;
  ioData.tofDiagnosticAvailable = true;
  ioData.tofRawDistanceMm = measurement.RangeMilliMeter;
  ioData.tofSignalRateRaw = measurement.SignalRateRtnMegaCps;
  ioData.tofAmbientRateRaw = measurement.AmbientRateRtnMegaCps;
  ioData.tofSpadCountRaw = measurement.EffectiveSpadRtnCount;
  ++tofStatistics.statusCounts[measurement.RangeStatus];

  // Only status 0 is accepted as a valid range; failures have diagnostic labels.
  if (measurement.RangeStatus == 0 && measurement.RangeMilliMeter > 0) {
    ioData.tofDistanceMm = measurement.RangeMilliMeter;
    ioData.tofValid = true;
    const unsigned long now = millis();
    if (!ioData.tofFilterInitialized) {
      ioData.tofFilteredDistanceMm = measurement.RangeMilliMeter;
      ioData.tofFilterInitialized = true;
    } else {
      const unsigned long elapsed = now - ioData.tofLastValidMs;
      const float alpha = 1.0F - expf(-static_cast<float>(elapsed) /
                                       static_cast<float>(TOF_FILTER_TIME_CONSTANT_MS));
      ioData.tofFilteredDistanceMm +=
          alpha * (static_cast<float>(measurement.RangeMilliMeter) -
                   ioData.tofFilteredDistanceMm);
    }
    ioData.tofLastValidMs = now;
    ++tofStatistics.accepted;
  }
}

const __FlashStringHelper* actuatorStateLabel() {
  switch (actuatorState) {
    case ActuatorState::IDLE: return F("IDLE");
    case ActuatorState::MOVE_UP_INITIAL: return F("MOVE_UP_INITIAL");
    case ActuatorState::MOVE_DOWN: return F("MOVE_DOWN");
    case ActuatorState::HOLD: return F("HOLD");
    case ActuatorState::MOVE_UP_RETURN: return F("MOVE_UP_RETURN");
    default: return F("UNKNOWN");
  }
}

void startActuatorCycle() {
  if (actuatorState != ActuatorState::IDLE) {
    return;  // A press during a cycle is never queued.
  }
  stopMotor();
  if (!ACTUATOR_SETUP_CONFIRMED) {
    Serial.println(F("ACTUATOR_START=DISABLED"));
    return;
  }
  // Freeze the existing filtered A1/A2 settings at the start-switch edge.
  cycleTargetMm = constrain(ioData.heightSetMm,
                            static_cast<float>(ACTUATOR_LOWER_MIN_MM),
                            static_cast<float>(ACTUATOR_UPPER_MM));
  cycleHoldMs = static_cast<unsigned long>(ioData.pressTimeSetS * 1000.0F);
  readTof();  // Obtain a fresh position before accepting the cycle.
  if (!ioData.tofValid || !tofControlValid()) {
    Serial.println(F("ACTUATOR_START=TOF_INVALID"));
    return;
  }
  actuatorState = ActuatorState::MOVE_UP_INITIAL;
}

void updateActuator() {
  if (actuatorState == ActuatorState::IDLE) {
    stopMotor();
    return;
  }
  if (!ACTUATOR_SETUP_CONFIRMED || !tofControlValid()) {
    stopMotor();
    actuatorState = ActuatorState::IDLE;
    Serial.println(F("ACTUATOR_ABORT=DISABLED_OR_TOF_INVALID"));
    return;  // No automatic restart when ranging recovers.
  }

  switch (actuatorState) {
    case ActuatorState::MOVE_UP_INITIAL:
      if (tofControlDistanceMm() >= ACTUATOR_UPPER_MM) {
        stopMotor();
        actuatorState = ActuatorState::MOVE_DOWN;
      } else {
        driveMotor(true);
      }
      break;
    case ActuatorState::MOVE_DOWN:
      if (tofControlDistanceMm() <= cycleTargetMm) {
        stopMotor();
        holdStartedMs = millis();
        actuatorState = ActuatorState::HOLD;
      } else {
        driveMotor(false);
      }
      break;
    case ActuatorState::HOLD:
      stopMotor();
      if (millis() - holdStartedMs >= cycleHoldMs) {
        actuatorState = ActuatorState::MOVE_UP_RETURN;
      }
      break;
    case ActuatorState::MOVE_UP_RETURN:
      if (tofControlDistanceMm() >= ACTUATOR_UPPER_MM) {
        stopMotor();
        actuatorState = ActuatorState::IDLE;
      } else {
        driveMotor(true);
      }
      break;
    default:
      stopMotor();
      actuatorState = ActuatorState::IDLE;
      break;
  }
}

void readStartSwitch() {
  const unsigned long now = millis();
  const bool rawPressed = (digitalRead(PIN_START_SWITCH) == LOW);

  if (rawPressed != switchLastRawPressed) {
    switchLastRawPressed = rawPressed;
    switchLastChangeMs = now;
  }

  if ((now - switchLastChangeMs) >= SWITCH_DEBOUNCE_MS &&
      switchStablePressed != rawPressed) {
    switchStablePressed = rawPressed;
    ioData.startPressed = switchStablePressed;

    if (switchStablePressed) {
      beep();
      startActuatorCycle();
    }
  }
}

void printValueOrError(float value, bool valid, uint8_t decimals) {
  if (valid) {
    display.print(value, decimals);
  } else {
    display.print(F("ERR"));
  }
}

const __FlashStringHelper* tofFailureLabel() {
  if (!tofReady) {
    return i2cAddressDetected[TOF_I2C_ADDRESS] ? F("INIT ERR") : F("NOT FOUND");
  }
  if (ioData.tofApiError == VL53L0X_ERROR_CONTROL_INTERFACE) {
    return F("I2C ERR");
  }
  if (ioData.tofApiError == VL53L0X_ERROR_TIME_OUT) {
    return F("TIMEOUT");
  }
  if (ioData.tofApiError != VL53L0X_ERROR_NONE) {
    return F("READ ERR");
  }

  // ST range status describes measurement quality, not an exact near/far cause.
  switch (ioData.tofRangeStatus) {
    case 0: return F("BAD DATA");  // Status 0 with a zero distance is still invalid.
    case 1: return F("UNSTABLE");  // Sigma check failed (measurement noise).
    case 2: return F("WEAK/FAR?"); // Signal check failed; far is only one possibility.
    case 3: return F("NEAR?");     // Minimum-range check failed.
    case 4: return F("RANGE?");    // Phase check failed (ambiguity or noise).
    case 5: return F("SENSOR ERR");
    case 255: return F("NO DATA");
    default: return F("UNKNOWN");
  }
}

void updateDisplay() {
  if (!oledReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextWrap(false);

  // Standard font: 6x8 cells. Widest diagnostic row is 18 characters (108 pixels).
  // Six rows at y=0,10,...,50 end at y=57, within the 128x64 display.
  display.setCursor(0, 0);
  display.print(F("Tset "));
  display.print(ioData.temperatureSetC, 0);
  display.print(F("C  "));
  display.print(ioData.pressTimeSetS, 1);
  display.print('s');

  display.setCursor(0, 10);
  display.print(F("Hset  "));
  display.print(ioData.heightSetMm, 0);
  display.print(F(" Tgt "));
  if (actuatorState == ActuatorState::IDLE) {
    display.print(F("---"));
  } else if (actuatorState == ActuatorState::MOVE_UP_INITIAL ||
             actuatorState == ActuatorState::MOVE_UP_RETURN) {
    display.print(ACTUATOR_UPPER_MM);
  } else {
    display.print(cycleTargetMm, 0);
  }

  display.setCursor(0, 20);
  display.print(F("Temp "));
  printValueOrError(ioData.temperatureActualC, ioData.pt100Valid, 1);
  if (ioData.pt100Valid) {
    display.print('C');
  }

  display.setCursor(0, 30);
  display.print(F("ToF  "));
  if (tofControlValid()) {
    display.print(tofControlDistanceMm(), 0);
    display.print(F("mm"));
  } else {
    display.print(tofFailureLabel());
    if (tofReady) {
      if (ioData.tofApiError != VL53L0X_ERROR_NONE) {
        display.print(' ');
        display.print(static_cast<int>(ioData.tofApiError));
      } else {
        display.print(F(" S"));
        display.print(ioData.tofRangeStatus);
      }
    }
  }

  display.setCursor(0, 40);
  display.print(F("SW "));
  display.print(ioData.startPressed ? F("LOW") : F("HIGH"));

  display.setCursor(0, 50);
  display.print(F("SSR OFF  PWM "));
  display.print(motorPwmOutput);
  display.display();
}

void printTofDiagnostics() {
  Serial.print(F(",I2C_CLOCK_HZ="));
  Serial.print(I2C_CLOCK_HZ);
  Serial.print(F(",TOF_DIAG="));
  if (!tofReady) {
    Serial.print(i2cAddressDetected[TOF_I2C_ADDRESS] ? F("INIT_ERR") : F("NOT_FOUND"));
  } else if (!ioData.tofApiAttempted) {
    Serial.print(F("NO_SAMPLE"));
  } else {
    Serial.print(ioData.tofDiagnosticAvailable ? F("AVAILABLE") : F("API_ERR"));
  }
  Serial.print(F(",TOF_API="));
  if (ioData.tofApiAttempted) {
    Serial.print(static_cast<int>(ioData.tofApiError));
  } else {
    Serial.print(F("NOT_RUN"));
  }
  Serial.print(F(",TOF_VALID="));
  Serial.print(ioData.tofValid ? 1 : 0);
  if (ioData.tofDiagnosticAvailable) {
    // Raw distance on a rejected range is diagnostic only, never a valid position.
    Serial.print(F(",TOF_DIST="));
    Serial.print(ioData.tofRawDistanceMm);
    Serial.print(F("mm,TOF_SIGNAL_MCPS="));
    Serial.print(static_cast<float>(ioData.tofSignalRateRaw) / TOF_RATE_FIXED_SCALE, 6);
    Serial.print(F(",TOF_AMBIENT_MCPS="));
    Serial.print(static_cast<float>(ioData.tofAmbientRateRaw) / TOF_RATE_FIXED_SCALE, 6);
    Serial.print(F(",TOF_SPAD="));
    Serial.print(static_cast<float>(ioData.tofSpadCountRaw) / TOF_SPAD_FIXED_SCALE, 4);
    // Keep exact integers as well, avoiding loss from AVR float/decimal rounding.
    Serial.print(F(",TOF_SIGNAL_RAW="));
    Serial.print(ioData.tofSignalRateRaw);
    Serial.print(F(",TOF_AMBIENT_RAW="));
    Serial.print(ioData.tofAmbientRateRaw);
    Serial.print(F(",TOF_SPAD_RAW="));
    Serial.print(ioData.tofSpadCountRaw);
  } else {
    Serial.print(F(",TOF_DIST=NA,TOF_SIGNAL_MCPS=NA,TOF_AMBIENT_MCPS=NA,TOF_SPAD=NA"));
    Serial.print(F(",TOF_SIGNAL_RAW=NA,TOF_AMBIENT_RAW=NA,TOF_SPAD_RAW=NA"));
  }

  Serial.print(F(",TOF_WINDOW="));
  Serial.print(tofStatisticsWindow);
  Serial.print(F(",TOF_WINDOW_MS="));
  Serial.print(millis() - tofStatistics.startedAtMs);
  Serial.print(F(",TOF_N="));
  Serial.print(tofStatistics.attempts);
  Serial.print(F(",TOF_OK="));
  Serial.print(tofStatistics.accepted);
  Serial.print(F(",TOF_API_ERRS="));
  Serial.print(tofStatistics.apiErrors);
  Serial.print(F(",TOF_BAD_RANGE="));
  Serial.print(tofStatistics.attempts - tofStatistics.accepted - tofStatistics.apiErrors);
  Serial.print(F(",TOF_STATUS_COUNTS="));
  bool firstStatus = true;
  for (uint16_t status = 0; status < 256; ++status) {
    if (tofStatistics.statusCounts[status] == 0) {
      continue;
    }
    if (!firstStatus) {
      Serial.print('|');
    }
    Serial.print(status);
    Serial.print(':');
    Serial.print(tofStatistics.statusCounts[status]);
    firstStatus = false;
  }
  if (firstStatus) {
    Serial.print(F("NONE"));
  }
  Serial.print(F(",TOF_RIT_REQUEST="));
  Serial.print(TOF_RIT_ENABLED ? 1 : 0);
  Serial.print(F(",TOF_RIT_ENABLE="));
  if (tofRitReadbackAvailable) {
    Serial.print(tofRitReadback);  // Boot API readback, not a new per-sample read.
  } else {
    Serial.print(F("NA"));
  }
  Serial.print(F(",TOF_SETTINGS="));
  Serial.print(!tofSettingsChecked ? F("NOT_RUN") :
               (tofSettingsVerified ? F("OK") : F("ERROR")));
}

void updateSerial() {
  Serial.print(F("TEMP_SET="));
  Serial.print(ioData.temperatureSetC, 1);
  Serial.print(F("C,TEMP_ACT="));
  if (ioData.pt100Valid) {
    Serial.print(ioData.temperatureActualC, 1);
    Serial.print(F("C,PT100_R="));
    Serial.print(ioData.pt100ResistanceOhm, 2);
    Serial.print(F("ohm,PT100_ADC="));
    Serial.print(ioData.pt100Adc);
  } else {
    Serial.print(F("ERR,PT100_R=ERR,PT100_ADC="));
    Serial.print(ioData.pt100Adc);
  }

  Serial.print(F(",TIME_SET="));
  Serial.print(ioData.pressTimeSetS, 1);
  Serial.print(F("s,HEIGHT_SET="));
  Serial.print(ioData.heightSetMm, 0);
  Serial.print(F("mm,TOF="));
  if (tofControlValid()) {
    Serial.print(tofControlDistanceMm(), 0);
    Serial.print(F("mm"));
  } else {
    Serial.print(F("ERR"));
  }
  Serial.print(F(",TOF_STATUS="));
  if (tofReady) {
    Serial.print(ioData.tofRangeStatus);
  } else {
    Serial.print(F("INIT_ERR"));
  }

  Serial.print(F(",START_SW="));
  Serial.print(ioData.startPressed ? F("LOW") : F("HIGH"));
  Serial.print(F(",SSR=OFF,MOTOR_PWM="));
  Serial.print(motorPwmOutput);
  Serial.print(F(",HOTPLATE_SSR=OFF"));
  printTofDiagnostics();
  Serial.print(F(",ACTUATOR_STATE="));
  Serial.print(actuatorStateLabel());
  Serial.print(F(",ACTUATOR_SETUP_CONFIRMED="));
  Serial.print(ACTUATOR_SETUP_CONFIRMED ? 1 : 0);
  Serial.print(F(",CYCLE_TARGET_MM="));
  Serial.print(cycleTargetMm, 1);
  Serial.print(F(",CYCLE_HOLD_MS="));
  Serial.print(cycleHoldMs);
  Serial.println();
}

void setup() {
  // Set safe output values before changing pins to OUTPUT, minimizing startup transients.
  digitalWrite(PIN_MOTOR_DIR, LOW);
  digitalWrite(PIN_MOTOR_PWM, LOW);
  digitalWrite(PIN_SSR, LOW);
  digitalWrite(PIN_SSR_HOTPLATE, LOW);
  digitalWrite(PIN_BUZZER, BUZZER_OFF_LEVEL);

  pinMode(PIN_START_SWITCH, INPUT_PULLUP);  // Internal pull-up; NO switch connects D4 to GND.
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_PWM, OUTPUT);
  pinMode(PIN_SSR, OUTPUT);
  pinMode(PIN_SSR_HOTPLATE, OUTPUT);
  enforceSafeOutputs();
  setBuzzerOutput(false);

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("DORAYAKI_IO_CHECK_START"));
  Serial.println(F("SAFETY: SSR=OFF, MOTOR_PWM=0, HOTPLATE_SSR=OFF"));

  switchLastRawPressed = (digitalRead(PIN_START_SWITCH) == LOW);
  switchStablePressed = switchLastRawPressed;
  ioData.startPressed = switchStablePressed;
  switchLastChangeMs = millis();

  initializeI2cDevices();

  readAnalogInputs();
  readPt100();
  resetTofStatistics();
  readTof();
  updateDisplay();
  updateSerial();

  const unsigned long now = millis();
  lastSensorMs = now;
  lastDisplayMs = now;
  lastSerialMs = now;
}

void loop() {
  const unsigned long now = millis();

  // Both SSRs stay LOW; disabled/idle/hold/invalid-position motor stays stopped.
  enforceSafeOutputs();
  readStartSwitch();
  updateBuzzer();
  readTofDiagnosticCommands();

  if ((now - lastSensorMs) >= SENSOR_INTERVAL_MS) {
    lastSensorMs = now;
    readAnalogInputs();
    readPt100();
    readTof();
  }

  updateActuator();

  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }

  if ((now - lastSerialMs) >= SERIAL_INTERVAL_MS) {
    lastSerialMs = now;
    updateSerial();
  }
}
