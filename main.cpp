#include <portaudio.h>
#include "MidiFileParser.h"
#include "OPL3Emulator.h"
#include "OPL3Driver.h"

// ... Callback-функция для PortAudio

int main(int argc, char* argv[]) {
    MidiFileParser parser;
    if (!parser.load("test.mid")) {
        printf("Failed to load MIDI file.\n");
        return 1;
    }

    OPL3Emulator emulator;
    OPL3Driver driver(emulator);

    // Инициализация PortAudio
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paInt16, 44100, 256, audioCallback, &emulator);
    Pa_StartStream(stream);

    // Главный цикл воспроизведения
    auto& events = parser.getEvents();
    double currentTime = 0.0;
    auto nextEvent = events.begin();

    while (nextEvent != events.end() && currentTime <= nextEvent->time) {
        driver.handleEvent(*nextEvent);
        ++nextEvent;
        // Здесь должна быть точная синхронизация по времени
        // Например, с использованием Pa_GetStreamTime()
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}