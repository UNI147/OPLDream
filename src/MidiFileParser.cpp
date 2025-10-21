#include "MidiFileParser.h"
#include <fstream>
#include <iostream>

bool MidiFileParser::load(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }
    
    // Чтение заголовка
    char header[14];
    file.read(header, 14);
    
    if (header[0] != 'M' || header[1] != 'T' || header[2] != 'h' || header[3] != 'd') {
        std::cerr << "Not a MIDI file" << std::endl;
        return false;
    }
    
    uint32_t length = (uint8_t(header[4]) << 24) | (uint8_t(header[5]) << 16) | 
                     (uint8_t(header[6]) << 8) | uint8_t(header[7]);
    uint16_t format = (uint8_t(header[8]) << 8) | uint8_t(header[9]);
    uint16_t tracks = (uint8_t(header[10]) << 8) | uint8_t(header[11]);
    uint16_t division = (uint8_t(header[12]) << 8) | uint8_t(header[13]);
    
    std::cout << "MIDI File: format=" << format << ", tracks=" << tracks 
              << ", division=" << division << std::endl;
    
    events.clear();
    
    // Чтение треков
    for (int i = 0; i < tracks; i++) {
        if (!parseTrack(file, division)) {
            std::cerr << "Failed to parse track " << i << std::endl;
            return false;
        }
    }
    
    std::cout << "Loaded " << events.size() << " MIDI events" << std::endl;
    return true;
}

bool MidiFileParser::parseTrack(std::ifstream& file, uint16_t division) {
    char trackHeader[8];
    file.read(trackHeader, 8);
    
    if (file.gcount() != 8) {
        return false;
    }
    
    if (trackHeader[0] != 'M' || trackHeader[1] != 'T' || 
        trackHeader[2] != 'r' || trackHeader[3] != 'k') {
        std::cerr << "Invalid track header" << std::endl;
        return false;
    }
    
    uint32_t trackLength = (uint8_t(trackHeader[4]) << 24) | (uint8_t(trackHeader[5]) << 16) |
                          (uint8_t(trackHeader[6]) << 8) | uint8_t(trackHeader[7]);
    
    std::cout << "Track length: " << trackLength << " bytes" << std::endl;
    
    // Упрощенная временная модель: 120 BPM = 500000 микросекунд на четверть ноты
    double microsecondsPerQuarterNote = 500000.0;
    double ticksPerMicrosecond = division / microsecondsPerQuarterNote;
    
    double currentTime = 0.0;
    uint8_t lastStatus = 0;
    
    while (trackLength > 0) {
        // Читаем delta-time
        uint32_t deltaTime = readVariableLength(file, trackLength);
        if (file.eof()) break;
        
        // Конвертируем в секунды
        currentTime += deltaTime / (division * ticksPerMicrosecond * 1000.0);
        
        uint8_t statusByte;
        file.read(reinterpret_cast<char*>(&statusByte), 1);
        if (file.eof()) break;
        trackLength--;
        
        // Проверяем running status
        if (statusByte < 0x80) {
            // Это data byte, используем последний status
            statusByte = lastStatus;
            file.seekg(-1, std::ios::cur); // Возвращаемся назад
            trackLength++;
        } else {
            lastStatus = statusByte;
        }
        
        uint8_t eventType = statusByte & 0xF0;
        uint8_t channel = statusByte & 0x0F;
        
        switch (eventType) {
            case 0x80: { // Note Off
                uint8_t note, velocity;
                file.read(reinterpret_cast<char*>(&note), 1);
                file.read(reinterpret_cast<char*>(&velocity), 1);
                if (file.eof()) break;
                trackLength -= 2;
                
                events.push_back(MidiEvent(currentTime, NOTE_OFF, channel, note, velocity));
                break;
            }
            case 0x90: { // Note On
                uint8_t note, velocity;
                file.read(reinterpret_cast<char*>(&note), 1);
                file.read(reinterpret_cast<char*>(&velocity), 1);
                if (file.eof()) break;
                trackLength -= 2;
                
                if (velocity > 0) {
                    events.push_back(MidiEvent(currentTime, NOTE_ON, channel, note, velocity));
                } else {
                    // Note On с velocity=0 трактуется как Note Off
                    events.push_back(MidiEvent(currentTime, NOTE_OFF, channel, note, 0));
                }
                break;
            }
            case 0xB0: { // Control Change
                uint8_t controller, value;
                file.read(reinterpret_cast<char*>(&controller), 1);
                file.read(reinterpret_cast<char*>(&value), 1);
                if (file.eof()) break;
                trackLength -= 2;
                
                events.push_back(MidiEvent(currentTime, CONTROL_CHANGE, channel, controller, value));
                break;
            }
            case 0xC0: { // Program Change
                uint8_t program;
                file.read(reinterpret_cast<char*>(&program), 1);
                if (file.eof()) break;
                trackLength -= 1;
                
                events.push_back(MidiEvent(currentTime, PROGRAM_CHANGE, channel, program, 0));
                break;
            }
            case 0xE0: { // Pitch Bend
                uint8_t lsb, msb;
                file.read(reinterpret_cast<char*>(&lsb), 1);
                file.read(reinterpret_cast<char*>(&msb), 1);
                if (file.eof()) break;
                trackLength -= 2;
                // Пропускаем для простоты
                break;
            }
            case 0xF0: { // System messages
                if (statusByte == 0xFF) {
                    // Meta event
                    uint8_t metaType;
                    file.read(reinterpret_cast<char*>(&metaType), 1);
                    if (file.eof()) break;
                    trackLength--;
                    
                    uint32_t length = readVariableLength(file, trackLength);
                    if (file.eof()) break;
                    
                    // Пропускаем данные meta-события
                    file.seekg(length, std::ios::cur);
                    trackLength -= length;
                } else {
                    // Пропускаем другие system messages
                    uint32_t length = readVariableLength(file, trackLength);
                    if (file.eof()) break;
                    file.seekg(length, std::ios::cur);
                    trackLength -= length;
                }
                break;
            }
            default:
                // Пропускаем неизвестные события
                file.seekg(2, std::ios::cur);
                trackLength -= 2;
                break;
        }
        
        if (file.eof()) break;
    }
    
    return true;
}

uint32_t MidiFileParser::readVariableLength(std::ifstream& file, uint32_t& remaining) {
    uint32_t value = 0;
    uint8_t byte;
    
    do {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (file.eof()) break;
        remaining--;
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    
    return value;
}