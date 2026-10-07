#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin assignment (Arduino Mega 2560 Rev3)
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_POT_TEMP = A0;
constexpr uint8_t PIN_POT_PRESS_TIME = A1;
constexpr uint8_t PIN_POT_HEIGHT = A2;
constexpr uint8_t PIN_PT100 = A7;

constexpr uint8_t PIN_START_SWITCH = 4;  // External pull-up: released=HIGH, pressed=LOW
constexpr uint8_t PIN_BUZZER = 5;
constexpr uint8_t PIN_MOTOR_DIR = 8;
constexpr uint8_t PIN_MOTOR_PWM = 9;
constexpr uint8_t PIN_SSR = 12;

// Mega 2560 hardware I2C pins are fixed at SDA=D20 and SCL=D21.
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t TOF_I2C_ADDRESS = 0x29;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr int8_t OLED_RESET_PIN = -1;

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
constexpr unsigned long SENSOR_INTERVAL_MS = 100;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250;
constexpr unsigned long SERIAL_INTERVAL_MS = 1000;
constexpr unsigned long DISPLAY_PAGE_INTERVAL_MS = 2500;
constexpr unsigned long SWITCH_DEBOUNCE_MS = 30;
constexpr unsigned long START_BEEP_MS = 100;

// -----------------------------------------------------------------------------
// Analog setting ranges and smoothing
// -----------------------------------------------------------------------------
constexpr float TEMP_SET_MIN_C = 150.0F;
constexpr float TEMP_SET_MAX_C = 300.0F;
constexpr float PRESS_TIME_MIN_S = 1.0F;
constexpr float PRESS_TIME_MAX_S = 10.0F;
constexpr float HEIGHT_SET_MIN_MM = 0.0F;
constexpr float HEIGHT_SET_MAX_MM = 300.0F;

// Exponential moving average: larger value reacts faster, smaller value is smoother.
constexpr float ANALOG_FILTER_ALPHA = 0.20F;

// -----------------------------------------------------------------------------
// PT100 conversion/calibration
// Circuit: 5V -- series resistor -- A7 -- PT100 -- GND
// Rpt100 = series_resistor * ADC / (1023 - ADC)
// -----------------------------------------------------------------------------
constexpr float PT100_SERIES_RESISTOR_OHM = 1000.0F; // Actual divider resistor: 1 kOhm
constexpr float PT100_R0_OHM = 100.0F;
constexpr float PT100_CVD_A = 3.9083e-3F;
constexpr float PT100_CVD_B = -5.775e-7F;
constexpr float PT100_CAL_GAIN = 1.0F;      // Future span calibration
constexpr float PT100_CAL_OFFSET_C = 0.0F; // Future zero calibration
constexpr float PT100_MIN_C = 0.0F;
constexpr float PT100_MAX_C = 850.0F;

// Guard bands keep the divider calculation away from zero and full-scale faults.
constexpr uint16_t PT100_ADC_MIN_VALID = 1;
constexpr uint16_t PT100_ADC_MAX_VALID = 1022;

// -----------------------------------------------------------------------------
// Buzzer behavior
// Current implementation assumes an active buzzer: HIGH=ON, LOW=OFF.
// Change setBuzzerOutput() if a passive or active-low buzzer is selected later.
// -----------------------------------------------------------------------------
constexpr uint8_t BUZZER_ON_LEVEL = HIGH;
constexpr uint8_t BUZZER_OFF_LEVEL = LOW;
