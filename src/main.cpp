#include <Arduino.h>

#include <config.h>
#include <math.h>
#include <mic.h>

#include "OneButton.h"

// Конфигурация ADC
// #define MIC_PIN ADC1_CHANNEL_1

esp_adc_cal_characteristics_t adc_chars;
float silence_level_adc = 0;
OneButton onboard_btn(BTN0, false, false);



void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Измерение громкости с MAX9814 (смещение 1.25V) ===");
  
  setup_adc(&adc_chars);
  float measured_bias = calibrate_silence(silence_level_adc);
  
  // Проверяем, насколько измерение соответствует спецификации
  if (fabs(measured_bias - MAX9814_BIAS) > 0.2) {
    Serial.printf("⚠️  Внимание: большое расхождение! Проверьте питание.\n");
  }
  
  Serial.println("\nВремя\tСредн dB\tПик dB\tRMS mV\tОтклонение");
  Serial.println("------------------------------------------------");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
}

bool led_flag = false;

void loop() {
  unsigned long start_time = millis();
  unsigned long samples_collected = 0;
  float max_voltage = 0;
  float sum_voltage_squared = 0;
  
  // Сбор данных за 1 секунду
  while (millis() - start_time < 1000) {
    const int raw_value = adc1_get_raw(MIC_PIN);
    
    // Преобразуем в напряжение и вычитаем смещение 1.25V
    const float voltage = (raw_value / ADC_RESOLUTION) * REFERENCE_VOLTAGE;
    const float signal_voltage = voltage - MAX9814_BIAS;  // Используем правильное смещение!
    
    sum_voltage_squared += signal_voltage * signal_voltage;
    if (fabs(signal_voltage) > max_voltage) {
      max_voltage = fabs(signal_voltage);
    }
    
    samples_collected++;
    delayMicroseconds(1000000 / SAMPLE_RATE);
  }
  digitalWrite(LED_BUILTIN, led_flag);
  led_flag = !led_flag;
  
  // Вычисляем RMS напряжение сигнала
  float rms_voltage = sqrt(sum_voltage_squared / samples_collected);
  
  // Преобразуем в dB
  float avg_dB = voltage_to_dB(rms_voltage);
  float peak_dB = voltage_to_dB(max_voltage);
  
  // Выводим результаты в милливольтах для наглядности
  Serial.printf("%lu\t%.1f\t\t%.1f\t%.1f\t%+.1f\n", 
                millis()/1000, avg_dB, peak_dB, 
                rms_voltage * 1000,  // RMS в mV
                (rms_voltage * 1000) - 12.5);  // Отклонение от 12.5mV
  
  // Перекалибровка по команде
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'c' || cmd == 'C') {
      calibrate_silence(silence_level_adc);
      Serial.println("Время\tСредн dB\tПик dB\tRMS mV\tОтклонение");
      Serial.println("------------------------------------------------");
    }
  }
}