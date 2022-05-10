#!/usr/bin/env python3
import serial
import pygame
from time import sleep

dict = {
    '100000000000': 'piano2/C5.mp3',
    '010000000000': 'piano2/Db5.mp3',
    '001000000000': 'piano2/D5.mp3',
    '000100000000': 'piano2/Eb5.mp3',
    '000010000000': 'piano2/E5.mp3',
    '000001000000': 'piano2/F5.mp3',
    '000000100000': 'piano2/Gb5.mp3',
    '000000010000': 'piano2/G5.mp3',
    '000000001000': 'piano2/Ab5.mp3',
    '000000000100': 'piano2/A5.mp3',
    '000000000010': 'piano2/Bb5.mp3',
    '000000000001': 'piano2/B5.mp3',
}

zero_char = '0'
one_char = '1'

def play_note(file_name):
    print('init =', pygame.mixer.get_init())
    print('channels =', pygame.mixer.get_num_channels())
    pygame.mixer.music.load(file_name)
    pygame.mixer.music.play()
    print(file_name)

def convert_binary_to_piano_note(line):
    while line.count('1') > 0:
        file = dict[line]
        play_note(file)
        line = line.replace(one_char, zero_char, 1)

if __name__ == '__main__':
    pygame.mixer.init()
    ser = serial.Serial('/dev/ttyACM0', 57600, timeout=0)
    ser.reset_input_buffer()
    prev_line = ''
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').rstrip()
            if line != prev_line:
                print('line: ' + line)
                convert_binary_to_piano_note(line)
            prev_line = line

