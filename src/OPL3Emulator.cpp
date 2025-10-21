#include "OPL3Emulator.h"
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Добавляем таблицы скоростей огибающей
static const int attackRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
static const int decayRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
static const int releaseRates[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};

OPL3Operator::OPL3Operator() {
    reset();
    sampleRate = 44100.0; // Инициализируем sampleRate
    modulator = nullptr;   // Инициализируем modulator
    feedbackLevel = 0.0f;  // Инициализируем feedbackLevel
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

void OPL3Operator::setFrequency(uint16_t fnum, uint8_t block) {
    // F-Number = Music Frequency * 2^(20-Block) / 49716 Hz
    double freq = 49716.0 * fnum / (1 << (20 - block));
    phaseStep = (uint32_t)(freq * (1 << 20) / sampleRate);
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
    // Реализация согласно регистрам Attack/Decay/Sustain/Release
    switch (envStage) {
        case 0: // Attack
            envLevel += attackRates[attackRate];
            if (envLevel >= 0x3FF) { // 10-bit envelope
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

// Добавляем метод getOutput для использования в FM синтезе
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