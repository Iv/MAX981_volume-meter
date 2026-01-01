#include <Arduino.h>

#include <math.h>
#include "OneButton.h"
#include "Ticker.h"
#include <Preferences.h>

#include <config.h>
#include <mic.h>

#include <GyverNTP.h>
#include <WiFiManager.h>

esp_adc_cal_characteristics_t adc_chars;
float silence_level_adc = 0;
bool led_flag = false;
OneButton onboard_btn(BTN0, true, true);

constexpr uint16_t config_portal_timeout = 3000;
constexpr uint16_t btn0_long_press_timeout = 3000;

Preferences preferences;
WiFiManager wm;


uint32_t long_press_start = 0;
bool during_long_press = false;

volatile MIC_SIGNAL mic_signal;


void btn0_click() {
  Serial.println("btn0_click");
}

void btn0_pressed() {
  during_long_press = true;
  long_press_start = millis();
  digitalWrite(LED_BUILTIN, LOW);

  wm.resetSettings();
}

void btn0_during_long_press() {
  if (millis() - long_press_start > btn0_long_press_timeout) {
    digitalWrite(LED_BUILTIN, HIGH);

  }
}

void btn0_long_press_release() {
  long_press_start = 0;
  during_long_press = false;
}

void print_val() {
  Serial.printf("%lu\t%.1f\t\t%.1f\t%.1f\t%+.1f\n",
              millis()/1000,
              mic_signal.avg_dB,
              mic_signal.avg_dB,
              mic_signal.rms_voltage*1000,  // RMS в mV
              (mic_signal.rms_voltage * 1000) - 12.5);  // Отклонение от 12.5mV

  led_flag = !led_flag;
  if (!during_long_press)  digitalWrite(LED_BUILTIN, led_flag);
}

Ticker print_ticker(print_val, 1000);

void codeForCore1Task(void *parameter) {

  for (;;) {
    unsigned long start_time = millis();
    unsigned long samples_collected = 0;
    float max_voltage = 0;
    float sum_voltage_squared = 0;

    // Сбор данных за 1 секунду
    while (millis() - start_time < 1000) {
      const int raw_value = adc1_get_raw(MIC_PIN);

      // Преобразуем в напряжение и вычитаем смещение 1.25V
      const float voltage = (static_cast<float>(raw_value) / ADC_RESOLUTION) * REFERENCE_VOLTAGE;
      const float signal_voltage = voltage - MAX9814_BIAS;  // Используем правильное смещение!

      sum_voltage_squared += signal_voltage * signal_voltage;
      if (fabs(signal_voltage) > max_voltage) {
        max_voltage = fabs(signal_voltage);
      }

      samples_collected++;
      delayMicroseconds(1000000 / SAMPLE_RATE);
    }

    // Вычисляем RMS напряжение сигнала
    mic_signal.rms_voltage = sqrt(sum_voltage_squared / static_cast<float>(samples_collected));
    // Преобразуем в dB
    mic_signal.avg_dB = voltage_to_dB(mic_signal.rms_voltage);
    mic_signal.peak_dB = voltage_to_dB(max_voltage);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res = false;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("Mic_Relay"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }



  Serial.println("\n=== Измерение громкости с MAX9814 (смещение 1.25V) ===");

  // preferences.begin("mic");

  setup_adc(&adc_chars);
  const float measured_bias = calibrate_silence(silence_level_adc);
  
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

  onboard_btn.setPressMs(500);
  onboard_btn.attachClick(btn0_click);
  onboard_btn.attachLongPressStart(btn0_pressed);

  print_ticker.start();

  xTaskCreatePinnedToCore(
    codeForCore1Task, // Function to implement the task
    "Core1Task",      // Name of the task (for debugging)
    10000,            // Stack size in words
    NULL,             // Task input parameter (not used here)
    1,                // Priority of the task (0 is lowest)
    NULL,             // Task handle (not used here)
    1                 // Core where the task should run (Core 1)
  );

  // обработчик ошибок
  NTP.onError([]() {
      Serial.println(NTP.readError());
      Serial.print("online: ");
      Serial.println(NTP.online());
  });

  // // обработчик секунды (вызывается из тикера)
  // NTP.onSecond([]() {
  //     Serial.println("new second!");
  // });

  // обработчик синхронизации (вызывается из sync)
  // NTP.onSync([](uint32_t unix) {
  //     Serial.println("sync: ");
  //     Serial.print(unix);
  // });

  NTP.begin(3);                           // запустить и указать часовой пояс
  // NTP.setPeriod(30);                   // период синхронизации в секундах
  // NTP.setHost("ntp1.stratum2.ru");     // установить другой хост
  // NTP.setHost(IPAddress(1, 2, 3, 4));  // установить другой хост
  NTP.asyncMode(false);                // выключить асинхронный режим
  // NTP.ignorePing(true);                // не учитывать пинг до сервера
  // NTP.updateNow();                     // обновить прямо сейчас
}


void loop() {
  // Перекалибровка по команде
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'c' || cmd == 'C') {
      const float measured_bias = calibrate_silence(silence_level_adc);

      // Проверяем, насколько измерение соответствует спецификации
      if (fabs(measured_bias - MAX9814_BIAS) > 0.2) {
        Serial.printf("⚠️  Внимание: большое расхождение! Проверьте питание.\n");
      }

      Serial.println("Время\tСредн dB\tПик dB\tRMS mV\tОтклонение");
      Serial.println("------------------------------------------------");
    }
  }

  onboard_btn.tick();
  print_ticker.update();


  // if (NTP.tick()) {
  //   // вывод даты и времени строкой
  //   Serial.print(NTP.toString());  // NTP.timeToString(), NTP.dateToString()
  //   Serial.print(':');
  //   Serial.println(NTP.ms());  // + миллисекунды текущей секунды. Внутри tick всегда равно 0
  // }

  // if (NTP.newSecond()) {
  //   // новую секунду можно поймать и здесь
  // }

  // изменился онлайн-статус
  if (NTP.statusChanged()) {
    Serial.print("STATUS: ");
    Serial.println(NTP.online());
  }
}