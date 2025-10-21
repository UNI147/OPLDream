#ifndef OPL3EMULATOR_H
#define OPL3EMULATOR_H

#include <cstdint>
#include <cmath>

// Объявляем OPL3_CLOCK как extern
extern const double OPL3_CLOCK;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class LFO {
private:
    double phase;
    double phaseStep;
    double sampleRate;
    
public:
    LFO(double rate, double sr) : phase(0), sampleRate(sr) {
        phaseStep = 2.0 * M_PI * rate / sampleRate;
    }
    
    double getValue() {
        double value = std::sin(phase);
        phase += phaseStep;
        if(phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
        return value;
    }
};

class OPL3Operator {
public:
    OPL3Operator();
    void reset();
    void setFrequency(uint16_t fnum, uint8_t block);
    void setKeyOn(bool on);
    void updateEnvelope();
    int16_t getSample();
    double getOutput();
    double getWaveformOutput(double phase);
    int getCurrentRate();

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

    // Делаем фазу и шаг фазы публичными для доступа из OPL3Channel
    uint32_t phase;     // Фазовый аккумулятор
    uint32_t phaseStep; // Шаг фазы

    // Добавляем публичные методы для отладки
    bool isKeyOn() const { return keyOn; }
    int getEnvLevel() const { return envLevel; }
    int getEnvStage() const { return envStage; }

private:
    bool keyOn;
    int envStage; // 0:Attack, 1:Decay, 2:Sustain, 3:Release
    int envLevel; // Текущий уровень огибающей (0-1023)
    int targetLevel; // Целевой уровень для текущей стадии
    LFO lfo = LFO{6.0, 49716.0}; // LFO для тремоло/вибрато (~6 Гц)
};

class OPL3Channel {
public:
    OPL3Channel();
    void reset();
    void setFrequency(uint16_t fnum, uint8_t block);
    void setKeyOn(bool on);
    int16_t getSample();
    
    // Добавляем метод для проверки активности
    bool isActive() const;

    OPL3Operator op1, op2;
    uint8_t feedback;
    uint8_t synthType;
    bool left, right;
    
    uint16_t fnum;
    uint8_t block;
};

class OPL3Emulator {
public:
    OPL3Emulator();
    void reset();
    void writeRegister(uint16_t reg, uint8_t value);
    void render(int16_t* buffer, int samples);
    
    // Добавляем метод для получения частоты дискретизации
    double getSampleRate() const { return sampleRate; }
    
    // Добавляем метод для установки частоты дискретизации
    void setSampleRate(double rate) { 
        sampleRate = rate;
        for (int i = 0; i < 18; i++) {
            channels[i].op1.sampleRate = rate;
            channels[i].op2.sampleRate = rate;
        }
    }

    OPL3Channel channels[18]; // 18 melodic channels

private:
    double sampleRate;
    bool rhythmMode = false;
    
    OPL3Operator& getOperator(int index);
    void enable4OPMode(int channelPair);
    void updatePercussion(uint8_t percussionBits);
};

#endif