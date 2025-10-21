#include "OPL3Emulator.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const double OPL3_CLOCK = 14318180.0 / 288.0; // ~49716 Hz

// Улучшенные скорости огибающей - более быстрые значения
static const int attackRates[16] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
static const int decayRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};
static const int releaseRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};

OPL3Operator::OPL3Operator() {
    reset();
    sampleRate = OPL3_CLOCK; // Используем нативную частоту OPL3
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
    phaseStep = (uint32_t)(actualFreq * (1 << 20) / sampleRate);
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
    double normalizedPhase = fmod(phase, 1048576.0) / 1048576.0;
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
    return (int16_t)(output * 16384.0); // Увеличиваем громкость
}

// OPL3Channel методы
OPL3Channel::OPL3Channel() {
    reset();
}

void OPL3Channel::reset() {
    op1.reset();
    op2.reset();
    feedback = 4; // Средняя обратная связь
    synthType = 0; // FM синтез
    left = true;
    right = true;
    fnum = 0;
    block = 0;
    
    // Настраиваем операторы для FM синтеза
    op1.modulator = nullptr;
    op2.modulator = &op1;
    op1.feedbackLevel = feedback / 7.0f;
    op2.feedbackLevel = 0.0f; // Carrier не имеет обратной связи
}

bool OPL3Channel::isActive() const {
    return op1.isKeyOn() || op2.isKeyOn();
}

void OPL3Channel::setFrequency(uint16_t frequencyNum, uint8_t blockNum) {
    fnum = frequencyNum;
    block = blockNum;
    op1.setFrequency(frequencyNum, blockNum);
    op2.setFrequency(frequencyNum, blockNum);
}

void OPL3Channel::setKeyOn(bool on) {
    op1.setKeyOn(on);
    op2.setKeyOn(on);
}

int16_t OPL3Channel::getSample() {
    if (!isActive()) {
        return 0;
    }
    
    // Для FM синтеза: modulator -> carrier
    // Получаем выход модулятора
    double modulatorOutput = op1.getOutput();
    
    // Временно применяем модуляцию к фазе carrier
    double modAmount = modulatorOutput * 1024.0; // Увеличиваем глубину модуляции
    double modulatedPhase = op2.phase + modAmount;
    
    // Сохраняем оригинальную фазу
    uint32_t originalPhase = op2.phase;
    
    // Временно устанавливаем модулированную фазу
    op2.phase = (uint32_t)fmod(modulatedPhase, 1048576.0);
    
    // Получаем выход carrier с модуляцией
    double carrierOutput = op2.getOutput();
    
    // Восстанавливаем фазу carrier
    op2.phase = originalPhase;
    
    // Конвертируем в 16-бит с большей амплитудой
    int32_t result = (int32_t)(carrierOutput * 16384.0);
    
    // Ограничение
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    
    return (int16_t)result;
}

// OPL3Emulator методы
OPL3Emulator::OPL3Emulator() : sampleRate(OPL3_CLOCK) { // Используем нативную частоту OPL3
    reset();
}

void OPL3Emulator::reset() {
    // Устанавливаем sample rate для всех операторов
    for (int i = 0; i < 18; i++) {
        channels[i].op1.sampleRate = sampleRate;
        channels[i].op2.sampleRate = sampleRate;
        channels[i].reset();
    }
    
    // Включаем OPL3 режим и Waveform Select
    writeRegister(0x105, 0x01); // OPL3 enable
    writeRegister(0x104, 0x00); // Все каналы в 2-OP режиме для начала
    writeRegister(0x01, 0x20);  // Waveform select enable
    
    std::cout << "OPL3 Emulator initialized with native sample rate: " << sampleRate << std::endl;
}

OPL3Operator& OPL3Emulator::getOperator(int index) {
    static OPL3Operator dummy;
    if (index < 0 || index >= 36) return dummy;
    
    int channel = index / 2;
    int opInChannel = index % 2;
    
    return opInChannel == 0 ? channels[channel].op1 : channels[channel].op2;
}

void OPL3Emulator::writeRegister(uint16_t reg, uint8_t value) {
    //printf("Write reg: 0x%03X = 0x%02X\n", reg, value);
    
    // Обработка основных регистров
    if (reg >= 0x20 && reg <= 0x35) {
        int opIndex = reg - 0x20;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.tremolo = (value >> 7) & 1;
            op.vibrato = (value >> 6) & 1;
            op.sustain = (value >> 5) & 1;
            op.ksr = (value >> 4) & 1;
            op.multi = value & 0x0F;
            if (op.multi == 0) op.multi = 1; // Исправляем: MULTI=0 означает 1, а не 0.5
            
            // Обновляем частоту с новыми параметрами
            if (op.fnum > 0) {
                op.setFrequency(op.fnum, op.block);
            }
        }
    }
    else if (reg >= 0x40 && reg <= 0x55) {
        int opIndex = reg - 0x40;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.ksl = (value >> 6) & 3;
            op.outputLevel = value & 0x3F;
        }
    }
    else if (reg >= 0x60 && reg <= 0x75) {
        int opIndex = reg - 0x60;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.attackRate = (value >> 4) & 0x0F;
            op.decayRate = value & 0x0F;
        }
    }
    else if (reg >= 0x80 && reg <= 0x95) {
        int opIndex = reg - 0x80;
        if (opIndex < 18) {
            OPL3Operator& op = getOperator(opIndex);
            op.sustainLevel = (value >> 4) & 0x0F;
            op.releaseRate = value & 0x0F;
        }
    }
    else if (reg >= 0xA0 && reg <= 0xA8) {
        int ch = reg - 0xA0;
        if (ch < 9) {
            channels[ch].fnum = (channels[ch].fnum & 0x300) | value;
            channels[ch].setFrequency(channels[ch].fnum, channels[ch].block);
        }
    }
    else if (reg >= 0xB0 && reg <= 0xB8) {
        int ch = reg - 0xB0;
        if (ch < 9) {
            bool keyOn = (value >> 5) & 1;
            uint8_t block = (value >> 2) & 7;
            channels[ch].block = block;
            channels[ch].fnum = (channels[ch].fnum & 0xFF) | ((value & 3) << 8);
            
            channels[ch].setFrequency(channels[ch].fnum, block);
            channels[ch].setKeyOn(keyOn);
        }
    }
    else if (reg >= 0xC0 && reg <= 0xC8) {
        int ch = reg - 0xC0;
        if (ch < 9) {
            channels[ch].feedback = (value >> 1) & 7;
            channels[ch].synthType = value & 1;
            
            // Обновляем обратную связь
            channels[ch].op1.feedbackLevel = channels[ch].feedback / 7.0f;
        }
    }
    else if (reg == 0xBD) {
        // Rhythm/percussion - пока игнорируем
    }
    else if (reg == 0x01) {
        // Test register / Waveform select - уже обработано в reset()
    }
    else if (reg == 0x105) {
        // OPL3 enable - уже обработано в reset()
    }
}

void OPL3Emulator::render(int16_t* buffer, int samples) {
    memset(buffer, 0, samples * sizeof(int16_t));
    
    static int debugCounter = 0;
    int activeChannelsThisFrame = 0;
    int maxSample = 0;
    
    for (int i = 0; i < samples; i += 2) {
        int32_t leftMixed = 0;
        int32_t rightMixed = 0;
        int activeChannels = 0;
        
        // Микшируем все активные каналы
        for (int j = 0; j < 18; j++) {
            if (channels[j].isActive()) {
                int16_t sample = channels[j].getSample();
                
                if (sample != 0) {
                    if (channels[j].left) {
                        leftMixed += sample;
                    }
                    if (channels[j].right) {
                        rightMixed += sample;
                    }
                    activeChannels++;
                    activeChannelsThisFrame++;
                    
                    if (abs(sample) > maxSample) maxSample = abs(sample);
                }
            }
        }
        
        // Нормализация и ограничение
        if (activeChannels > 0) {
            leftMixed = (leftMixed * 2) / activeChannels; // Увеличиваем громкость
            rightMixed = (rightMixed * 2) / activeChannels;
        }
        
        // Ограничение
        if (leftMixed > 32767) leftMixed = 32767;
        if (leftMixed < -32768) leftMixed = -32768;
        if (rightMixed > 32767) rightMixed = 32767;
        if (rightMixed < -32768) rightMixed = -32768;
        
        // Стерео вывод
        buffer[i] = (int16_t)leftMixed;
        buffer[i + 1] = (int16_t)rightMixed;
    }
    
    // Отладочный вывод
    if (debugCounter++ % 100 == 0) {
        printf("Render: %d active, max sample=%d\n", activeChannelsThisFrame, maxSample);
    }
}