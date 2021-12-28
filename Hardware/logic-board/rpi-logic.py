#!/usr/bin/env python3
import serial
import RPi.GPIO as GPIO

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(19, GPIO.OUT)

if __name__ == '__main__':
    GPIO.output(19, True)
    ser = serial.Serial('/dev/ttyACM0', 57600, timeout=0)
    ser.reset_input_buffer()
    while True:
        if ser.in_waiting > 0:
            line = ser.read(12).decode('utf-8').rstrip()
            print(line)

