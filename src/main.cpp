#include <portaudio.h>
#include "MidiFileParser.h"
#include "OPL3Emulator.h"
#include "OPL3Driver.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>

static int audioCallback(const void* input, void* output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void* userData) {
    (void)input;
    (void)timeInfo;
    (void)statusFlags;
    
    OPL3Emulator* emulator = static_cast<OPL3Emulator*>(userData);
    int16_t* buffer = static_cast<int16_t*>(output);
    
    // Очистка буфера
    memset(buffer, 0, frameCount * 2 * sizeof(int16_t));
    
    // Рендеринг в буфер
    emulator->render(buffer, static_cast<int>(frameCount * 2));
    
    return paContinue;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <midi_file>" << std::endl;
        std::cout << "No MIDI file provided, running in test mode..." << std::endl;
        
        // Тестовый режим без файла
        OPL3Emulator emulator;
        OPL3Driver driver(emulator);
        
        // Загружаем тестовый инструмент
        driver.loadGMInstrument(0, 0); // Piano
        
        // Простая тестовая нота
        driver.noteOn(0, 60, 100);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        driver.noteOff(0, 60);
        
        std::cout << "Test completed." << std::endl;
        return 0;
    }

    MidiFileParser parser;
    if (!parser.load(argv[1])) {
        std::cerr << "Failed to load MIDI file: " << argv[1] << std::endl;
        return 1;
    }

    OPL3Emulator emulator;
    OPL3Driver driver(emulator);

    // Инициализация PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 
                               0,      // input channels
                               2,      // output channels (stereo)
                               paInt16, // sample format
                               44100,  // sample rate
                               256,    // frames per buffer
                               audioCallback, 
                               &emulator);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Playing MIDI file: " << argv[1] << std::endl;

    // Главный цикл воспроизведения
    auto events = parser.getEvents();
    auto nextEvent = events.begin();
    auto startTime = std::chrono::steady_clock::now();

    while (nextEvent != events.end()) {
        auto currentTime = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        
        while (nextEvent != events.end() && elapsed >= nextEvent->time) {
            driver.handleEvent(*nextEvent);
            ++nextEvent;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Проверка состояния потока
        if (Pa_IsStreamStopped(stream)) {
            std::cerr << "Audio stream stopped unexpectedly!" << std::endl;
            break;
        }
    }

    // Даем время завершить звучание
    std::cout << "Finishing playback..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    std::cout << "Playback finished." << std::endl;
    return 0;
}