#include "OPL3Operator.h"
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Улучшенные скорости огибающей - более быстрые значения
static const int attackRates[16] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
static const int decayRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};
static const int releaseRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};

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
    
    // Правильный расчет частоты для OPL3
    double baseFreq = (OPL3_CLOCK * frequencyNum) / (1 << 19);
    
    // Применяем множитель частоты
    double mult_factor;
    if (multi == 0) mult_factor = 0.5;
    else mult_factor = multi;
    
    double actualFreq = baseFreq * mult_factor;
    
    // Применяем блок (октаву)
    actualFreq *= (1 << blockNum);
    
    // Рассчитываем шаг фазы
    phaseStep = static_cast<uint32_t>(actualFreq * (1 << 20) / sampleRate);
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
    int rate = 0;
    
    switch (envStage) {
        case 0: // Attack
            rate = attackRates[attackRate];
            if (rate > 0) {
                envLevel += rate;
                if (envLevel >= targetLevel) {
                    envLevel = targetLevel;
                    envStage = 1; // Decay
                    targetLevel = sustainLevel * 64; // sustainLevel 0-15 -> 0-960
                }
            } else {
                envLevel = targetLevel;
                envStage = 1;
            }
            break;
            
        case 1: // Decay  
            rate = decayRates[decayRate];
            if (rate > 0) {
                envLevel -= rate;
                if (envLevel <= targetLevel) {
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
            
        case 3: // Release
            rate = releaseRates[releaseRate];
            if (rate > 0) {
                envLevel -= rate;
                if (envLevel < 0) envLevel = 0;
            } else {
                envLevel = 0;
            }
            break;
    }
}

double OPL3Operator::getOutput() {
    // Если не нажата клавиша или огибающая на минимуме - выход 0
    if (!keyOn && envLevel <= 0) {
        return 0.0;
    }
    
    // Обновляем огибающую каждый семпл
    updateEnvelope();
    
    // Если огибающая на минимуме после обновления
    if (envLevel <= 0) {
        return 0.0;
    }
    
    // Генерируем волну (синус)
    double normalizedPhase = std::fmod(static_cast<double>(phase), 1048576.0) / 1048576.0;
    double sample = sin(normalizedPhase * 2.0 * M_PI);
    
    // Применяем огибающую (инвертированную: 0=макс, 1023=мин)
    double amplitude = (1023 - envLevel) / 1023.0;
    
    // Применяем уровень вывода (0=макс, 63=мин)
    amplitude *= (63 - outputLevel) / 63.0;
    
    // Увеличиваем общую громкость
    amplitude *= 4.0; // Увеличиваем амплитуду в 4 раза
    
    // Обновляем фазу для следующего семпла
    phase += phaseStep;
    if (phase >= 1048576) {
        phase -= 1048576;
    }
    
    return sample * amplitude;
}

int16_t OPL3Operator::getSample() {
    double output = getOutput();
    return static_cast<int16_t>(output * 16384.0); // Увеличиваем громкость
}