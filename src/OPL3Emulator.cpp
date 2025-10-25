#include "OPL3Emulator.h"
#include <cstring>
#include <iostream>

OPL3Emulator::OPL3Emulator() : sampleRate(OPL3_CLOCK) {
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
        handleOperatorRegister(reg - 0x20, value, &OPL3Operator::tremolo, 7, 
                              &OPL3Operator::vibrato, 6, &OPL3Operator::sustain, 5,
                              &OPL3Operator::ksr, 4, &OPL3Operator::multi, 0x0F);
    }
    else if (reg >= 0x40 && reg <= 0x55) {
        handleOperatorRegister(reg - 0x40, value, &OPL3Operator::ksl, 6, 
                              nullptr, 0, nullptr, 0, nullptr, 0, &OPL3Operator::outputLevel, 0x3F);
    }
    else if (reg >= 0x60 && reg <= 0x75) {
        handleOperatorRegister(reg - 0x60, value, &OPL3Operator::attackRate, 4, 
                              nullptr, 0, nullptr, 0, nullptr, 0, &OPL3Operator::decayRate, 0x0F);
    }
    else if (reg >= 0x80 && reg <= 0x95) {
        handleOperatorRegister(reg - 0x80, value, &OPL3Operator::sustainLevel, 4, 
                              nullptr, 0, nullptr, 0, nullptr, 0, &OPL3Operator::releaseRate, 0x0F);
    }
    else if (reg >= 0xA0 && reg <= 0xA8) {
        handleFrequencyLowRegister(reg - 0xA0, value);
    }
    else if (reg >= 0xB0 && reg <= 0xB8) {
        handleFrequencyHighRegister(reg - 0xB0, value);
    }
    else if (reg >= 0xC0 && reg <= 0xC8) {
        handleChannelConfigRegister(reg - 0xC0, value);
    }
    else if (reg == 0xBD) {
        // Rhythm/percussion - пока игнорируем
    }
    // Остальные регистры уже обработаны в reset()
}

void OPL3Emulator::handleOperatorRegister(int opIndex, uint8_t value,
    uint8_t OPL3Operator::*field1, int shift1,
    uint8_t OPL3Operator::*field2, int shift2, 
    uint8_t OPL3Operator::*field3, int shift3,
    uint8_t OPL3Operator::*field4, int shift4,
    uint8_t OPL3Operator::*field5, uint8_t mask5) {
    
    if (opIndex < 18) {
        OPL3Operator& op = getOperator(opIndex);
        
        if (field1) op.*field1 = (value >> shift1) & 1;
        if (field2) op.*field2 = (value >> shift2) & 1;
        if (field3) op.*field3 = (value >> shift3) & 1;
        if (field4) op.*field4 = (value >> shift4) & 1;
        if (field5) op.*field5 = value & mask5;
        
        // Обновляем частоту с новыми параметрами
        if (op.fnum > 0) {
            op.setFrequency(op.fnum, op.block);
        }
    }
}

void OPL3Emulator::handleFrequencyLowRegister(int channel, uint8_t value) {
    if (channel < 9) {
        channels[channel].fnum = (channels[channel].fnum & 0x300) | value;
        channels[channel].setFrequency(channels[channel].fnum, channels[channel].block);
    }
}

void OPL3Emulator::handleFrequencyHighRegister(int channel, uint8_t value) {
    if (channel < 9) {
        bool keyOn = (value >> 5) & 1;
        uint8_t block = (value >> 2) & 7;
        channels[channel].block = block;
        channels[channel].fnum = (channels[channel].fnum & 0xFF) | ((value & 3) << 8);
        
        channels[channel].setFrequency(channels[channel].fnum, block);
        channels[channel].setKeyOn(keyOn);
    }
}

void OPL3Emulator::handleChannelConfigRegister(int channel, uint8_t value) {
    if (channel < 9) {
        channels[channel].feedback = (value >> 1) & 7;
        channels[channel].synthType = value & 1;
        
        // Обновляем обратную связь
        channels[channel].op1.feedbackLevel = channels[channel].feedback / 7.0f;
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

void OPL3Emulator::setSampleRate(double rate) { 
    sampleRate = rate;
    for (int i = 0; i < 18; i++) {
        channels[i].op1.sampleRate = rate;
        channels[i].op2.sampleRate = rate;
    }
}