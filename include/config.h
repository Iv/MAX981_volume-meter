//
// Created by iv on 12/29/25.
//

#ifndef ESP32_MIC_RELAY_CONFIG_H
#define ESP32_MIC_RELAY_CONFIG_H

#include <Arduino.h>


constexpr uint8_t BTN0 = 0;

constexpr uint8_t RELAY1 = 16;
constexpr uint8_t RELAY2 = 17;

constexpr uint8_t LED = 23;

#undef LED_BUILTIN
#define LED_BUILTIN LED

// Конфигурация ADC
// #define MIC_PIN ADC1_CHANNEL_1  // Esp32 S3 version.
#define MIC_PIN ADC1_CHANNEL_4


#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_12
#define SAMPLE_RATE 8000
#define REFERENCE_VOLTAGE 3.1f
#define ADC_RESOLUTION 4095.0f

// Параметры MAX9814
#define MAX9814_BIAS 1.25f  // Смещение 1.25V вместо 1.65V!
#define SENSITIVITY_MV_PER_PA 12.5f  // 12.5 mV/Pa для усиления 60dB


#endif //ESP32_MIC_RELAY_CONFIG_H