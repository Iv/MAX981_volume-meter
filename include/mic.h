//
// Created by iv on 12/30/25.
//

#ifndef MAX981_VOLUME_METER_MIC_H
#define MAX981_VOLUME_METER_MIC_H

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <config.h>

esp_adc_cal_characteristics_t adc_chars;

float calibrate_silence(float &silence_level);
float voltage_to_dB(float voltage_rms);

void setup_adc(esp_adc_cal_characteristics_t *chars);

#endif //MAX981_VOLUME_METER_MIC_H