#!/usr/bin/env python3
import serial

if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 57600, timeout=0)
    ser.reset_input_buffer()
    while True:
        if ser.in_waiting > 0:
            line = ser.read(13).decode('utf-8').rstrip()
            print(line)

