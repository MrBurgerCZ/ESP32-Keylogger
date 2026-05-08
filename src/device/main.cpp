#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

USBHIDKeyboard Keyboard;

// UART pins for XIAO ESP32-S3 (D6=TX, D7=RX)
#define TX_PIN 44 
#define RX_PIN 43
#define BUTTON_PIN 0

void setup() {
  Serial.begin(115200);
  
  // uart mezi deskama
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Keyboard.begin();
  USB.begin();
  
  Serial.println("ready");
}

void loop() {
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();

    if (message.startsWith("MOD:")) {
      uint8_t modifier = 0;
      uint8_t keys[6] = {0, 0, 0, 0, 0, 0};

      // parsing MOD:X,KEYS:X.X.X.X.X.X
      int parsed = sscanf(message.c_str(), "MOD:%hhX,KEYS:%hhX.%hhX.%hhX.%hhX.%hhX.%hhX", 
                          &modifier, &keys[0], &keys[1], &keys[2], &keys[3], &keys[4], &keys[5]);

      if (parsed == 7) {
        KeyReport myReport;

        myReport.modifiers = modifier;
        myReport.reserved = 0; // tohle USB protokol vyzaduje mit na nule
        myReport.keys[0] = keys[0];
        myReport.keys[1] = keys[1];
        myReport.keys[2] = keys[2];
        myReport.keys[3] = keys[3];
        myReport.keys[4] = keys[4];
        myReport.keys[5] = keys[5];

        // posleme UKAZATEL na tu strukturu (pomoci operatoru &)
        Keyboard.sendReport(&myReport);
      }
    }
    Serial.println(message);
  }
}