#include "OPL3Channel.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    // Правильная FM модуляция: выход модулятора влияет на частоту несущей
    double modulatorOutput = op1.getOutput();
    
    // Модулируем фазу carrier (частотная модуляция)
    double modDepth = modulatorOutput * 8.0 * (feedback + 1);
    double phaseMod = modDepth * sin(2.0 * M_PI * op1.phase / 1048576.0);
    
    // Временно применяем модуляцию
    uint32_t originalPhase = op2.phase;
    op2.phase = static_cast<uint32_t>(std::fmod(op2.phase + phaseMod * 65536.0, 1048576.0));
    
    double carrierOutput = op2.getOutput();
    
    // Восстанавливаем фазу
    op2.phase = originalPhase;
    
    // Обновляем фазы операторов
    op1.phase = static_cast<uint32_t>(std::fmod(op1.phase + op1.phaseStep, 1048576.0));
    op2.phase = static_cast<uint32_t>(std::fmod(op2.phase + op2.phaseStep, 1048576.0));
    
    int32_t result = static_cast<int32_t>(carrierOutput * 8192.0);
    
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    
    return static_cast<int16_t>(result);
}