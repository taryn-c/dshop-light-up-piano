#!/usr/bin/env python3
import serial
import pygame
from time import sleep

dict = {
    '100000000000': 'piano/c4.ogg',
    '010000000000': 'piano/d4.ogg',
    '001000000000': 'piano/e4.ogg',
    '000100000000': 'piano/f4.ogg',
    '000010000000': 'piano/g4.ogg',
    '000001000000': 'piano/a5.ogg',
    '000000100000': 'piano/b5.ogg',
    '000000010000': 'piano/c5.ogg',
    '000000001000': 'piano/d5.ogg',
    '000000000100': 'piano/e5.ogg',
    '000000000010': 'piano/f5.ogg',
    '000000000001': 'piano/g5.ogg',
}

zero_char = '0'
one_char = '1'

def play_note(file_name):
    print('init =', pygame.mixer.get_init())
    print('channels =', pygame.mixer.get_num_channels())
    snd = pygame.mixer.Sound(file_name)
    snd.play()
    print('length =', snd.get_length())

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

