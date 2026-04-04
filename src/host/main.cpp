#include <Arduino.h>

// UART piny pro XIAO ESP32-S3 (D6=TX, D7=RX)
#define TX_PIN 44 
#define RX_PIN 43

void setup() {
  Serial.begin(115200);
  
  // uart mezi deskama
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("ready");
}

void loop() {
  // prijem dat
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();

    if (message == "1") {
      Serial.printf("[%lu] 1\n", millis());
      digitalWrite(LED_BUILTIN, LOW);
      
    }
    else if (message == "0") {
      Serial.printf("[%lu] 0\n", millis());
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}