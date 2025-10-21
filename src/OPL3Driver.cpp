#include "OPL3Driver.h"
#include <cstdio>

uint8_t getOperatorOffset(uint8_t channel, uint8_t operatorNum) {
    // Таблица смещений операторов для OPL3
    static const uint8_t operatorOffsets[9][2] = {
        {0x00, 0x03}, // Channel 0: Op1=0x00, Op2=0x03
        {0x01, 0x04}, // Channel 1: Op1=0x01, Op2=0x04
        {0x02, 0x05}, // Channel 2: Op1=0x02, Op2=0x05
        {0x08, 0x0B}, // Channel 3: Op1=0x08, Op2=0x0B
        {0x09, 0x0C}, // Channel 4: Op1=0x09, Op2=0x0C
        {0x0A, 0x0D}, // Channel 5: Op1=0x0A, Op2=0x0D
        {0x10, 0x13}, // Channel 6: Op1=0x10, Op2=0x13
        {0x11, 0x14}, // Channel 7: Op1=0x11, Op2=0x14
        {0x12, 0x15}  // Channel 8: Op1=0x12, Op2=0x15
    };
    
    if (channel < 9 && operatorNum < 2) {
        return operatorOffsets[channel][operatorNum];
    }
    return 0;
}

OPL3Driver::OPL3Driver(OPL3Emulator& emu) : emulator(emu) {
    // Инициализация каналов
    for (int i = 0; i < 16; i++) {
        channelMap[i].oplChannel = i % 18; // Простое распределение
        channelMap[i].volume = 100;
        channelMap[i].pan = 64;
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
    (void)velocity; // Помечаем неиспользуемый параметр
    
    int oplChannel = channelMap[midiChannel].oplChannel;
    
    // Простая установка частоты
    uint16_t fnum = 100 + note * 50; // Упрощенный расчет
    uint8_t block = static_cast<uint8_t>(note / 12);
    
    emulator.channels[oplChannel].setFrequency(fnum, block);
    emulator.channels[oplChannel].setKeyOn(true);
    
    channelMap[midiChannel].note = note;
}

void OPL3Driver::noteOff(int midiChannel, uint8_t note) {
    (void)note; // Помечаем неиспользуемый параметр
    
    int oplChannel = channelMap[midiChannel].oplChannel;
    emulator.channels[oplChannel].setKeyOn(false);
}

void OPL3Driver::controlChange(int midiChannel, uint8_t controller, uint8_t value) {
    printf("Control Change: ch=%d, controller=%d, value=%d\n", 
           midiChannel, controller, value);
}

void OPL3Driver::programChange(int midiChannel, uint8_t program) {
    printf("Program Change: ch=%d, program=%d\n", midiChannel, program);
}

int OPL3Driver::allocateOPLChannel(int midiChannel) {
    return midiChannel % 18; // Простое распределение
}

void OPL3Driver::loadPatch(uint8_t midiChannel, const OPL3Patch& patch) {
    int oplChannel = allocateOPLChannel(midiChannel);
    channelMap[midiChannel].patch = patch;
    channelMap[midiChannel].oplChannel = oplChannel;

    // Записываем параметры патча в регистры OPL3
    uint8_t baseReg = (oplChannel < 9) ? 0x00 : 0x100;
    uint8_t ch = static_cast<uint8_t>(oplChannel % 9);

    // Регистры для оператора 1 и 2
    uint8_t op1_offset = getOperatorOffset(ch, 0);
    uint8_t op2_offset = getOperatorOffset(ch, 1);

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

    // Регистр Cx: Feedback/Synth Type
    uint8_t cvalue = static_cast<uint8_t>((patch.feedback << 1) | patch.synthType);
    emulator.writeRegister(baseReg + 0xC0 + ch, cvalue);
}