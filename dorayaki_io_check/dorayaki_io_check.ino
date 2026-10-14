#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VL53L0X.h>
#include <math.h>

#include "config.h"

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);
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

  bool startPressed = false;
};

IoData ioData;

bool oledReady = false;
bool tofReady = false;
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

bool timeReached(unsigned long now, unsigned long target) {
  return static_cast<long>(now - target) >= 0;
}

void enforceSafeOutputs() {
  // Phase 1 invariant: motor and heater are never energized.
  digitalWrite(PIN_MOTOR_DIR, LOW);  // Direction is not yet defined; level is harmless at PWM=0.
  analogWrite(PIN_MOTOR_PWM, 0);
  digitalWrite(PIN_SSR, LOW);
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

void initializeI2cDevices() {
  Wire.begin();
  Wire.setClock(400000UL);
  scanI2cBus();

  oledReady = i2cAddressDetected[OLED_I2C_ADDRESS] &&
              display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
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
  Serial.print(F("TOF_INIT="));
  Serial.println(tofReady ? F("OK") : F("ERR"));

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

void readTof() {
  ioData.tofValid = false;
  ioData.tofRangeStatus = 255;
  ioData.tofApiError = VL53L0X_ERROR_NONE;
  if (!tofReady) {
    return;
  }

  VL53L0X_RangingMeasurementData_t measurement = {};
  ioData.tofApiError = tof.rangingTest(&measurement, false);
  if (ioData.tofApiError != VL53L0X_ERROR_NONE) {
    return;  // Failed API calls must not expose stale or incomplete range data.
  }
  ioData.tofRangeStatus = measurement.RangeStatus;

  // Only status 0 is accepted as a valid range; failures have diagnostic labels.
  if (measurement.RangeStatus == 0 && measurement.RangeMilliMeter > 0) {
    ioData.tofDistanceMm = measurement.RangeMilliMeter;
    ioData.tofValid = true;
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
  display.print(F("mm"));

  display.setCursor(0, 20);
  display.print(F("Temp "));
  printValueOrError(ioData.temperatureActualC, ioData.pt100Valid, 1);
  if (ioData.pt100Valid) {
    display.print('C');
  }

  display.setCursor(0, 30);
  display.print(F("ToF  "));
  if (ioData.tofValid) {
    display.print(ioData.tofDistanceMm);
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
  display.print(F("SSR OFF  PWM 0"));  // Matches the enforced Phase 1 invariant.
  display.display();
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
  if (ioData.tofValid) {
    Serial.print(ioData.tofDistanceMm);
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
  Serial.println(F(",SSR=OFF,MOTOR_PWM=0"));
}

void setup() {
  // Set safe output values before changing pins to OUTPUT, minimizing startup transients.
  digitalWrite(PIN_MOTOR_DIR, LOW);
  digitalWrite(PIN_MOTOR_PWM, LOW);
  digitalWrite(PIN_SSR, LOW);
  digitalWrite(PIN_BUZZER, BUZZER_OFF_LEVEL);

  pinMode(PIN_START_SWITCH, INPUT_PULLUP);  // Internal pull-up; NO switch connects D4 to GND.
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_PWM, OUTPUT);
  pinMode(PIN_SSR, OUTPUT);
  enforceSafeOutputs();
  setBuzzerOutput(false);

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("DORAYAKI_IO_CHECK_START"));
  Serial.println(F("SAFETY: SSR=OFF, MOTOR_PWM=0"));

  switchLastRawPressed = (digitalRead(PIN_START_SWITCH) == LOW);
  switchStablePressed = switchLastRawPressed;
  ioData.startPressed = switchStablePressed;
  switchLastChangeMs = millis();

  initializeI2cDevices();

  readAnalogInputs();
  readPt100();
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

  // Reassert safety outputs on every loop. No sensor result can energize them.
  enforceSafeOutputs();
  readStartSwitch();
  updateBuzzer();

  if ((now - lastSensorMs) >= SENSOR_INTERVAL_MS) {
    lastSensorMs = now;
    readAnalogInputs();
    readPt100();
    readTof();
  }

  if ((now - lastDisplayMs) >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }

  if ((now - lastSerialMs) >= SERIAL_INTERVAL_MS) {
    lastSerialMs = now;
    updateSerial();
  }
}
