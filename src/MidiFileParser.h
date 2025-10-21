#ifndef MIDIFILEPARSER_H
#define MIDIFILEPARSER_H

#include "MidiEvent.h"
#include <vector>

class MidiFileParser {
public:
    bool load(const char* filename);
    const std::vector<MidiEvent>& getEvents() const { return events; }

private:
    std::vector<MidiEvent> events;
};

#endif