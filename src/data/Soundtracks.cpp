
#include <Arduino.h>

#include "Soundtracks.h"

#include "SoundtracksData.h"

const uint8_t Soundtracks::trackNamesCount = ARRAY_SIZE(soundTrackNames);

void Soundtracks::getTrackName_P(uint8_t trackNumber, char *buffer) {
    strcpy_P(buffer, (PGM_P)pgm_read_word(&soundTrackNames[trackNumber]));
}
long Soundtracks::getTrackLength_P(uint8_t trackNumber) {
    long value;
    memcpy_P(&value, &soundTrackDurations[trackNumber], sizeof(value));
    return value;
}


