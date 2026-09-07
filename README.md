# JetSurf Wireless BLE Controller Application

## How to build:
idf required: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html

run `idf.py build` in the root dir

## How to flash:
run `idf.py flash` or `idf.py -p PORT flash` in the root dir

## Flash and monitor output
run `idf.py -p PORT flash monitor`
for example: `idf.py -p /dev/ttyUSB0 flash monitor`

## WSL <-> Windows usb port bind command
* In ps: `usbipd list`
* In ps: `usbipd bind --busid 1-6`
* In ps: `usbipd attach --wsl --busid 1-6`
