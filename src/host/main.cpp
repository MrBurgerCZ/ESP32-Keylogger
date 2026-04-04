#include <Arduino.h>
#include "EspUsbHost.h"

// UART piny pro XIAO ESP32-S3 (D6=TX, D7=RX)
#define TX_PIN 44 
#define RX_PIN 43

class MyEspUsbHost : public EspUsbHost {
  void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report) {
    
    // novy keypress?
    if (memcmp(&report, &last_report, sizeof(hid_keyboard_report_t)) == 0) return;

    Serial1.print("MOD:");
    Serial1.print(report.modifier, BIN);
    
    Serial1.print(",KEYS:");
    for (int i = 0; i < 6; i++) {
      Serial1.print(report.keycode[i], HEX);
      if (i < 5) Serial1.print(".");
    }
    Serial1.println();
  }
};

MyEspUsbHost usbHost;

void setup() {
  Serial.begin(115200);
  
  // uart mezi deskama
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  usbHost.begin();
  usbHost.interval = 1;

  Serial.println("ready");
}

void loop() {
  usbHost.task();
}