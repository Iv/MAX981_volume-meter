#include <mic.h>

void setup_adc(esp_adc_cal_characteristics_t *chars) {
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(MIC_PIN, ADC_ATTEN);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, chars);
}

float calibrate_silence(float &silence_level) {
    Serial.println("КАЛИБРОВКА: сохраняйте полную тишину 3 секунды...");

    long sum = 0;
    int samples = 3000;

    for (int i = 0; i < samples; i++) {
        sum += adc1_get_raw(MIC_PIN);
        if (i % 1000 == 0) Serial.print(".");
        delay(1);
    }

    silence_level = sum / static_cast<float>(samples);
    float measured_voltage = (silence_level / ADC_RESOLUTION) * REFERENCE_VOLTAGE;

    Serial.printf("\nКалибровка завершена:\n");
    Serial.printf(" - ADC значение тишины: %.0f/4095\n", silence_level);
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