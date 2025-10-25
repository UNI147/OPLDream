#ifndef OPL3OPERATOR_H
#define OPL3OPERATOR_H

#include <cstdint>

#ifndef OPL3_CLOCK
#define OPL3_CLOCK 49716.0
#endif

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

    double sampleRate;
    OPL3Operator* modulator;
    float feedbackLevel;

    uint16_t fnum;
    uint8_t block;

    // Фаза и шаг фазы
    uint32_t phase;     // Фазовый аккумулятор
    uint32_t phaseStep; // Шаг фазы

    // Методы для отладки
    bool isKeyOn() const { return keyOn; }
    int getEnvLevel() const { return envLevel; }
    int getEnvStage() const { return envStage; }

private:
    bool keyOn;
    int envStage; // 0:Attack, 1:Decay, 2:Sustain, 3:Release
    int envLevel; // Текущий уровень огибающей (0-1023)
    int targetLevel; // Целевой уровень для текущей стадии
};

#endif