#!/usr/bin/env bash

# Builds and uploads the firmware to the microcontroller with ArduinoOTA
# Requires avahi-utils package `sudo apt install avahi-utils`

# Get the dynamic IP for the microcontroller
PORT=$(avahi-resolve-host-name -4 Headwind.local | awk '{print $2}')

# Build project files if the build flag is passed
if [ "$1" = "-b" ]; then
    pio run -v
fi

# Upload project to the ESP32 on the correct port
pio run -t upload --upload-port=$PORT -v
