
#ifndef SOUNDTRACKS_H
#define SOUNDTRACKS_H

#include <Arduino.h>

// #include "Common.h"
// #include "util/templates.h"
#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))

class Soundtracks {
    public:
        // only 8.3 filename format supported by sound card currently
        static const uint8_t maxTracknameLength = 13; // 12 + 1 for null terminator
        static const uint8_t trackNamesCount;

        //
        // Use  char buffer[Soundtracks::maxNameLength]  to allocate local buffer;
        //
        static void getTrackName_P(uint8_t trackNumber, char *buffer);
        static long getTrackLength_P(uint8_t trackNumber);
};

#endif
