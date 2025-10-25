#include "OPL3Operator.h"
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Улучшенные скорости огибающей - более реалистичные значения
static const double attackRates[16] = {
    0, 0.003, 0.006, 0.012, 0.024, 0.048, 0.072, 0.114,
    0.168, 0.240, 0.300, 0.378, 0.462, 0.558, 0.672, 0.798
};

static const double decayReleaseRates[16] = {
    0, 0.002, 0.004, 0.008, 0.016, 0.032, 0.048, 0.080,
    0.126, 0.192, 0.282, 0.392, 0.522, 0.672, 0.882, 1.140
};

OPL3Operator::OPL3Operator() {
    reset();
    sampleRate = OPL3_CLOCK;
    modulator = nullptr;
    feedbackLevel = 0.0f;
}

void OPL3Operator::reset() {
    phase = 0;
    phaseStep = 0;
    keyOn = false;
    envStage = 3; // Release
    envLevel = 0;
    targetLevel = 0;
    
    // Инициализация параметров по умолчанию - более громкие настройки
    tremolo = 0;
    vibrato = 0;
    sustain = 1; // Включаем сустейн по умолчанию
    ksr = 0;
    multi = 1;
    ksl = 0;
    outputLevel = 0; // Максимальная громкость
    attackRate = 15; // Быстрая атака
    decayRate = 0;   // Медленный спад
    sustainLevel = 15; // Максимальный уровень сустейна
    releaseRate = 0;  // Медленный релиз
    waveform = 0;
    fnum = 0;
    block = 0;
}

void OPL3Operator::setFrequency(uint16_t frequencyNum, uint8_t blockNum) {
    fnum = frequencyNum;
    block = blockNum;
    
    // ПРАВИЛЬНЫЙ расчет согласно документации OPL3 (стр. 10)
    // F-Number = f * 2^(20-block) / 49716
    // => f = (F-Number * 49716) / 2^(20-block)
    
    double baseFreq = (frequencyNum * OPL3_CLOCK) / (1 << (20 - blockNum));
    
    // Применяем множитель частоты согласно таблице из документации (стр. 8-9)
    double mult_factor;
    static const double multi_table[16] = {
        0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15
    };
    mult_factor = multi_table[multi & 0x0F];
    
    double actualFreq = baseFreq * mult_factor;
    
    // Рассчитываем шаг фазы для генератора с 20-битной фазой (1<<20 = 1048576)
    phaseStep = static_cast<uint32_t>((actualFreq * 1048576.0) / sampleRate);
    
    // Отладочный вывод для проверки
    if (frequencyNum > 0 && blockNum > 0 && actualFreq > 100) {
        printf("Freq: fnum=%d, block=%d, multi=%d -> base=%.1fHz, actual=%.1fHz, step=%u\n", 
               frequencyNum, blockNum, multi, baseFreq, actualFreq, phaseStep);
    }
}

void OPL3Operator::setKeyOn(bool on) {
    if (keyOn == on) return;
    
    keyOn = on;
    if (on) {
        envStage = 0; // Attack
        envLevel = 0;
        targetLevel = 1023;
        phase = 0; // Сбрасываем фазу при новом нажатии
    } else {
        envStage = 3; // Release
        targetLevel = 0;
    }
}

void OPL3Operator::updateEnvelope() {
    double rate = 0.0;
    
    switch (envStage) {
        case 0: // Attack (экспоненциальный рост)
            rate = attackRates[attackRate];
            if (rate > 0) {
                envLevel = static_cast<int>(envLevel + (1024.0 - envLevel) * rate);
                if (envLevel >= 1000) { // Практически достигли максимума
                    envLevel = 1023;
                    envStage = 1; // Decay
                    targetLevel = static_cast<int>(sustainLevel * 68.2); // 0-15 -> 0-1023
                }
            } else {
                envLevel = 1023;
                envStage = 1;
            }
            break;
            
        case 1: // Decay (экспоненциальный спад)
            rate = decayReleaseRates[decayRate];
            if (rate > 0) {
                envLevel = static_cast<int>(envLevel - (envLevel - targetLevel) * rate);
                if (envLevel <= targetLevel + 1) {
                    envLevel = targetLevel;
                    envStage = 2; // Sustain
                }
            } else {
                envLevel = targetLevel;
                envStage = 2;
            }
            break;
            
        case 2: // Sustain - ничего не делаем, ждем KEY_OFF
            break;
            
        case 3: // Release (экспоненциальный спад)
            rate = decayReleaseRates[releaseRate];
            if (rate > 0) {
                envLevel = static_cast<int>(envLevel - envLevel * rate); // Экспоненциальный спад к 0
                if (envLevel < 1) envLevel = 0;
            } else {
                envLevel = 0;
            }
            break;
    }
}

double OPL3Operator::getOutput() {
    if (!keyOn && envLevel <= 0) {
        return 0.0;
    }
    
    updateEnvelope();
    
    if (envLevel <= 0) {
        return 0.0;
    }
    
    // Генерируем волну
    double normalizedPhase = std::fmod(static_cast<double>(phase), 1048576.0) / 1048576.0;
    double sample = sin(normalizedPhase * 2.0 * M_PI);
    
    // Экспоненциальная амплитудная модуляция (ближе к реальному OPL3)
    double amplitude = std::exp((1023.0 - envLevel) / 256.0) / 20.0;
    
    // Применяем уровень вывода
    amplitude *= (64.0 - outputLevel) / 64.0;
    
    // Обновляем фазу
    phase += phaseStep;
    if (phase >= 1048576) {
        phase -= 1048576;
    }
    
    return sample * amplitude;
}

int16_t OPL3Operator::getSample() {
    double output = getOutput();
    return static_cast<int16_t>(output * 8192.0);
}