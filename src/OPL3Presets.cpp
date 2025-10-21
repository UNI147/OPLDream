#include "OPL3Presets.h"
#include <cstring> // Добавляем для strcmp

static const OPL3Preset presets[] = {
    // Piano
    {
        "Acoustic Piano",
        {0x21, 0x21},
        {0x00, 0x00},
        {0xF0, 0xF0},
        {0x77, 0x77},
        {0, 0},
        4, 0
    },
    {
        "Electric Piano", 
        {0x31, 0x31},
        {0x12, 0x12},
        {0xA3, 0xA3},
        {0x54, 0x54},
        {0, 0},
        3, 0
    },
    // Bass
    {
        "Acoustic Bass",
        {0x31, 0x31},
        {0x09, 0x00},
        {0x82, 0xF2},
        {0x73, 0x74},
        {0, 0},
        6, 0
    },
    {
        "Synth Bass",
        {0x31, 0x11},
        {0x0F, 0x00},
        {0x89, 0xF1},
        {0x23, 0x25},
        {0, 0},
        7, 0
    },
    // Lead
    {
        "Lead Synth",
        {0x71, 0x31},
        {0x1C, 0x00},
        {0x8A, 0xFA},
        {0x37, 0x47},
        {0, 0},
        3, 0
    },
    // Strings
    {
        "Strings",
        {0x61, 0x61},
        {0x1F, 0x1F},
        {0x8E, 0x8E},
        {0x8A, 0x8A},
        {0, 0},
        0, 1
    },
    // Brass
    {
        "Trumpet",
        {0x21, 0x21},
        {0x1C, 0x00},
        {0x8A, 0xFA},
        {0x64, 0x67},
        {0, 0},
        3, 0
    }
};

const OPL3Preset* OPL3PresetLibrary::getPreset(int index) {
    if (index >= 0 && index < static_cast<int>(sizeof(presets) / sizeof(presets[0]))) {
        return &presets[index];
    }
    return &presets[0]; // Fallback
}

const OPL3Preset* OPL3PresetLibrary::findPreset(const char* name) {
    for (size_t i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
        if (strcmp(presets[i].name, name) == 0) {
            return &presets[i];
        }
    }
    return nullptr;
}

int OPL3PresetLibrary::getPresetCount() {
    return sizeof(presets) / sizeof(presets[0]);
}