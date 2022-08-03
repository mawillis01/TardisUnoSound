#include <Arduino.h>


// Wire Slave Receiver
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Master Writer" example for use with this

// Created 29 March 2006

// This example code is in the public domain.

// #define DEBUG 1
// #define OUTPUT_TRACK_CODE 1

#include <SPI.h>
#include <Wire.h>

#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>


// define the pins used
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin
// #define DREQ 2       // VS1053 Data request, ideally an Interrupt pin  --- seems to break i2c

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);



// export to .h file   ????????????????????????????????????????????????????????????
// N.B. I2C 7-bit addresses can only be in the range of 0x08 -> 0x77
const int PLAYER_I2C_ADDRESS = 0x14;
const int MUX_ADDRESS = 0x70;
const int PLAYER_MUX_CHANNEL = 0;
//
//          12345678.123
// '/sounds/FILENAME.MP3'  music board only accepts 8.3 filenames
//  12345678901234567890
//           1         2
const int MAX_TRACK_NAME_LENGTH = 20;


// const byte SOUNDPLAYER_MSG_VOLUME = 0;
// const byte SOUNDPLAYER_MSG_TRACK = 1;
// const byte SOUNDPLAYER_MSG_STOP = 2;
// const byte SOUNDPLAYER_MSG_TONE = 3;

const byte SOUNDPLAYER_MSG_VOLUME = 100;
const byte SOUNDPLAYER_MSG_TRACK = 101;
const byte SOUNDPLAYER_MSG_TRACK_AT_VOLUME = 102;
const byte SOUNDPLAYER_MSG_STOP = 103;
const byte SOUNDPLAYER_MSG_TONE = 104;

const bool RESULT_OK = true;
const bool RESULT_ERROR = false;


uint8_t i2cMsgBuffer[64];
int ic2MsgHowMany = 0;
int msgReceived = false;
const char *soundsDir = (const char*)"/sounds/";

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void initI2cMsgBuffer() {
  memset(i2cMsgBuffer,0,sizeof(i2cMsgBuffer));
}


#ifdef DEBUG
void dumpData(byte *data, int dataLength) {
    for (int n = 0; n <= dataLength; n++) {
        byte v = data[n];
        // Serial.print("0x");
        // Serial.print(v < 16 ? "0" : "");
        // Serial.print(v, HEX);
        Serial.print(v < 10 ? "0" : "");
        Serial.print(v < 100 ? "0" : "");
        Serial.print(v, DEC);
        Serial.print(" ");
    }
    Serial.println();
}
#endif

void processI2cMessage() {
  // no interrupts? ?????????????????
  uint8_t msgType = i2cMsgBuffer[0];

  #ifdef DEBUG
    // dumpData(i2cMsgBuffer, sizeof(i2cMsgBuffer));
    dumpData(i2cMsgBuffer, ic2MsgHowMany);
  #endif

  if( msgType == SOUNDPLAYER_MSG_VOLUME ) {
    // check for correct number of bytes... ?????
    int newVolume = i2cMsgBuffer[1]; // get single byte message type
    #ifdef DEBUG
    Serial.print(F("setting volume to "));
    Serial.println(newVolume);
    #endif
    musicPlayer.setVolume(newVolume, newVolume);
  }

  if( msgType == SOUNDPLAYER_MSG_TRACK ) {
    char filename[MAX_TRACK_NAME_LENGTH + 1];
    memset(filename, 0, sizeof(filename));
    strcpy(filename, soundsDir);
    memcpy(&filename[strlen(soundsDir)], (char *)&i2cMsgBuffer[1], ic2MsgHowMany - 1);
    //??? send filename to music player shield
    #ifdef DEBUG
    Serial.print(F("start playing: "));
    Serial.print((char *)&i2cMsgBuffer[1]);
    Serial.print(F(" : "));
    Serial.print(filename);
    Serial.print(F(" "));
    Serial.println(strlen(filename));
    #endif

    if (musicPlayer.playingMusic) {
      //??? stop? needed???? ????????????????????????????????
      musicPlayer.stopPlaying();
    }
    bool result = musicPlayer.startPlayingFile(filename);
    #ifdef DEBUG
    Serial.print(F("startPlayingFile returned: "));
    Serial.println(result);
    #endif
    if(result == RESULT_ERROR) {
      // ERROR RESPONSE ===================================== ??????????????
    }
  }


  if( msgType == SOUNDPLAYER_MSG_TRACK_AT_VOLUME ) {
    int newVolume = i2cMsgBuffer[1];
    musicPlayer.setVolume(newVolume, newVolume);
    delay(50); // ============================================

    char filename[MAX_TRACK_NAME_LENGTH + 1];
    memset(filename, 0, sizeof(filename));
    strcpy(filename, soundsDir);
    memcpy(&filename[strlen(soundsDir)], (char *)&i2cMsgBuffer[2], ic2MsgHowMany - 1);
    //??? send filename to music player shield
    #ifdef DEBUG
    Serial.print(F("start playing at: "));
    Serial.print(newVolume);
    Serial.print(F(" : "));
    Serial.print(filename);
    Serial.print(F(" "));
    Serial.println(strlen(filename));
    #endif

    if (musicPlayer.playingMusic) {
      //??? stop? needed???? ????????????????????????????????
      musicPlayer.stopPlaying();
    }
    bool result = musicPlayer.startPlayingFile(filename);
    #ifdef DEBUG
    Serial.print(F("startPlayingFile returned: "));
    Serial.println(result);
    #endif
    if(result == RESULT_ERROR) {
      // ERROR RESPONSE ===================================== ??????????????
    }
  }

  if( msgType == SOUNDPLAYER_MSG_STOP ) {
    musicPlayer.stopPlaying();
    #ifdef DEBUG
    Serial.println(F("stop playing"));
    #endif
  }

// NOT WORKING ================================================ ???
  if( msgType == SOUNDPLAYER_MSG_TONE ) {
    // if (musicPlayer.playingMusic) {
    //   musicPlayer.stopPlaying();
    // }
    // uint8_t toneFreq = i2cMsgBuffer[1]; // get single byte message type
    musicPlayer.sineTest(0x66, 250);
    #ifdef DEBUG
    Serial.println(F("play tone"));
    #endif
  }

  initI2cMsgBuffer();
  msgReceived = false;
  // restore interrupts... Serial?
}


void receiveMessage(int howManyBytes) {
  // ????? IMPORTANT -- needs a way to stage messages to allow previous message to finish processing
  // Wire library only supports 32 byte messages...
  // good to know (https://www.arduino.cc/reference/en/language/functions/communication/wire/)

  uint8_t bufSize = sizeof(i2cMsgBuffer);

  #ifdef DEBUG
    Serial.print("rcv (");
    Serial.print(howManyBytes);
    Serial.print("): ");
  #endif
  for ( uint8_t i=0; i<howManyBytes; i++) {
    byte b = Wire.read();

    #ifdef DEBUG
        // Serial.print("0x");
        // Serial.print(b < 16 ? "0" : "");
        // Serial.print(b, HEX);
        Serial.print(b < 10 ? "0" : "");
        Serial.print(b < 100 ? "0" : "");
        Serial.print(b, DEC);
        Serial.print(" ");
    #endif

    if ( i < bufSize - 1 ) {
      i2cMsgBuffer[i] = b;
    }
    // else throw out overflow...
  }
  #ifdef DEBUG
    Serial.println();
  #endif
  ic2MsgHowMany = howManyBytes;
  msgReceived = true;
}


#ifdef OUTPUT_TRACK_CODE

/// File listing helper
void printDirectory(File dir, int numTabs = 0) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

void printPROGMEMtrackNames(File dir) {
    int fileCount = 0;
    while(true) {
        File entry =  dir.openNextFile();
        if (! entry) {
            break;
        }

        if (!entry.isDirectory()) {
            Serial.print("const char trackname_");
            if (fileCount < 10) {
                Serial.print("0");
            }
            Serial.print(fileCount);
            Serial.print("[] PROGMEM = \"");
            Serial.print(entry.name());
            Serial.println("\";");
            fileCount++;
        }
        entry.close();
    }

    Serial.println("\n");
    Serial.println("PGM_P const soundTrackNames[] PROGMEM =\n{");
    for(int i=0; i<fileCount; i++) {
        Serial.print("\ttrackname_");
        if (i < 10) {
            Serial.print("0");
        }
        Serial.print(i);
        Serial.println(",");
    }
    Serial.println("};");
}
#endif


void initSDCardAndMusicPlayer() {

  if (! musicPlayer.begin()) { // initialise the music player
    #ifdef DEBUG
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
    #endif
  }
  // error handling! ????????????????????????????????????????????????????
  #ifdef DEBUG
  Serial.println(F("VS1053 found"));
  #endif

  if (!SD.begin(CARDCS)) {
    #ifdef DEBUG
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more                  ????? ERROR RESPONSE!
    #endif
  }

  // list files
  #ifdef OUTPUT_TRACK_CODE
  printPROGMEMtrackNames(SD.open(soundsDir));
  #endif

  // Set volume for left, right channels. lower numbers == louder volume
  // 0 (max) - 100 (off)
  musicPlayer.setVolume(60,60);

  // Timer interrupts are not suggested, better to use DREQ interrupt!
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  bool result = musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  #ifdef DEBUG
    Serial.print(F("useInterrupt result: "));
    Serial.println(result);
  #endif
  if(result == RESULT_ERROR) {
    // ERROR RESPONSE ===================================== ??????????????
    #ifdef DEBUG
      Serial.println(F("error setting interrupt use"));
    #endif
  }
  
  // Play one file, don't return until complete
  // Serial.println(F("Playing trdstoff"));
  // musicPlayer.playFullFile("/trdstoff.mp3");
  // musicPlayer.startPlayingFile("/trdstoff.mp3");
  // Play another file in the background, REQUIRES interrupts!
  // Serial.println(F("Playing portal01"));
  // musicPlayer.startPlayingFile("/portal01.mp3");
}




// =========================================================================
void setup() {
  #ifdef DEBUG
  Serial.begin(9600);           // start serial for output
//  while (!Serial);  //  DON'T PUT THIS OUTSIDE OF AN 'IS DEBUG' block
  Serial.println(F("waiting for i2c messages"));
  #endif

  // if DEBUG ?????
  // pinMode(LED_BUILTIN, OUTPUT);

  initI2cMsgBuffer();
  initSDCardAndMusicPlayer(); // check for error ??? ===========================================

  musicPlayer.sineTest(0x44, 250);    // Make a tone to indicate VS1053 is working

  Wire.begin(PLAYER_I2C_ADDRESS);    // join i2c bus
  Wire.onReceive(receiveMessage); // register event
}

// =========================================================================
void loop() {
  if( msgReceived ) {
    processI2cMessage();
  }
  // delay(10);  // ??? ==================================
}


