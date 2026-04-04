#include <Arduino.h>

// UART piny pro XIAO ESP32-S3 (D6=TX, D7=RX)
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
  
  Serial.println("ready");
}

bool lastButtonState = HIGH; // tlacitko nestisknuto

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial1.println("1"); 
    Serial.printf("[%lu] button pressed -> sending 1\n", millis());
    
    delay(50);
  }

  if (currentButtonState == HIGH && lastButtonState == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    
    Serial1.println("0");
    Serial.printf("[%lu] button released -> sending 0\n", millis());
    
    delay(50);
  }

  lastButtonState = currentButtonState;
}