#ifndef OPL3EMULATOR_H
#define OPL3EMULATOR_H

#include <cstdint>

class OPL3Operator {
public:
    OPL3Operator();
    void reset();
    void setFrequency(uint16_t fnum, uint8_t block);
    void setKeyOn(bool on);
    void updateEnvelope();
    int16_t getSample();
    double getOutput();

    // Регистры
    uint8_t tremolo, vibrato, sustain, ksr, multi;
    uint8_t ksl, outputLevel;
    uint8_t attackRate, decayRate;
    uint8_t sustainLevel, releaseRate;
    uint8_t waveform;

    // Добавляем недостающие поля
    double sampleRate;
    class OPL3Operator* modulator;
    float feedbackLevel;

    // Добавляем поля для частоты
    uint16_t fnum;

private:
    uint32_t phase;     // Фазовый аккумулятор
    uint32_t phaseStep; // Шаг фазы
    bool keyOn;
    int envStage; // 0:Attack, 1:Decay, 2:Sustain, 3:Release
    int envLevel; // Текущий уровень огибающей
};

class OPL3Channel {
public:
    OPL3Channel();
    void reset();
    void setFrequency(uint16_t fnum, uint8_t block);
    void setKeyOn(bool on);
    int16_t getSample();

    OPL3Operator op1, op2;
    uint8_t feedback;
    uint8_t synthType;
    bool left, right;
    
    // Добавляем поле для частоты
    uint16_t fnum;
};

class OPL3Emulator {
public:
    OPL3Emulator();
    void reset();
    void writeRegister(uint16_t reg, uint8_t value);
    void render(int16_t* buffer, int samples);

    // Добавьте это для доступа из OPL3Driver
    OPL3Channel channels[18]; // 18 melodic channels

private:
    double sampleRate;
    OPL3Operator& getOperator(int index);
};

#endif