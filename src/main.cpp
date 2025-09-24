#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <math.h>

// Конфигурация ADC
#define MIC_PIN ADC1_CHANNEL_1  // GPIO2 соответствует ADC1_CHANNEL_1

#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_11
#define SAMPLE_RATE 8000
#define REFERENCE_VOLTAGE 3.1
#define ADC_RESOLUTION 4095.0

// Параметры MAX9814
#define MAX9814_BIAS 1.25  // Смещение 1.25V вместо 1.65V!
#define SENSITIVITY_MV_PER_PA 12.5  // 12.5 mV/Pa для усиления 60dB

esp_adc_cal_characteristics_t adc_chars;
float silence_level_adc = 0;

void setup_adc() {
  adc1_config_width(ADC_WIDTH);
  adc1_config_channel_atten(MIC_PIN, ADC_ATTEN);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, &adc_chars);
}

float calibrate_silence() {
  Serial.println("КАЛИБРОВКА: сохраняйте полную тишину 3 секунды...");
  
  long sum = 0;
  int samples = 3000;
  
  for (int i = 0; i < samples; i++) {
    sum += adc1_get_raw(MIC_PIN);
    if (i % 1000 == 0) Serial.print(".");
    delay(1);
  }
  
  silence_level_adc = sum / (float)samples;
  float measured_voltage = (silence_level_adc / ADC_RESOLUTION) * REFERENCE_VOLTAGE;
  
  Serial.printf("\nКалибровка завершена:\n");
  Serial.printf(" - ADC значение тишины: %.0f/4095\n", silence_level_adc);
  Serial.printf(" - Измеренное напряжение: %.3f V\n", measured_voltage);
  Serial.printf(" - Ожидаемое смещение MAX9814: %.3f V\n", MAX9814_BIAS);
  Serial.printf(" - Расхождение: %.3f V\n", measured_voltage - MAX9814_BIAS);
  
  return measured_voltage;
}

float voltage_to_dB(float voltage_rms) {
  if (voltage_rms < 0.00001) return 30.0;
  
  // Формула для преобразования напряжения в dB SPL
  // 94 dB SPL соответствует 1 Паскалю (Pa)
  // MAX9814: 12.5 mV/Pa при усилении 60dB
  float voltage_reference = SENSITIVITY_MV_PER_PA / 1000.0;  // 0.0125 V
  
  float dB_SPL = 20.0 * log10(voltage_rms / voltage_reference) + 35.13;
  
  // Реалистичные пределы для помещений
  return constrain(dB_SPL, 30.0, 120.0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Измерение громкости с MAX9814 (смещение 1.25V) ===");
  
  setup_adc();
  float measured_bias = calibrate_silence();
  
  // Проверяем, насколько измерение соответствует спецификации
  if (fabs(measured_bias - MAX9814_BIAS) > 0.2) {
    Serial.printf("⚠️  Внимание: большое расхождение! Проверьте питание.\n");
  }
  
  Serial.println("\nВремя\tСредн dB\tПик dB\tRMS mV\tОтклонение");
  Serial.println("------------------------------------------------");
}

void loop() {
  unsigned long start_time = millis();
  unsigned long samples_collected = 0;
  float max_voltage = 0;
  float sum_voltage_squared = 0;
  
  // Сбор данных за 1 секунду
  while (millis() - start_time < 1000) {
    int raw_value = adc1_get_raw(MIC_PIN);
    
    // Преобразуем в напряжение и вычитаем смещение 1.25V
    float voltage = (raw_value / ADC_RESOLUTION) * REFERENCE_VOLTAGE;
    float signal_voltage = voltage - MAX9814_BIAS;  // Используем правильное смещение!
    
    sum_voltage_squared += signal_voltage * signal_voltage;
    if (fabs(signal_voltage) > max_voltage) {
      max_voltage = fabs(signal_voltage);
    }
    
    samples_collected++;
    delayMicroseconds(1000000 / SAMPLE_RATE);
  }
  
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
      calibrate_silence();
      Serial.println("Время\tСредн dB\tПик dB\tRMS mV\tОтклонение");
      Serial.println("------------------------------------------------");
    }
  }
}