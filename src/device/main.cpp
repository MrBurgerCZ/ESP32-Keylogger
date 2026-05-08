#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBMSC.h"
#include "FFat.h"
#include "esp_partition.h"

USBHIDKeyboard Keyboard;
USBMSC msc;

#define TX_PIN 44 
#define RX_PIN 43
#define BUTTON_PIN 0

bool keyboardConnected = false;
bool mscActive = false;
unsigned long startTime = 0;
const unsigned long WINDOW_TIME = 5000;
const esp_partition_t* partition = NULL;

// nizkourovnove callbacky pro mass Storage
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (partition == NULL) return -1;
  esp_partition_read(partition, lba * 512 + offset, buffer, bufsize);
  return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (partition == NULL) return -1;
  esp_partition_erase_range(partition, lba * 512 + offset, bufsize);
  esp_partition_write(partition, lba * 512 + offset, buffer, bufsize);
  return bufsize;
}

void activateMSC() {
  if (partition == NULL) return;
  msc.vendorID("XIAO");
  msc.productID("LOGGER");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.mediaPresent(true);
  msc.begin(partition->size / 512, 512);
  mscActive = true;
  Serial.println("MSC: AKTIVOVANO");
}

void setup() {
  Serial.begin(115200);

  // uart mezi deskama
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // priprava partition pro disk
  partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");

  USB.VID(0x04D9);
  USB.PID(0x1702);
  USB.productName("HID Keyboard Device");
  USB.manufacturerName("Standard");

  Keyboard.begin();
  USB.begin();
  
  startTime = millis();
}

void loop() {
  // aktivace mass storage kdyz zmacknu tlacitko v prvnich 5 sekundach po startu a zadna klavesnice neni pripojena
  if (!mscActive && (millis() - startTime < WINDOW_TIME)) {
    // Blikání pro indikaci možnosti vstupu do údržby
    digitalWrite(LED_BUILTIN, (millis() % 200 < 100) ? LOW : HIGH);

    if (digitalRead(BUTTON_PIN) == LOW && !keyboardConnected) {
      activateMSC();
      digitalWrite(LED_BUILTIN, LOW); 
    }
  } else if (!mscActive) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();

    // detekce pripojeni klavesnice
    if (message.startsWith("STATUS:")) {
      if (message.endsWith("CONNECTED")) {
        keyboardConnected = true;
      } else if (message.endsWith("DISCONNECTED")) {
        keyboardConnected = false;
      }
    }

    if (message.startsWith("MOD:")) {
      uint8_t modifier = 0;
      uint8_t keys[6] = {0};
      int parsed = sscanf(message.c_str(), "MOD:%hhX,KEYS:%hhX.%hhX.%hhX.%hhX.%hhX.%hhX", 
                          &modifier, &keys[0], &keys[1], &keys[2], &keys[3], &keys[4], &keys[5]);

      if (parsed == 7) {
        KeyReport myReport = {modifier, 0, {keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]}};
        Keyboard.sendReport(&myReport);
      }
    }
    Serial.println(message);
  }
}