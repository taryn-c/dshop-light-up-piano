#!/usr/bin/env python3
import serial
import pygame
import threading
import socketio
from time import sleep
from collections import Counter

sio = socketio.Client()

# dashed notes are sharps 
dict = {
    '100000000000': 'piano/c5.ogg',
    '010000000000': 'piano/c-5.ogg',
    '001000000000': 'piano/d5.ogg',
    '000100000000': 'piano/d-5.ogg',
    '000010000000': 'piano/e5.ogg',
    '000001000000': 'piano/f5.ogg',
    '000000100000': 'piano/f-5.ogg',
    '000000010000': 'piano/g5.ogg',
    '000000001000': 'piano/g-5.ogg',
    '000000000100': 'piano/a5.ogg',
    '000000000010': 'piano/a-5.ogg',
    '000000000001': 'piano/b5.ogg',
}

zero_char = '0'
one_char = '1'

def extract_note(multiple_notes):
    no_sound_line = '000000000000'
    first_index = multiple_notes.index(one_char)
    no_sound_line = no_sound_line[:first_index] + one_char + no_sound_line[first_index+1:]
    return no_sound_line
    
def play_note(file_name):
    # print('init =', pygame.mixer.get_init())
    # print('channels =', pygame.mixer.get_num_channels())
    # play mp3 files
    # pygame.mixer.music.load(file_name)
    # pygame.mixer.music.play()
    snd = pygame.mixer.Sound(file_name)
    pygame.mixer.find_channel(True).play(snd)
    snd.set_volume(0.1)
    print(file_name)

def convert_binary_to_piano_note(line):
    line_copy = line[:]
    while line_copy.count('1') > 0:
        file = dict[extract_note(line_copy)]
        play_note(file)
        # t1 = threading.Thread(target=play_note, args=(file,))
        # t1.start()
        # t1.join()
        line_copy = line_copy.replace(one_char, zero_char, 1)

if __name__ == '__main__':
    pygame.mixer.init()
    sio.connect('http://localhost:3000')
    ser = serial.Serial('/dev/ttyACM0', 57600, timeout=0)
    ser.reset_input_buffer()
    prev_line = ''
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').rstrip()
            line_counter = Counter(line)
            prev_line_counter = Counter(prev_line)
            if line != prev_line:
                print('line1: ' + line)
                sio.emit('pythonToServer', {'line': line})
                # print('line_counter: ' + str(line_counter))
                convert_binary_to_piano_note(line)
            prev_line = line

# 00101
# 00100
