#include "OPL3Emulator.h"
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OPL3Operator::OPL3Operator() {
    reset();
}

void OPL3Operator::reset() {
    phase = 0;
    phaseStep = 0;
    keyOn = false;
    envStage = 3; // Release
    envLevel = 0;
}

void OPL3Operator::setFrequency(uint16_t fnum, uint8_t block) {
    // Правильный расчет частоты согласно документации OPL3
    // F-Number = Music Frequency * 2^(20-Block) / 49716 Hz
    uint32_t baseFreq = fnum;
    phaseStep = (baseFreq << block) * 8; // Упрощенный расчет
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
    // Простая огибающая для тестирования
    switch (envStage) {
        case 0: // Attack
            envLevel += 2;
            if (envLevel >= 127) {
                envLevel = 127;
                envStage = 1; // Decay
            }
            break;
        case 1: // Decay
            envLevel -= 1;
            if (envLevel <= 64) {
                envLevel = 64;
                envStage = 2; // Sustain
            }
            break;
        case 2: // Sustain
            // Остаемся на sustain уровне
            break;
        case 3: // Release
            envLevel -= 1;
            if (envLevel < 0) envLevel = 0;
            break;
    }
}

int16_t OPL3Operator::getSample() {
    if (!keyOn && envStage == 3 && envLevel <= 0) {
        return 0;
    }
    
    // Простая синусоида для тестирования
    double angle = (phase * 2.0 * M_PI) / (1 << 20);
    int16_t sample = (int16_t)(sin(angle) * 30000 * envLevel / 127.0);
    
    phase += phaseStep;
    if (phase >= (1 << 20)) phase -= (1 << 20);
    
    // Простая огибающая
    updateEnvelope();
    
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
}

void OPL3Channel::setFrequency(uint16_t fnum, uint8_t block) {
    op1.setFrequency(fnum, block);
    op2.setFrequency(fnum, block);
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
    for (auto& channel : channels) {
        channel.reset();
    }
}

void OPL3Emulator::writeRegister(uint16_t reg, uint8_t value) {
    // Базовая обработка регистров
    // Пока просто логируем
    printf("Write reg: 0x%03X = 0x%02X\n", reg, value);
}

void OPL3Emulator::render(int16_t* buffer, int samples) {
    for (int i = 0; i < samples; i += 2) {
        int16_t sample = 0;
        
        // Микшируем все активные каналы
        for (auto& channel : channels) {
            if (channel.left || channel.right) {
                sample += channel.getSample();
            }
        }
        
        // Стерео вывод
        buffer[i] = sample;     // Left
        buffer[i + 1] = sample; // Right
    }
}