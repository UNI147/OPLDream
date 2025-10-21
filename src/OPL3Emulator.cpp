#include "OPL3Emulator.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

const double OPL3_CLOCK = 14318180.0 / 288.0; // ~49716 Hz

// Улучшенные скорости огибающей - более быстрые значения
static const int attackRates[16] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
static const int decayRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};
static const int releaseRates[16] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};

double OPL3Operator::getWaveformOutput(double phase) {
    switch(waveform & 0x07) { // Используем только младшие 3 бита
        case 0: return std::sin(phase * 2.0 * M_PI); // Sine
        case 1: return std::fabs(std::sin(phase * 2.0 * M_PI)); // Half-sine
        case 2: return std::fabs(std::sin(phase * 2.0 * M_PI)) * 2.0 - 1.0; // Absolute sine
        case 3: { // Pulse-sine
            double sine = std::sin(phase * 2.0 * M_PI);
            return sine > 0 ? 1.0 : -1.0;
        }
        case 4: // Sine - even periods only
            return std::sin(phase * M_PI); // Полупериод
        case 5: // Abs-Sine - even periods only  
            return std::fabs(std::sin(phase * M_PI)) * 2.0 - 1.0;
        case 6: // Square
            return std::sin(phase * 2.0 * M_PI) > 0 ? 1.0 : -1.0;
        case 7: // Derived Square
            return std::sin(phase * 4.0 * M_PI) > 0 ? 1.0 : -1.0;
        default: return std::sin(phase * 2.0 * M_PI);
    }
}

int OPL3Operator::getCurrentRate() {
    int rate = 0;
    switch(envStage) {
        case 0: rate = attackRates[attackRate]; break;
        case 1: rate = decayRates[decayRate]; break;
        case 3: rate = releaseRates[releaseRate]; break;
        default: rate = 0;
    }
    
    return rate * 2;
}

OPL3Operator::OPL3Operator() : lfo(6.0, OPL3_CLOCK) {
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
    
    // Инициализация параметров по умолчанию
    tremolo = 0;
    vibrato = 0;
    sustain = 1;
    ksr = 0;
    multi = 1;
    ksl = 0;
    outputLevel = 0;
    attackRate = 15;
    decayRate = 0;
    sustainLevel = 15;
    releaseRate = 0;
    waveform = 0;
    fnum = 0;
    block = 0;
}

void OPL3Operator::setFrequency(uint16_t frequencyNum, uint8_t blockNum) {
    fnum = frequencyNum;
    block = blockNum;
    
    // Более точный расчет согласно документации
    double baseFreq = (OPL3_CLOCK * frequencyNum) / (1 << 19);
    
    // Применяем множитель с учетом специальных случаев
    double mult_factor;
    switch(multi) {
        case 0: mult_factor = 0.5; break;
        case 1: mult_factor = 1.0; break;
        case 2: mult_factor = 2.0; break;
        case 3: mult_factor = 3.0; break;
        case 4: mult_factor = 4.0; break;
        case 5: mult_factor = 5.0; break;
        case 6: mult_factor = 6.0; break;
        case 7: mult_factor = 7.0; break;
        case 8: mult_factor = 8.0; break;
        case 9: mult_factor = 9.0; break;
        case 10: mult_factor = 10.0; break;
        case 11: mult_factor = 10.0; break; // Специальный случай
        case 12: mult_factor = 12.0; break;
        case 13: mult_factor = 12.0; break; // Специальный случай
        case 14: mult_factor = 15.0; break;
        case 15: mult_factor = 15.0; break; // Специальный случай
        default: mult_factor = multi;
    }
    
    double actualFreq = baseFreq * mult_factor * (1 << blockNum);
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
    int rate = getCurrentRate();
    
    switch(envStage) {
        case 0: // Attack - СВЕРХМЕДЛЕННАЯ
            if (rate > 0) {
                envLevel += rate / 2; // Еще больше замедляем атаку
                if (envLevel >= targetLevel) {
                    envLevel = targetLevel;
                    envStage = 1; // Decay
                    targetLevel = sustainLevel * 64;
                }
            } else {
                envLevel = targetLevel;
                envStage = 1;
            }
            break;
            
        case 1: // Decay - СВЕРХМЕДЛЕННАЯ  
            if (rate > 0) {
                envLevel -= rate / 4; // Еще больше замедляем спад
                if (envLevel <= targetLevel) {
                    envLevel = targetLevel;
                    envStage = 2; // Sustain
                }
            } else {
                envLevel = targetLevel;
                envStage = 2;
            }
            break;
            
        case 2: // Sustain
            // Ничего не делаем, ждем KEY_OFF
            break;
            
        case 3: // Release - СВЕРХМЕДЛЕННАЯ
            if (rate > 0) {
                envLevel -= rate / 6; // Еще больше замедляем релиз
                if (envLevel < 0) envLevel = 0;
            } else {
                envLevel = 0;
            }
            break;
    }
    
    // Отладочный вывод для отслеживания огибающей
    static int debugCounter = 0;
    if (debugCounter++ % 500 == 0 && keyOn) {
        printf("Envelope: stage=%d, level=%d/%d, rate=%d\n", 
               envStage, envLevel, targetLevel, rate);
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
    
    double modPhase = static_cast<double>(phase);
    
    // Применяем вибрато (если включено)
    if(vibrato) {
        modPhase += lfo.getValue() * 512.0; // Увеличиваем глубину вибрато
    }
    
    double normalizedPhase = std::fmod(modPhase, 1048576.0) / 1048576.0;
    double sample = getWaveformOutput(normalizedPhase);
    
    // Применяем тремоло (если включено)
    double amplitude = (1023 - envLevel) / 1023.0;
    if(tremolo) {
        amplitude *= 1.0 + lfo.getValue() * 0.5; // Увеличиваем глубину тремоло
    }
    
    // МАКСИМАЛЬНАЯ АМПЛИТУДА - минимальное влияние outputLevel
    amplitude *= (63 - outputLevel) / 20.0; // Еще больше уменьшаем делитель
    
    // МАКСИМАЛЬНОЕ УСИЛЕНИЕ
    amplitude *= 5.0; // Увеличиваем в 5 раз (было 3)
    
    // Применяем обратную связь (если есть)
    if (modulator == this && feedbackLevel > 0.0f) {
        sample += sample * feedbackLevel * 2.0f; // Усиливаем обратную связь
    }
    
    // Обновляем фазу для следующего семпла
    phase += phaseStep;
    if (phase >= 1048576) {
        phase -= 1048576;
    }
    
    return sample * amplitude;
}

int16_t OPL3Operator::getSample() {
    double output = getOutput();
    // МАКСИМАЛЬНАЯ ГРОМКОСТЬ ВЫХОДА
    return static_cast<int16_t>(output * 32767.0); // Максимально возможное значение для int16_t
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
    double modulatorOutput = op1.getOutput();
    
    // Применяем модуляцию к фазе carrier с учетом обратной связи
    double modAmount = modulatorOutput * 2048.0 * (feedback / 7.0f);
    
    // Сохраняем оригинальную фазу
    uint32_t originalPhase = op2.phase;
    
    // Применяем модуляцию к фазе
    double modulatedPhase = originalPhase + modAmount;
    modulatedPhase = fmod(modulatedPhase, 1048576.0);
    
    // Временно устанавливаем модулированную фазу для carrier
    op2.phase = static_cast<uint32_t>(modulatedPhase);
    
    // Получаем выход carrier с модуляцией
    double carrierOutput = op2.getOutput();
    
    // Восстанавливаем фазу carrier
    op2.phase = originalPhase;
    
    // Применяем обратную связь к модулятору (если нужно)
    if (feedback > 0) {
        // Обновляем фазу модулятора с обратной связью
        double feedbackPhase = op1.phase + carrierOutput * 512.0 * (feedback / 7.0f);
        op1.phase = static_cast<uint32_t>(fmod(feedbackPhase, 1048576.0));
    }
    
    // Конвертируем в 16-бит
    int32_t result = static_cast<int32_t>(carrierOutput * 8192.0); // Уменьшаем амплитуду для предотвращения клиппинга
    
    // Ограничение
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    
    return static_cast<int16_t>(result);
}

// OPL3Emulator методы
OPL3Emulator::OPL3Emulator() : sampleRate(OPL3_CLOCK) {
    reset();
}

void OPL3Emulator::reset() {
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

void OPL3Emulator::enable4OPMode(int channelPair) {
    // Заглушка для 4-OP режима - будет реализовано позже
    std::cout << "4-OP mode enabled for channel pair " << channelPair << std::endl;
}

void OPL3Emulator::updatePercussion(uint8_t percussionBits) {
    // Заглушка для режима перкуссии - будет реализовано позже
    std::cout << "Percussion update: " << static_cast<int>(percussionBits) << std::endl;
}

OPL3Operator& OPL3Emulator::getOperator(int index) {
    static OPL3Operator dummy;
    if (index < 0 || index >= 36) return dummy;
    
    int channel = index / 2;
    int opInChannel = index % 2;
    
    return opInChannel == 0 ? channels[channel].op1 : channels[channel].op2;
}

void OPL3Emulator::writeRegister(uint16_t reg, uint8_t value) {
    // Обработка регистра 0x104 - включение 4-OP режимов
    if(reg == 0x104) {
        for(int i = 0; i < 6; i++) {
            if(value & (1 << i)) {
                enable4OPMode(i);
            }
        }
        return;
    }
    
    // Обработка регистра 0xBD - режим перкуссии
    if(reg == 0xBD) {
        rhythmMode = (value & 0x20) != 0;
        updatePercussion(value & 0x1F);
        return;
    }
    
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
                    
                    if (std::abs(sample) > maxSample) maxSample = std::abs(sample);
                }
            }
        }
        
        // МАКСИМАЛЬНОЕ УСИЛЕНИЕ МИКШИРОВАНИЯ
        if (activeChannels > 0) {
            // Максимальное усиление без клиппинга
            leftMixed = leftMixed * 4; // Увеличиваем в 4 раза (было 3)
            rightMixed = rightMixed * 4;
        }
        
        // Ограничение (на всякий случай)
        if (leftMixed > 32767) leftMixed = 32767;
        if (leftMixed < -32768) leftMixed = -32768;
        if (rightMixed > 32767) rightMixed = 32767;
        if (rightMixed < -32768) rightMixed = -32768;
        
        // Стерео вывод
        buffer[i] = static_cast<int16_t>(leftMixed);
        buffer[i + 1] = static_cast<int16_t>(rightMixed);
    }
    
    // Отладочный вывод громкости
    if (debugCounter++ % 50 == 0) {
        printf("Render: %d active channels, max sample amplitude=%d\n", 
               activeChannelsThisFrame, maxSample);
    }
}