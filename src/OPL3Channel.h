#ifndef OPL3CHANNEL_H
#define OPL3CHANNEL_H

#include "OPL3Operator.h"

class OPL3Channel {
public:
    OPL3Channel();
    void reset();
    void setFrequency(uint16_t fnum, uint8_t block);
    void setKeyOn(bool on);
    int16_t getSample();
    
    // Метод для проверки активности
    bool isActive() const;

    OPL3Operator op1, op2;
    uint8_t feedback;
    uint8_t synthType;
    bool left, right;
    
    uint16_t fnum;
    uint8_t block;
};

#endif