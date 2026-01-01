//
// Created by iv on 12/30/25.
//

#ifndef MAX981_VOLUME_METER_MIC_H
#define MAX981_VOLUME_METER_MIC_H

#include <driver/adc.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include <config.h>

typedef struct {
    float avg_dB;
    float peak_dB;
    float rms_voltage;
} MIC_SIGNAL;

float calibrate_silence(float &silence_level);
float voltage_to_dB(float voltage_rms);

void setup_adc(adc_cali_handle_t *handle);

#endif //MAX981_VOLUME_METER_MIC_H