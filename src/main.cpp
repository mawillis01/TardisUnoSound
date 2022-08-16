#include <Arduino.h>

// #define DEBUG 1
// #define OUTPUT_TRACK_CODE 1

#include <SPI.h>
#include <Wire.h>

#include <SD.h>
#include <Adafruit_VS1053.h>

#include "util/MemoryFree.h"
#include "util/CodeGenerator.h"

#include "data/Soundtracks.h"


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

// const byte SOUNDPLAYER_MSG_VOLUME = 100;
// const byte SOUNDPLAYER_MSG_TRACK = 101;

const byte SOUNDPLAYER_MSG_AMBIANCE = 100;
const byte SOUNDPLAYER_MSG_EFFECT = 101;

const byte SOUNDPLAYER_MSG_TRACK_AT_VOLUME = 102;
const byte SOUNDPLAYER_MSG_STOP = 103;
const byte SOUNDPLAYER_MSG_TONE = 104;
const byte SOUNDPLAYER_MSG_MASTER_VOLUME = 105;

uint8_t masterVolume = 100; // 0(low) .. 255(full)
uint8_t lastRawVolumeRequest = 100; // 0 .. masterVolume
const bool RESULT_OK = true;
const bool RESULT_ERROR = false;

const uint8_t SOUND_VOLUME_RAW_LOW = 250;

uint8_t i2cMsgBuffer[32];
int ic2MsgHowMany = 0;
int msgReceived = false;
const char *soundsDir = (const char*)"/sounds/";


// =========================================================================
void initI2cMsgBuffer() {
  memset(i2cMsgBuffer, 0, sizeof(i2cMsgBuffer));
}


// =========================================================================
// The useable volume range for the musicPlayer object is 0 (loudest) to 100
// (barely audible). I want this to be reversed (0 off, 100 max) and also
// mapped against the "master volume" to scale all volume requests against
// whatever the user sets the volume control to. I can't just have a single
// volume setting because different tracks have different levels which need
// to be normalized with individula volume settings.
// This function is called whenever a request for a "local" volume level is
// requested such as the SOUNDPLAYER_MSG_TRACK_AT_VOLUME message.
// Input should be [0..100]
// masterVolume gets constrained to [x..100] so the sound is never completely
// shut off.
// =========================================================================
uint8_t convertVolume(uint8_t volume) {
  uint8_t contrainedRequest = constrain(volume, 0, 100);
  uint8_t constrainedMaster = map(masterVolume, 0, 100, 15, 100);
  // scale request to the currect master setting
  uint8_t scaledToMasterVolume = map(contrainedRequest, 0, 100, 0, constrainedMaster);
  // reverse to match sound player requirement
  uint8_t newVolume = 100 - scaledToMasterVolume;

    // Serial.print("lastRaw: ");
    // Serial.print(lastRawVolumeRequest);
    // Serial.print(" request: ");
    // Serial.print(volume);
    // Serial.print("  masterVolume: ");
    // Serial.print(masterVolume);
    // Serial.print("  masterToMappedVolume: ");
    // Serial.print(scaledToMasterVolume);
    // Serial.print("  newVolume is: ");
    // Serial.println(newVolume); // =======================================
  return newVolume;
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



// ============================================================================
// ============================================================================
void processI2cMessage() {
  // no interrupts? ?????????????????
  uint8_t msgType = i2cMsgBuffer[0];

  #ifdef DEBUG
    // dumpData(i2cMsgBuffer, sizeof(i2cMsgBuffer));
    dumpData(i2cMsgBuffer, ic2MsgHowMany);
    // Serial.print("free memory: ");
    // Serial.println(freeMemory()); // says 2279, make no sense on UNO with only 2k
  #endif

  // =========================================================================
  // =========================================================================
  if( msgType == SOUNDPLAYER_MSG_MASTER_VOLUME ) {
    int newMasterVolume = i2cMsgBuffer[1]; // get single byte message type
    #ifdef DEBUG
    Serial.print(F("setting master volume to "));
    Serial.println(newMasterVolume);
    #endif
    masterVolume = constrain(newMasterVolume, 0, 100);
    // needs to remap last raw volume to newVolume
    uint8_t newVolume = convertVolume(lastRawVolumeRequest);
    musicPlayer.setVolume(newVolume, newVolume);
  }

  // =========================================================================
  // =========================================================================
  // if( msgType == SOUNDPLAYER_MSG_VOLUME ) {
  //   // check for correct number of bytes... ?????
  //   int newVolume = i2cMsgBuffer[1]; // get single byte message type
  //   #ifdef DEBUG
  //   Serial.print(F("setting volume to "));
  //   Serial.println(newVolume);
  //   #endif
  //   musicPlayer.setVolume(newVolume, newVolume);
  // }

  // =========================================================================
  // =========================================================================
  // if( msgType == SOUNDPLAYER_MSG_TRACK ) {
  //   char filename[MAX_TRACK_NAME_LENGTH + 1];
  //   memset(filename, 0, sizeof(filename));
  //   strcpy(filename, soundsDir);
  //   memcpy(&filename[strlen(soundsDir)], (char *)&i2cMsgBuffer[1], ic2MsgHowMany - 1);
  //   #ifdef DEBUG
  //   Serial.print(F("start playing: "));
  //   Serial.print((char *)&i2cMsgBuffer[1]);
  //   Serial.print(F(" : "));
  //   Serial.print(filename);
  //   Serial.print(F(" "));
  //   Serial.println(strlen(filename));
  //   #endif

  //   if (musicPlayer.playingMusic) {
  //     musicPlayer.stopPlaying();
  //   }

  //   bool result = musicPlayer.startPlayingFile(filename);
  //   #ifdef DEBUG
  //   Serial.print(F("startPlayingFile returned: "));
  //   Serial.println(result);
  //   #endif
  //   if(result == RESULT_ERROR) {
  //     // ERROR RESPONSE ===================================== ??????????????
  //   }
  // }


  // =========================================================================
  // =========================================================================
  if( msgType == SOUNDPLAYER_MSG_TRACK_AT_VOLUME ) {
    int newVolume = i2cMsgBuffer[1];

    // prevent jumps in volume while playing a track ===
    if (musicPlayer.playingMusic) {
      musicPlayer.stopPlaying();
    }
    musicPlayer.setVolume(SOUND_VOLUME_RAW_LOW, SOUND_VOLUME_RAW_LOW);
    delay(10);
    // =================================================

    char filename[MAX_TRACK_NAME_LENGTH + 1];
    memset(filename, 0, sizeof(filename));
    strcpy(filename, soundsDir);
    memcpy(&filename[strlen(soundsDir)], (char *)&i2cMsgBuffer[2], ic2MsgHowMany - 1);

    #ifdef DEBUG
    Serial.print(F("start playing at: "));
    Serial.print(newVolume);
    Serial.print(F(" : "));
    Serial.print(filename);
    Serial.print(F(" "));
    Serial.println(strlen(filename));
    #endif

    bool result = musicPlayer.startPlayingFile(filename);
    #ifdef DEBUG
    Serial.print(F("startPlayingFile returned: "));
    Serial.println(result);
    #endif
    if(result == RESULT_ERROR) {
      // ERROR RESPONSE ===================================== ??????????????
    }
    lastRawVolumeRequest = newVolume;
    newVolume = convertVolume(newVolume);
    musicPlayer.setVolume(newVolume, newVolume);
  }


  // =========================================================================
  // =========================================================================
  if( msgType == SOUNDPLAYER_MSG_STOP ) {
    musicPlayer.stopPlaying();
    #ifdef DEBUG
    Serial.println(F("stop playing"));
    #endif
  }


  // =========================================================================
  // NOTE: DOES NOT WORK WHILE TRACK IS PLAYING  =============================
  if( msgType == SOUNDPLAYER_MSG_TONE ) {
    uint8_t toneFreq = i2cMsgBuffer[1]; // get single byte message type
    musicPlayer.sineTest(toneFreq, 250);
    #ifdef DEBUG
    Serial.println(F("play tone"));
    #endif
  }

  initI2cMsgBuffer();
  msgReceived = false;
  // restore interrupts... Serial?
}





// =========================================================================
// Function that executes whenever data is received from master.
// This function is registered as an interrupt response, see setup().
// =========================================================================
void receiveMessage(int howManyBytes) {
  // ????? IMPORTANT -- needs a way to queue messages to allow previous message to finish processing
  // Wire library only supports 32 byte messages...
  // good to know (https://www.arduino.cc/reference/en/language/functions/communication/wire/)

  uint8_t bufSize = sizeof(i2cMsgBuffer);

  #ifdef DEBUG
    Serial.print(F("rcv ("));
    Serial.print(howManyBytes);
    Serial.print(F("): "));
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



// =========================================================================
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
  CodeGenerator::printPROGMEMtrackNames(soundsDir);
  #endif

  // Set volume for left, right channels. lower numbers == louder volume
  // 0 (max) - 100 (off)
  musicPlayer.setVolume(50,50);

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
  
}


void startupBeep() {
  // NOTE: setVolume does NOT change tone volumes
  // yes it does... but only after starting the tone? huh?
  // this occasionally locks up with a continuous tone...
  musicPlayer.sineTest(0x44, 250);    // Make a tone to indicate VS1053 is working
  musicPlayer.setVolume(60,60);
  delay(250);
  musicPlayer.stopPlaying(); // hmmm, that fixes the comtinuous tone
}




// =========================================================================
// =========================================================================
void setup() {
  #ifdef DEBUG
  Serial.begin(9600);           // start serial for output
//  while (!Serial);  //  DON'T PUT THIS OUTSIDE OF AN 'IS DEBUG' block
  Serial.println(F("waiting for i2c messages"));
  #endif

  Serial.begin(9600);           // start serial for output
  Serial.println(F("begin....")); // ====================================================================

  initI2cMsgBuffer();
  initSDCardAndMusicPlayer(); // check for error ??? ===========================================

  Wire.begin(PLAYER_I2C_ADDRESS);   // join i2c bus
  Wire.onReceive(receiveMessage);   // register event

  // long trackLength = Soundtracks::getTrackLength_P(1); // only worked with a copy in projects...
  startupBeep();
}

// =========================================================================
// =========================================================================
void loop() {
  if( msgReceived ) {
    processI2cMessage();
  }
}


