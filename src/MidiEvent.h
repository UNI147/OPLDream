#ifndef MIDIEVENT_H
#define MIDIEVENT_H

#include <cstdint>
#include <vector>

enum MidiEventType {
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    CONTROL_CHANGE = 0xB0,
    PROGRAM_CHANGE = 0xC0,
    PITCH_BEND = 0xE0
};

struct MidiEvent {
    double time; // Время в секундах
    MidiEventType type;
    uint8_t channel;
    uint8_t data1; // Нота или номер контроллера
    uint8_t data2; // Скорость нажатия или значение контроллера

    MidiEvent(double t, MidiEventType ty, uint8_t ch, uint8_t d1, uint8_t d2)
        : time(t), type(ty), channel(ch), data1(d1), data2(d2) {}
};

#endif