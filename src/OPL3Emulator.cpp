#include "OPL3Emulator.h"
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Таблицы скоростей огибающей
static const int attackRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
static const int decayRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
static const int releaseRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};

OPL3Operator::OPL3Operator() {
    reset();
    sampleRate = 44100.0;
    modulator = nullptr;
    feedbackLevel = 0.0f;
}

void OPL3Operator::reset() {
    phase = 0;
    phaseStep = 0;
    keyOn = false;
    envStage = 3; // Release
    envLevel = 0;
    
    // Инициализация параметров по умолчанию
    tremolo = 0;
    vibrato = 0;
    sustain = 0;
    ksr = 0;
    multi = 1;
    ksl = 0;
    outputLevel = 0;
    attackRate = 0;
    decayRate = 0;
    sustainLevel = 0;
    releaseRate = 0;
    waveform = 0;
}

void OPL3Operator::setFrequency(uint16_t frequencyNum, uint8_t blockNum) {
    // F-Number = Music Frequency * 2^(20-Block) / 49716 Hz
    double freq = 49716.0 * frequencyNum / (1 << (20 - blockNum));
    phaseStep = (uint32_t)(freq * (1 << 20) / sampleRate);
    fnum = frequencyNum; // сохраняем в член класса
}

void OPL3Operator::setKeyOn(bool on) {
    keyOn = on;
    if (on) {
        envStage = 0; // Attack
        envLevel = 0;
    } else {
        envStage = 3; // Release
    }
}

void OPL3Operator::updateEnvelope() {
    switch (envStage) {
        case 0: // Attack
            envLevel += attackRates[attackRate];
            if (envLevel >= 0x3FF) {
                envLevel = 0x3FF;
                envStage = 1;
            }
            break;
        case 1: // Decay  
            envLevel -= decayRates[decayRate];
            if (envLevel <= sustainLevel << 6) {
                envLevel = sustainLevel << 6;
                envStage = 2;
            }
            break;
        case 2: // Sustain
            // Держим уровень
            break;
        case 3: // Release
            envLevel -= releaseRates[releaseRate];
            if (envLevel < 0) envLevel = 0;
            break;
    }
}

int16_t OPL3Operator::getSample() {
    if (!keyOn && envStage == 3 && envLevel <= 0) {
        return 0;
    }
    
    // Настоящий FM синтез
    double phaseMod = 0.0;
    if (modulator) {
        phaseMod = modulator->getOutput() * feedbackLevel;
    }
    
    double angle = ((phase + phaseMod) * 2.0 * M_PI) / (1 << 20);
    int16_t sample = (int16_t)(sin(angle) * envLevel);
    
    phase += phaseStep;
    if (phase >= (1 << 20)) phase -= (1 << 20);
    
    updateEnvelope();
    return sample;
}

double OPL3Operator::getOutput() {
    if (!keyOn && envStage == 3 && envLevel <= 0) {
        return 0.0;
    }
    
    double angle = (phase * 2.0 * M_PI) / (1 << 20);
    double sample = sin(angle) * (envLevel / 1024.0);
    
    return sample;
}

// OPL3Channel методы
OPL3Channel::OPL3Channel() {
    reset();
}

void OPL3Channel::reset() {
    op1.reset();
    op2.reset();
    feedback = 0;
    synthType = 0;
    left = true;
    right = true;
    
    // Настраиваем операторы для FM синтеза
    op1.modulator = nullptr;
    op2.modulator = &op1;
    op2.feedbackLevel = feedback / 7.0f;
}

void OPL3Channel::setFrequency(uint16_t frequencyNum, uint8_t blockNum) {
    op1.setFrequency(frequencyNum, blockNum);
    op2.setFrequency(frequencyNum, blockNum);
    fnum = frequencyNum; // сохраняем в член класса
}

void OPL3Channel::setKeyOn(bool on) {
    op1.setKeyOn(on);
    op2.setKeyOn(on);
}

int16_t OPL3Channel::getSample() {
    int16_t sample1 = op1.getSample();
    int16_t sample2 = op2.getSample();
    
    // Простое сложение для тестирования
    int32_t mixed = sample1 + sample2;
    
    // Ограничение
    if (mixed > 32767) mixed = 32767;
    if (mixed < -32768) mixed = -32768;
    
    return (int16_t)mixed;
}

// OPL3Emulator методы
OPL3Emulator::OPL3Emulator() : sampleRate(44100.0) {
    reset();
}

void OPL3Emulator::reset() {
    for (int i = 0; i < 18; i++) {
        channels[i].reset();
    }
}

// Вспомогательная функция для получения оператора
OPL3Operator& OPL3Emulator::getOperator(int index) {
    // Простая маппинг - каждый канал имеет 2 оператора
    int channel = index / 2;
    int opInChannel = index % 2;
    
    return opInChannel == 0 ? channels[channel].op1 : channels[channel].op2;
}

void OPL3Emulator::writeRegister(uint16_t reg, uint8_t value) {
    printf("Write reg: 0x%03X = 0x%02X\n", reg, value);
    
    // Обработка основных регистров
    if (reg >= 0x20 && reg <= 0x35) {
        // Tremolo/Vibrato/Sustain/KSR/Multiplication
        int opIndex = (reg - 0x20) % 32;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.tremolo = (value >> 7) & 1;
            op.vibrato = (value >> 6) & 1;
            op.sustain = (value >> 5) & 1;
            op.ksr = (value >> 4) & 1;
            op.multi = value & 0x0F;
        }
    }
    else if (reg >= 0x40 && reg <= 0x55) {
        // KSL/Output Level
        int opIndex = (reg - 0x40) % 32;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.ksl = (value >> 6) & 3;
            op.outputLevel = value & 0x3F;
        }
    }
    else if (reg >= 0x60 && reg <= 0x75) {
        // Attack/Decay
        int opIndex = (reg - 0x60) % 32;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.attackRate = (value >> 4) & 0x0F;
            op.decayRate = value & 0x0F;
        }
    }
    else if (reg >= 0x80 && reg <= 0x95) {
        // Sustain/Release
        int opIndex = (reg - 0x80) % 32;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.sustainLevel = (value >> 4) & 0x0F;
            op.releaseRate = value & 0x0F;
        }
    }
    else if (reg >= 0xA0 && reg <= 0xA8) {
        // Frequency (low)
        int ch = reg - 0xA0;
        if (ch < 9) {
            // Сохраняем fnum для использования в B0-B8
            channels[ch].fnum = (channels[ch].fnum & 0x300) | value;
        }
    }
    else if (reg >= 0xB0 && reg <= 0xB8) {
        // Key On/Block/Frequency (high)
        int ch = reg - 0xB0;
        if (ch < 9) {
            bool keyOn = (value >> 5) & 1;
            uint8_t block = (value >> 2) & 7;
            channels[ch].fnum = (channels[ch].fnum & 0xFF) | ((value & 3) << 8);
            
            channels[ch].setKeyOn(keyOn);
            channels[ch].setFrequency(channels[ch].fnum, block);
        }
    }
    else if (reg == 0xBD) {
        // Rhythm/percussion
        // TODO: Implement percussion
    }
}

void OPL3Emulator::render(int16_t* buffer, int samples) {
    for (int i = 0; i < samples; i += 2) {
        int16_t sample = 0;
        
        // Микшируем все активные каналы
        for (int j = 0; j < 18; j++) {
            if (channels[j].left || channels[j].right) {
                sample += channels[j].getSample();
            }
        }
        
        // Стерео вывод
        buffer[i] = sample;     // Left
        buffer[i + 1] = sample; // Right
    }
}