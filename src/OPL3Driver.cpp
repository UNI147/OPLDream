#include "OPL3Driver.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cmath>

// Используем extern для доступа к OPL3_CLOCK из OPL3Emulator.cpp
extern const double OPL3_CLOCK;

uint8_t getOperatorOffset(uint8_t channel, uint8_t operatorNum) {
    static const uint8_t operatorOffsets[9][2] = {
        {0x00, 0x03}, {0x01, 0x04}, {0x02, 0x05},
        {0x08, 0x0B}, {0x09, 0x0C}, {0x0A, 0x0D},
        {0x10, 0x13}, {0x11, 0x14}, {0x12, 0x15}
    };
    
    if (channel < 9 && operatorNum < 2) {
        return operatorOffsets[channel][operatorNum];
    }
    return 0;
}

OPL3Driver::OPL3Driver(OPL3Emulator& emu) : emulator(emu) {
    for (int i = 0; i < 16; i++) {
        channelMap[i].oplChannel = i % 18;
        channelMap[i].volume = 100;
        channelMap[i].pan = 64;
        currentPresets[i] = *OPL3PresetLibrary::getPreset(0);
    }
}

void OPL3Driver::handleEvent(const MidiEvent& event) {
    printf("MIDI Event: time=%.2f, ch=%d, type=%d, data1=%d, data2=%d\n",
           event.time, event.channel, event.type, event.data1, event.data2);
    
    switch (event.type) {
        case NOTE_ON:
            noteOn(event.channel, event.data1, event.data2);
            break;
        case NOTE_OFF:
            noteOff(event.channel, event.data1);
            break;
        case CONTROL_CHANGE:
            controlChange(event.channel, event.data1, event.data2);
            break;
        case PROGRAM_CHANGE:
            programChange(event.channel, event.data1);
            break;
        default:
            break;
    }
}

void OPL3Driver::noteOn(int midiChannel, uint8_t note, uint8_t velocity) {
    int oplChannel = channelMap[midiChannel].oplChannel;
    
    printf("Note On: MIDI ch=%d -> OPL ch=%d, note=%d, velocity=%d\n", 
           midiChannel, oplChannel, note, velocity);
    
    // Конвертируем MIDI ноту в частоту OPL3
    double freq = 8.176 * pow(2.0, note / 12.0); // C0 = 8.176 Hz
    
    // Улучшенная конвертация в F-Number и Block
    uint8_t block = 0;
    uint16_t fnum = 0;
    
    // Ищем подходящий block (fnum должен быть < 1024)
    for (block = 0; block < 8; block++) {
        fnum = static_cast<uint16_t>(freq * (1 << (20 - block)) / (OPL3_CLOCK / 72.0));
        if (fnum < 1024) break;
    }
    
    // Ограничиваем максимальные значения
    if (fnum >= 1024) {
        fnum = 1023;
        block = 7;
    }
    
    printf("  Frequency: %.2f Hz -> fnum=%d, block=%d\n", freq, fnum, block);
    
    // Устанавливаем частоту и включаем ноту
    emulator.channels[oplChannel].setFrequency(fnum, block);
    
    // Устанавливаем громкость на основе velocity
    uint8_t volume = static_cast<uint8_t>(63 - (velocity / 2)); // Исправлено предупреждение
    emulator.channels[oplChannel].op1.outputLevel = volume;
    emulator.channels[oplChannel].op2.outputLevel = volume;
    
    // Устанавливаем быструю атаку и медленный релиз
    emulator.channels[oplChannel].op1.attackRate = 15;
    emulator.channels[oplChannel].op2.attackRate = 15;
    emulator.channels[oplChannel].op1.releaseRate = 5;
    emulator.channels[oplChannel].op2.releaseRate = 5;
    
    emulator.channels[oplChannel].setKeyOn(true);
    
    channelMap[midiChannel].note = note;
}

void OPL3Driver::noteOff(int midiChannel, uint8_t note) {
    (void)note;
    
    int oplChannel = channelMap[midiChannel].oplChannel;
    emulator.channels[oplChannel].setKeyOn(false);
}

void OPL3Driver::controlChange(int midiChannel, uint8_t controller, uint8_t value) {
    printf("Control Change: ch=%d, controller=%d, value=%d\n", 
           midiChannel, controller, value);
}

void OPL3Driver::programChange(int midiChannel, uint8_t program) {
    printf("Program Change: ch=%d, program=%d\n", midiChannel, program);
    loadGMInstrument(static_cast<uint8_t>(midiChannel), program);
}

int OPL3Driver::allocateOPLChannel(int midiChannel) {
    return midiChannel % 18;
}

void OPL3Driver::loadPatch(uint8_t midiChannel, const OPL3Patch& patch) {
    int oplChannel = allocateOPLChannel(midiChannel);
    printf("Loading patch on MIDI ch=%d -> OPL ch=%d\n", midiChannel, oplChannel);
    
    // Убедимся, что канал в 2-OP режиме
    uint8_t baseReg = (oplChannel < 9) ? 0x00 : 0x100;
    uint8_t ch = static_cast<uint8_t>(oplChannel % 9);
    if (oplChannel < 6) {
        // Для каналов 0-5 можно отключить 4-OP режим, но в эмуляции пока не реализовано
    }

    // Регистры для оператора 1 и 2
    uint8_t op1_offset = getOperatorOffset(ch, 0);
    uint8_t op2_offset = getOperatorOffset(ch, 1);

    std::cout << "Writing OPL3 registers for channel " << oplChannel 
              << " (base=" << static_cast<int>(baseReg) << ", op1=" << static_cast<int>(op1_offset) 
              << ", op2=" << static_cast<int>(op2_offset) << ")" << std::endl;

    // Записываем параметры операторов
    emulator.writeRegister(baseReg + 0x20 + op1_offset, patch.trem_vib_sus_ksr_multi[0]);
    emulator.writeRegister(baseReg + 0x40 + op1_offset, patch.ksl_outputLevel[0]);
    emulator.writeRegister(baseReg + 0x60 + op1_offset, patch.attackDecay[0]);
    emulator.writeRegister(baseReg + 0x80 + op1_offset, patch.sustainRelease[0]);
    emulator.writeRegister(baseReg + 0xE0 + op1_offset, patch.waveform[0]);

    emulator.writeRegister(baseReg + 0x20 + op2_offset, patch.trem_vib_sus_ksr_multi[1]);
    emulator.writeRegister(baseReg + 0x40 + op2_offset, patch.ksl_outputLevel[1]);
    emulator.writeRegister(baseReg + 0x60 + op2_offset, patch.attackDecay[1]);
    emulator.writeRegister(baseReg + 0x80 + op2_offset, patch.sustainRelease[1]);
    emulator.writeRegister(baseReg + 0xE0 + op2_offset, patch.waveform[1]);

    // Регистр Cx: Feedback/Synth Type + стерео
    uint8_t cvalue = static_cast<uint8_t>((patch.feedback << 1) | (patch.synthType & 1));
    cvalue = static_cast<uint8_t>(cvalue | 0x30); // Включаем оба канала (левый и правый)
    emulator.writeRegister(baseReg + 0xC0 + ch, cvalue);
    
    std::cout << "Patch loaded successfully" << std::endl;
}

void OPL3Driver::loadGMInstrument(uint8_t midiChannel, uint8_t gmProgram) {
    if (gmProgram < OPL3PresetLibrary::getPresetCount()) {
        const OPL3Preset* preset = OPL3PresetLibrary::getPreset(gmProgram);
        loadCustomInstrument(midiChannel, *preset);
    }
}

void OPL3Driver::loadCustomInstrument(uint8_t midiChannel, const OPL3Preset& preset) {
    currentPresets[midiChannel] = preset;
    
    // Конвертируем OPL3Preset в OPL3Patch
    OPL3Patch patch;
    memcpy(patch.trem_vib_sus_ksr_multi, preset.trem_vib_sus_ksr_multi, 2);
    memcpy(patch.ksl_outputLevel, preset.ksl_outputLevel, 2);
    memcpy(patch.attackDecay, preset.attackDecay, 2);
    memcpy(patch.sustainRelease, preset.sustainRelease, 2);
    memcpy(patch.waveform, preset.waveform, 2);
    patch.feedback = preset.feedback;
    patch.synthType = preset.synthType;
    
    loadPatch(midiChannel, patch);
    
    printf("Loaded preset '%s' on channel %d\n", preset.name, midiChannel);
}