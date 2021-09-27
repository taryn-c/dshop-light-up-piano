/*******************************************************************************

 Bare Conductive Multi Board MIDI Piano
 --------------------------------------
 
 primary_midi_board.ino - touch triggered MIDI synth using multiple Touch Boards

 Maximum total number of boards is 7 (one primary board and six 
 secondary boards). All boards must share a common ground connection,
 with TX on all the secondary boards commoned together and connected to RX on
 the primary board. The first secondary board must have a connection between 
 its A0 and A0 on the primary board. The second secondary board must have a 
 connection between its A0 and A1 on the primary board, and so on.

 The primary board must also have the two MIDI enabling solder blobs made, 
 either using solder or Bare Conductive Electric Paint. These are located 
 just below digital pin 10 and just above the 2x3 ICSP header land pattern
 on the right-hand side of the board.

 Each board must also be powered, although up to 4 boards can share power by
 commoning up the 5V connection between them.
 
 Based on code by Jim Lindblom and plenty of inspiration from the Freescale 
 Semiconductor datasheets and application notes.
 
 Bare Conductive code written by Stefan Dzisiewski-Smith.
 
 This work is licensed under a MIT license https://opensource.org/licenses/MIT
 
 Copyright (c) 2016, Bare Conductive
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*******************************************************************************/

// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

#include <SoftwareSerial.h>

// LED strip includes
#include <Adafruit_NeoPixel.h>

SoftwareSerial mySerial(12, 10); // Soft TX on 10, we don't use RX in this code

// number of boards config
// you can reduce this to improve response time, but the code will work fine with it 
// left at 6 - do not try to increase this beyond 6!
// don't forget to edit the notes variable below if you change this
const uint8_t numSecondaryBoards = 1;
const uint8_t totalNumElectrodes = (numSecondaryBoards+1)*12;

// serial comms
const uint8_t serialPacketSize = 13;
uint8_t incomingPacket[serialPacketSize];

// secondary board touch variables
bool thisExternalTouchStatus[numSecondaryBoards][12];
bool lastExternalTouchStatus[numSecondaryBoards][12];

// compound touch variables
bool touchStatusChanged = false;
bool isNewTouch[totalNumElectrodes];
bool isNewRelease[totalNumElectrodes];

// MPR121 datastream behaviour constants
const bool MPR121_DATASTREAM_ENABLE = false;

// MIDI config
uint8_t note = 0; // The MIDI note value to be played
const uint8_t resetMIDI = 8; // Tied to VS1053 Reset line
uint8_t instrument = 0;

// these are the MIDI notes you want to map to each electrode
// starting with E0 thru E11 on the primary board
// then E0 thru E11 on the first secondary board and so on
// note - the number of note values below MUST equal the total number of electrodes
const uint8_t notes[totalNumElectrodes] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};

// LED pins
// maps electrode 0 to digital 0, electrode 2 to digital 1, electrode 3 to digital 10 and so on...
// A0..A5 are the analogue input pins, used as digital outputs in this example
const int ledPins[24] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 13
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 24
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup(){  
  // setup for one NeoPixel attached to pin 13
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
   // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.

  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) {}; //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Multi Board MIDI Piano");

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  // Reset the VS1053
  pinMode(resetMIDI, OUTPUT);
  digitalWrite(resetMIDI, LOW);
  delay(100);
  digitalWrite(resetMIDI, HIGH);
  delay(100);

  mySerial.begin(31250);
  
  // initialise MIDI
  setupMidi();

  for(int i=0; i<numSecondaryBoards; i++){
    for(int j=0; j<12; j++){
      thisExternalTouchStatus[i][j] = false;
      lastExternalTouchStatus[i][j] = false;
    }
  }

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }   

  for(int a=A0; a<=A5; a++){
    pinMode(a, OUTPUT);
    digitalWrite(a, LOW); 
  }

  Serial1.begin(57600);
  Serial1.setTimeout(3); // 3ms is more than enough time for a packet to be transferred (should take 2.26ms)
  delay(100);
}

void loop(){
  
  // reset everything that we combine from the two boards
  resetCompoundVariables();
  
  readLocalTouchInputs();
  
  readRemoteTouchInputs();
  
  processTouchInputs();
  
}


void readLocalTouchInputs(){

  // update our compound data on the local touch status

  if(MPR121.touchStatusChanged()){
    MPR121.updateTouchData();
    touchStatusChanged = true;
    
    for(int i=0; i<12; i++){
      isNewTouch[i] = MPR121.isNewTouch(i);
      isNewRelease[i] = MPR121.isNewRelease(i);
    }
  }

}

void readRemoteTouchInputs(){

  uint8_t numBytesRead;

  for(int a=A0; a<A0+numSecondaryBoards; a++){

    digitalWrite(a, HIGH);

    // try to read a full packet
    numBytesRead = Serial1.readBytesUntil(0x00, incomingPacket, serialPacketSize);

    // only process a complete packet
    if(numBytesRead==serialPacketSize){
      // save last status to detect touch / release edges
      for(int i=0; i<12; i++){
        lastExternalTouchStatus[a-A0][i] = thisExternalTouchStatus[a-A0][i];
      }
      
      if(incomingPacket[0] == 'T'){ // ensure we are synced with the packet 'header'
        for(int i=0; i<12; i++){
          if(incomingPacket[i+1]=='1'){
            thisExternalTouchStatus[a-A0][i] = true;
          } else {
            thisExternalTouchStatus[a-A0][i] = false;
          }
        }
      } 

      // now that we have read the remote touch data, merge it with the local data
      for(int i=0; i<12; i++){
        if(lastExternalTouchStatus[a-A0][i] != thisExternalTouchStatus[a-A0][i]){
          touchStatusChanged = true;
          if(thisExternalTouchStatus[a-A0][i]){
            // shift remote data up the array by 12 so as not to overwrite local data
            isNewTouch[i+(12*((a-A0)+1))] = true;
          } else {
            isNewRelease[i+(12*((a-A0)+1))] = true;
          }
        }
      }
    } else {
      Serial.print("incomplete packet from secondary board ");
      Serial.println(a-A0, DEC);
      Serial.print("number of bytes read was ");
      Serial.println(numBytesRead);
    }

    digitalWrite(a, LOW);
  }
}

void processTouchInputs(){
  // only make an action if we have one or fewer pins touched
  // ignore multiple touches
  
  for (int i=0; i < totalNumElectrodes; i++){  // Check which electrodes were pressed
    if(isNewTouch[i]){   
      //pin i was just touched
      Serial.print("pin ");
      Serial.print(i);
      Serial.println(" was just touched");
      // turn on the LED
      strip.setPixelColor(ledPins[i], strip.Color(150,0,0));
      strip.show();
      noteOn(0, notes[i], 0x60);

    } else if(isNewRelease[i]){
      //pin i was just released
      Serial.print("pin ");
      Serial.print(i);
      Serial.println(" is no longer being touched");
      // turn off the LED
      strip.setPixelColor(ledPins[i], strip.Color(0,0,0));
      strip.show();
      noteOff(0, notes[i], 0x60);

    }
  }
}

void resetCompoundVariables(){

  // simple reset for all coumpound variables

  touchStatusChanged = false;

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }  
}

// functions below are little helpers based on using the SoftwareSerial
// as a MIDI stream input to the VS1053 - all based on stuff from Nathan Seidle

// Send a MIDI note-on message.  Like pressing a piano key.
// channel ranges from 0-15
void noteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

// Send a MIDI note-off message.  Like releasing a piano key.
void noteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}

// Sends a generic MIDI message. Doesn't check to see that cmd is greater than 127, 
// or that data values are less than 127.
void talkMIDI(byte cmd, byte data1, byte data2) {
  mySerial.write(cmd);
  mySerial.write(data1);

  // Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  // (sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    mySerial.write(data2);
}

// SETTING UP THE INSTRUMENT:
// The below function "setupMidi()" is where the instrument bank is defined. Use the VS1053 instrument library
// below to aid you in selecting your desire instrument from within the respective instrument bank


void setupMidi(){
  
  // Volume - don't comment out this code!
  talkMIDI(0xB0, 0x07, 127); //0xB0 is channel message, set channel volume to max (127)
  
  // ---------------------------------------------------------------------------------------------------------
  // Melodic Instruments GM1 
  // ---------------------------------------------------------------------------------------------------------
  // To Play "Electric Piano" (5):
  talkMIDI(0xB0, 0, 0x00); // Default bank GM1  
  // We change the instrument by changing the middle number in the brackets 
  // talkMIDI(0xC0, number, 0); "number" can be any number from the melodic table below
  talkMIDI(0xC0, 5, 0); // Set instrument number. 0xC0 is a 1 data byte command(55,0) 
  // ---------------------------------------------------------------------------------------------------------
  // Percussion Instruments (Drums, GM1 + GM2) 
  // ---------------------------------------------------------------------------------------------------------  
  // uncomment the two lines of code below to use - you will also need to comment out the two "talkMIDI" lines 
  // of code in the Melodic Instruments section above 
  // talkMIDI(0xB0, 0, 0x78); // Bank select: drums
  // talkMIDI(0xC0, 0, 0); // Set a dummy instrument number
  // ---------------------------------------------------------------------------------------------------------
  
}

/* MIDI INSTRUMENT LIBRARY: 

MELODIC INSTRUMENTS (GM1) 
When using the Melodic bank (0x79 - same as default), open chooses an instrument and the octave to map 
To use these instruments below change "number" in talkMIDI(0xC0, number, 0) in setupMidi()


1   Acoustic Grand Piano       33  Acoustic Bass             65  Soprano Sax           97   Rain (FX 1)
2   Bright Acoustic Piano      34  Electric Bass (finger)    66  Alto Sax              98   Sound Track (FX 2)
3   Electric Grand Piano       35  Electric Bass (pick)      67  Tenor Sax             99   Crystal (FX 3)
4   Honky-tonk Piano           36  Fretless Bass             68  Baritone Sax          100  Atmosphere (FX 4)
5   Electric Piano 1           37  Slap Bass 1               69  Oboe                  101  Brigthness (FX 5)
6   Electric Piano 2           38  Slap Bass 2               70  English Horn          102  Goblins (FX 6)
7   Harpsichord                39  Synth Bass 1              71  Bassoon               103  Echoes (FX 7)
8   Clavi                      40  Synth Bass 2              72  Clarinet              104  Sci-fi (FX 8) 
9   Celesta                    41  Violin                    73  Piccolo               105  Sitar
10  Glockenspiel               42  Viola                     74  Flute                 106  Banjo
11  Music Box                  43  Cello                     75  Recorder              107  Shamisen
12  Vibraphone                 44  Contrabass                76  Pan Flute             108  Koto
13  Marimba                    45  Tremolo Strings           77  Blown Bottle          109  Kalimba
14  Xylophone                  46  Pizzicato Strings         78  Shakuhachi            110  Bag Pipe
15  Tubular Bells              47  Orchestral Harp           79  Whistle               111  Fiddle
16  Dulcimer                   48  Trimpani                  80  Ocarina               112  Shanai
17  Drawbar Organ              49  String Ensembles 1        81  Square Lead (Lead 1)  113  Tinkle Bell
18  Percussive Organ           50  String Ensembles 2        82  Saw Lead (Lead)       114  Agogo
19  Rock Organ                 51  Synth Strings 1           83  Calliope (Lead 3)     115  Pitched Percussion
20  Church Organ               52  Synth Strings 2           84  Chiff Lead (Lead 4)   116  Woodblock
21  Reed Organ                 53  Choir Aahs                85  Charang Lead (Lead 5) 117  Taiko
22  Accordion                  54  Voice oohs                86  Voice Lead (Lead)     118  Melodic Tom
23  Harmonica                  55  Synth Voice               87  Fifths Lead (Lead 7)  119  Synth Drum
24  Tango Accordion            56  Orchestra Hit             88  Bass + Lead (Lead 8)  120  Reverse Cymbal
25  Acoustic Guitar (nylon)    57  Trumpet                   89  New Age (Pad 1)       121  Guitar Fret Noise
26  Acoutstic Guitar (steel)   58  Trombone                  90  Warm Pad (Pad 2)      122  Breath Noise
27  Electric Guitar (jazz)     59  Tuba                      91  Polysynth (Pad 3)     123  Seashore 
28  Electric Guitar (clean)    60  Muted Trumpet             92  Choir (Pad 4)         124  Bird Tweet
29  Electric Guitar (muted)    61  French Horn               93  Bowed (Pad 5)         125  Telephone Ring
30  Overdriven Guitar          62  Brass Section             94  Metallic (Pad 6)      126  Helicopter
31  Distortion Guitar          63  Synth Brass 1             95  Halo (Pad 7)          127  Applause
32  Guitar Harmonics           64  Synth Brass 2             96  Sweep (Pad 8)         128  Gunshot  

PERCUSSION INSTRUMENTS (GM1 + GM2)

When in the drum bank (0x78), there are not different instruments, only different notes.
To play the different sounds, select an instrument # like 5, then play notes 27 to 87.
 
27  High Q                     43  High Floor Tom            59  Ride Cymbal 2         75  Claves 
28  Slap                       44  Pedal Hi-hat [EXC 1]      60  High Bongo            76  Hi Wood Block
29  Scratch Push [EXC 7]       45  Low Tom                   61  Low Bongo             77  Low Wood Block
30  Srcatch Pull [EXC 7]       46  Open Hi-hat [EXC 1]       62  Mute Hi Conga         78  Mute Cuica [EXC 4] 
31  Sticks                     47  Low-Mid Tom               63  Open Hi Conga         79  Open Cuica [EXC 4]
32  Square Click               48  High Mid Tom              64  Low Conga             80  Mute Triangle [EXC 5]
33  Metronome Click            49  Crash Cymbal 1            65  High Timbale          81  Open Triangle [EXC 5]
34  Metronome Bell             50  High Tom                  66  Low Timbale           82  Shaker
35  Acoustic Bass Drum         51  Ride Cymbal 1             67  High Agogo            83 Jingle bell
36  Bass Drum 1                52  Chinese Cymbal            68  Low Agogo             84  Bell tree
37  Side Stick                 53  Ride Bell                 69  Casbasa               85  Castanets
38  Acoustic Snare             54  Tambourine                70  Maracas               86  Mute Surdo [EXC 6] 
39  Hand Clap                  55  Splash Cymbal             71  Short Whistle [EXC 2] 87  Open Surdo [EXC 6]
40  Electric Snare             56  Cow bell                  72  Long Whistle [EXC 2]  
41  Low Floor Tom              57  Crash Cymbal 2            73  Short Guiro [EXC 3]
42  Closed Hi-hat [EXC 1]      58  Vibra-slap                74  Long Guiro [EXC 3]

*/
