#ifndef OPL3DRIVER_H
#define OPL3DRIVER_H

#include "MidiEvent.h"
#include "OPL3Emulator.h"

struct OPL3Patch {
    // Параметры для двух операторов
    uint8_t trem_vib_sus_ksr_multi[2];
    uint8_t ksl_outputLevel[2];
    uint8_t attackDecay[2];
    uint8_t sustainRelease[2];
    uint8_t waveform[2];
    uint8_t feedback;
    uint8_t synthType;
};

class OPL3Driver {
public:
    OPL3Driver(OPL3Emulator& emu);
    void handleEvent(const MidiEvent& event);
    void loadPatch(uint8_t midiChannel, const OPL3Patch& patch);
    
    // Сделаем методы публичными для тестирования
    void noteOn(int midiChannel, uint8_t note, uint8_t velocity);
    void noteOff(int midiChannel, uint8_t note);

private:
    OPL3Emulator& emulator;
    struct ChannelMapping {
        int oplChannel;     // Какой физический канал OPL3 использует этот MIDI-канал
        OPL3Patch patch;    // Текущий инструмент
        uint8_t note;       // Текущая нота
        uint8_t volume;     // Канальная громкость (CC7)
        uint8_t pan;        // Панорама (CC10)
    } channelMap[16];       // 16 MIDI-каналов

    int allocateOPLChannel(int midiChannel);
    void controlChange(int midiChannel, uint8_t controller, uint8_t value);
    void programChange(int midiChannel, uint8_t program);
};
#endif