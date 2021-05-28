#!/usr/bin/python3

import serial  # pip3 install pyserial
import sys
import time
import datetime

# Arduino serial dev paramaters
DEVICE = '/dev/ttyUSB0' # Change this as necessary
BAUD = 115200

# Open serial port
try:
    ser = serial.Serial(port=DEVICE, baudrate=BAUD, timeout=1, writeTimeout=1)
except:
    print('Cannot open serial port')
    sys.exit(0)

time.sleep(3)  # not ideal but works ;)
ser.flushInput()  # ignore bootstrap messages
ts = datetime.datetime.now().second + (datetime.datetime.now().microsecond / 1000000.0)
# Adjust by 'some' seconds!!! Make me variable!
ts = ts + 0.55  # add delay compensation
print("[+] Delay compensation added!")
# ts = "T%s" % int(round(ts))  # 0.6 -> 1 promotion can be too much?
ts = "T%s" % int(round(ts))
print(ts)
ser.write(ts.encode("ascii"))

while True:
    print(ser.read())
