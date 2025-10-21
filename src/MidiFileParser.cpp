#include "MidiFileParser.h"
#include <cstdio>

bool MidiFileParser::load(const char* filename) {
    printf("Loading MIDI file: %s\n", filename);
    // TODO: Реализовать парсинг MIDI файла
    // Пока добавим тестовые события
    events.push_back(MidiEvent(0.0, NOTE_ON, 0, 60, 100));
    events.push_back(MidiEvent(1.0, NOTE_OFF, 0, 60, 0));
    events.push_back(MidiEvent(2.0, NOTE_ON, 0, 64, 100));
    events.push_back(MidiEvent(3.0, NOTE_OFF, 0, 64, 0));
    return true;
}