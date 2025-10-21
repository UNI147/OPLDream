#ifndef MIDIFILEPARSER_H
#define MIDIFILEPARSER_H

#include "MidiEvent.h"
#include <vector>
#include <fstream>

class MidiFileParser {
public:
    bool load(const char* filename);
    const std::vector<MidiEvent>& getEvents() const { return events; }

private:
    std::vector<MidiEvent> events;
    
    bool parseTrack(std::ifstream& file, uint16_t division);
    uint32_t readVariableLength(std::ifstream& file, uint32_t& remaining);
};

#endif