#include "OPL3Channel.h"
#include <cmath>

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
    double modulatedPhase = static_cast<double>(op2.phase) + modAmount;
    
    // Сохраняем оригинальную фазу
    uint32_t originalPhase = op2.phase;
    
    // Временно устанавливаем модулированную фазу
    op2.phase = static_cast<uint32_t>(std::fmod(modulatedPhase, 1048576.0));
    
    // Получаем выход carrier с модуляцией
    double carrierOutput = op2.getOutput();
    
    // Восстанавливаем фазу carrier
    op2.phase = originalPhase;
    
    // Конвертируем в 16-бит с большей амплитудой
    int32_t result = static_cast<int32_t>(carrierOutput * 16384.0);
    
    // Ограничение
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    
    return static_cast<int16_t>(result);
}