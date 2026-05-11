#include <Arduino.h>
#include "EspUsbHost.h"

#define TX_PIN 44 
#define RX_PIN 43

class MyEspUsbHost : public EspUsbHost {
  void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report) {
    
    // new keypress?
    if (memcmp(&report, &last_report, sizeof(hid_keyboard_report_t)) == 0) return;

    Serial1.print("MOD:");
    Serial1.print(report.modifier, HEX);
    
    Serial1.print(",KEYS:");
    for (int i = 0; i < 6; i++) {
      Serial1.print(report.keycode[i], HEX);
      if (i < 5) Serial1.print(".");
    }
    Serial1.println();
  }
};

MyEspUsbHost usbHost;
bool wasConnected = false;

void setup() {
  Serial.begin(115200);
  
  // uart to the other board
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  usbHost.begin();
  usbHost.interval = 1;

  Serial.println("ready");
}

void loop() {
  usbHost.task();
  
  if (usbHost.isReady && !wasConnected) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial1.println("STATUS:CONNECTED");
    wasConnected = true;
  }
  if (!usbHost.isReady && wasConnected) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial1.println("STATUS:DISCONNECTED");
    Serial1.println("MOD:0,KEYS:0.0.0.0.0.0");
    wasConnected = false;
  }
}