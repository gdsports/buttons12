/*
  MIT License

  Copyright (c) 2018 gdsports625@gmail.com

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
*/

/*
 * MIDI Flatter -- It can't fight but it is flatter!
 *
 * buttons12 -- 12 button MIDI USB controller
 * 
 * 12 key membrane keypad matrix
 * SparkFun Pro Micro or Arduino Micro
 * 
 */

#include <Keypad.h>   /* https://playground.arduino.cc/Code/Keypad */
#include <MIDIUSB.h>  /* https://www.arduino.cc/en/Reference/MIDIUSB */
/* Not used except for some definitions such as midi::TimeCodeQuarterFrame */
#include <MIDI.h>     /* https://github.com/FortySevenEffects/arduino_midi_library */

/* Setup keypad matrix */
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 3, 2}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

/*************** MIDI USB functions ****************************/

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void USBNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t evt = {0x08, (byte)(0x80 | (channel - 1)), pitch, velocity};
  MidiUSB.sendMIDI(evt);
}

void USBNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t evt = {0x09, (byte)(0x90 | (channel - 1)), pitch, velocity};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0A = polyphonic key pressure)
// Second parameter is poly-keypress, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the pressure on the key after it "bottoms out"

void USBAfterTouchPoly(byte channel, byte pitch, byte pressure) {
  midiEventPacket_t evt = {0x0A, (byte)(0xA0 | (channel - 1)), pitch, pressure};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void USBControlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, (byte)(0xB0 | (channel - 1)), control, value};
  MidiUSB.sendMIDI(event);
}

// First parameter is the event type (0x0C = program change)
// Second parameter is program number, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the program number.
// Fourth parameter is 0.

void USBProgramChange(byte channel, byte number) {
  midiEventPacket_t evt = {0x0C, (byte)(0xC0 | (channel - 1)), number, 0};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0D = after touch channel pressure)
// Second parameter is channel pressure, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the channel pressure value.
// Fourth parameter is 0.

void USBAfterTouchChannel(byte channel, byte pressure) {
  midiEventPacket_t evt = {0x0D, (byte)(0xD0 | (channel - 1)), pressure, 0};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0E = pitch bend)
// Second parameter is pitch bend, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the least significant 7 bits of pitch bend
// Fourth parameter is the most significant 7 bits of pitch bend

void USBPitchBend(byte channel, int bend) {
  uint16_t uint_bend = (uint16_t)(bend - MIDI_PITCHBEND_MIN);
  midiEventPacket_t evt = {0x0E, (byte)(0xE0 | (channel - 1)),
    (byte)(uint_bend & 0x7F), (byte)((uint_bend >> 7) & 0x7F)
  };
  MidiUSB.sendMIDI(evt);
}

// Be sure to include 0xF0 and 0xF7.
void USBSystemExclusive(unsigned size, byte *data) {
  uint8_t bytesOut;
  midiEventPacket_t evt;

  while (size > 0) {
    bytesOut = min(3, size);
    evt.byte1 = *data++;
    size -= bytesOut;
    switch (bytesOut) {
      case 1:
        evt.header = 0x05;
        evt.byte2 = evt.byte3 = 0;
        break;
      case 2:
        evt.header = 0x06;
        evt.byte2 = *data++;
        evt.byte3 = 0;
        break;
      case 3:
        evt.header = (size > 0) ? 0x04 : 0x07;
        evt.byte2 = *data++;
        evt.byte3 = *data++;
        break;
      default:
        break;
    }
    MidiUSB.sendMIDI(evt);
  }
}

// Be sure to include 0xF0 and 0xF7.
// This version reads a buffer stored in PROGMEM
void USBSystemExclusive_P(unsigned size, byte *data) {
  uint8_t bytesOut;
  midiEventPacket_t evt;

  while (size > 0) {
    bytesOut = min(3, size);
    evt.byte1 = pgm_read_byte_near(data++);
    size -= bytesOut;
    switch (bytesOut) {
      case 1:
        evt.header = 0x05;
        evt.byte2 = evt.byte3 = 0;
        break;
      case 2:
        evt.header = 0x06;
        evt.byte2 = pgm_read_byte_near(data++);
        evt.byte3 = 0;
        break;
      case 3:
        evt.header = (size > 0) ? 0x04 : 0x07;
        evt.byte2 = pgm_read_byte_near(data++);
        evt.byte3 = pgm_read_byte_near(data++);
        break;
      default:
        break;
    }
    MidiUSB.sendMIDI(evt);
  }
}

void USBTimeCodeQuarterFrame(byte data)
{
  midiEventPacket_t evt = {0x02, midi::TimeCodeQuarterFrame, data, 0};
  MidiUSB.sendMIDI(evt);
}

void USBSongSelect(byte songnumber)
{
  midiEventPacket_t evt = {0x02, midi::SongSelect, songnumber, 0};
  MidiUSB.sendMIDI(evt);
}

void USBSongPosition(unsigned beats)
{
  midiEventPacket_t evt = {0x03, midi::SongPosition, (byte)(beats & 0x7F),
    (byte)((beats >> 7) & 0x7F)
  };
  MidiUSB.sendMIDI(evt);
}

inline void USBRealTime(midi::MidiType midiType)
{
  midiEventPacket_t evt = {0x0F, (byte)midiType, 0, 0};
  MidiUSB.sendMIDI(evt);
}

void USBTuneRequest(void)
{
  USBRealTime(midi::TuneRequest);
}

void USBTimingClock(void)
{
  USBRealTime(midi::Clock);
}

void USBStart(void)
{
  USBRealTime(midi::Start);
}

void USBContinue(void)
{
  USBRealTime(midi::Continue);
}

void USBStop(void)
{
  USBRealTime(midi::Stop);
}

void USBActiveSensing(void)
{
  USBRealTime(midi::ActiveSensing);
}

void USBSystemReset(void)
{
  USBRealTime(midi::SystemReset);
}

/* Put sysex in PROGMEM to avoid running out of RAM */
const byte SYSEX1[] PROGMEM = {
  0xF0, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xF7
};

void setup() {
  Serial.begin(115200);
  keypad.addEventListener(keypadEvent);
}

void loop() {
  keypad.getKey();
}

/* Called whenever a new keyboard event occurs */
void keypadEvent(KeypadEvent key)
{
  switch (keypad.getState()) {
    case PRESSED:
      Serial.print(F("Key down "));
      Serial.println(key);
      switch (key) {
        case '1':
          USBNoteOn(1, 60, 127);
          break;
        case '2':
          USBNoteOn(1, 61, 127);
          break;
        case '3':
          USBNoteOn(1, 62, 127);
          break;
        case '4':
          break;
        case '5':
          break;
        case '6':
          break;
        case '7':
          break;
        case '8':
          break;
        case '9':
          break;
        case '*':
          USBSystemExclusive_P(sizeof(SYSEX1), SYSEX1);
          break;
        case '0':
          break;
        case '#':
          break;
        default:
          break;
      }
      MidiUSB.flush();
      break;

    case RELEASED:
      Serial.print(F("Key up "));
      Serial.println(key);
      switch (key) {
        case '1':
          USBNoteOff(1, 60, 127);
          break;
        case '2':
          USBNoteOff(1, 61, 127);
          break;
        case '3':
          USBNoteOff(1, 62, 127);
          break;
        case '4':
          break;
        case '5':
          break;
        case '6':
          break;
        case '7':
          break;
        case '8':
          break;
        case '9':
          break;
        case '*':
          break;
        case '0':
          break;
        case '#':
          break;
        default:
          break;
      }
      MidiUSB.flush();
      break;
  }
}
