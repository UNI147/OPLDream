#ifndef OPL3PRESETS_H
#define OPL3PRESETS_H

#include <cstdint>

struct OPL3Preset {
    const char* name;
    
    // Параметры для двух операторов
    uint8_t trem_vib_sus_ksr_multi[2];
    uint8_t ksl_outputLevel[2];
    uint8_t attackDecay[2];
    uint8_t sustainRelease[2];
    uint8_t waveform[2];
    uint8_t feedback;
    uint8_t synthType;
};

class OPL3PresetLibrary {
public:
    static const OPL3Preset* getPreset(int index);
    static const OPL3Preset* findPreset(const char* name);
    static int getPresetCount();
};

#endif