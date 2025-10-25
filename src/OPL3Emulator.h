#ifndef OPL3EMULATOR_H
#define OPL3EMULATOR_H

#include "OPL3Channel.h"
#include <cstdint>

class OPL3Emulator {
public:
    OPL3Emulator();
    void reset();
    void writeRegister(uint16_t reg, uint8_t value);
    void render(int16_t* buffer, int samples);
    
    // Методы для работы с частотой дискретизации
    double getSampleRate() const { return sampleRate; }
    void setSampleRate(double rate);
    
    OPL3Channel channels[18]; // 18 melodic channels

private:
    double sampleRate;
    
    // Вспомогательные методы для обработки регистров
    OPL3Operator& getOperator(int index);
    void handleOperatorRegister(int opIndex, uint8_t value,
        uint8_t OPL3Operator::*field1, int shift1,
        uint8_t OPL3Operator::*field2, int shift2,
        uint8_t OPL3Operator::*field3, int shift3,
        uint8_t OPL3Operator::*field4, int shift4,
        uint8_t OPL3Operator::*field5, uint8_t mask5);
        
    void handleFrequencyLowRegister(int channel, uint8_t value);
    void handleFrequencyHighRegister(int channel, uint8_t value);
    void handleChannelConfigRegister(int channel, uint8_t value);
};

#endif