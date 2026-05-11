
# ESP32 Keylogger

> [!NOTE]
> Work in progress, see ***[goals](#goals)***

A simple hardware keylogger written in C++

You need ***two*** `ESP32-S3`s, an `USB OTG` adapter, and thats it!

I am using the Seeed Studio XIAO ESP32-S3 for it's size and low price.

![The final product](assets/keylogger.jpg)

## Goals
- [x] read keys from the keyboard
- [x] send keypresses over uart to the other board
- [x] emulate a keyboard with the second board with usb hid
- [x] persistent storage
- [x] save keypresses into a logfile
- [x] usb mass storage
- [x] parser.py
- [ ] config file **\***
- [ ] rtos **\***

**\*cancelled** *(for now)*
