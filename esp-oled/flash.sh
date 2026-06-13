#!/bin/bash

DEVICE=$(ls /dev/tty* | grep -E "(ACM*)|(USB*)")

pio run -e esp32dev -t upload --upload-port $DEVICE
