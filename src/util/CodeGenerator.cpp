
#include "CodeGenerator.h"


/// File listing helper
// void CodeGenerator::printDirectory(File dir, int numTabs = 0) {
//     while(true) {
//         File entry =  dir.openNextFile();
//         if (! entry) {
//             // no more files
//             //Serial.println(F("**nomorefiles**"));
//             break;
//         }
//         for (uint8_t i=0; i<numTabs; i++) {
//             Serial.print(F("\t"));
//         }
//         Serial.print(entry.name());
//             if (entry.isDirectory()) {
//             Serial.println(F("/"));
//             printDirectory(entry, numTabs+1);
//         } else {
//             // files have sizes, directories do not
//             Serial.print(F("\t\t"));
//             Serial.println(entry.size(), DEC);
//         }
//         entry.close();
//     }
// }


void CodeGenerator::printPROGMEMtrackNames(const char *soundsDir) {
    File dir = SD.open(soundsDir);

    int fileCount = 0;
    while(true) {
        File entry =  dir.openNextFile();
        if (! entry) {
            break;
        }

        if (!entry.isDirectory()) {
            Serial.print(F("const char trackname_"));
            if (fileCount < 10) {
                Serial.print(F("0"));
            }
            Serial.print(fileCount);
            Serial.print(F("[] PROGMEM = \""));
            Serial.print(entry.name());
            Serial.println(F("\";"));
            fileCount++;
        }
        entry.close();
    }

    Serial.println(F("\n"));
    Serial.println(F("PGM_P const soundTrackNames[] PROGMEM =\n{"));
    for(int i=0; i<fileCount; i++) {
        Serial.print(F("\ttrackname_"));
        if (i < 10) {
            Serial.print(F("0"));
        }
        Serial.print(i);
        Serial.println(F(","));
    }
    Serial.println(F("};"));

    dir.close();
}


