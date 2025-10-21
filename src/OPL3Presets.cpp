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
    {
        "Honky Tonk Piano",
        {0x21, 0x21},
        {0x0A, 0x0A},
        {0xF2, 0xF2},
        {0x74, 0x74},
        {0, 0},
        5, 0
    },
    {
        "Rhodes Piano",
        {0x31, 0x31},
        {0x1C, 0x1C},
        {0x85, 0x85},
        {0x63, 0x63},
        {0, 0},
        2, 0
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
    {
        "Finger Bass",
        {0x31, 0x31},
        {0x0C, 0x00},
        {0x86, 0xF6},
        {0x65, 0x67},
        {0, 0},
        5, 0
    },
    {
        "Pick Bass",
        {0x31, 0x31},
        {0x08, 0x00},
        {0x8A, 0xFA},
        {0x72, 0x75},
        {0, 0},
        6, 0
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
    {
        "Sawtooth Lead",
        {0x71, 0x31},
        {0x1A, 0x00},
        {0x8F, 0xF8},
        {0x45, 0x55},
        {0, 0},
        4, 0
    },
    {
        "Square Lead",
        {0x61, 0x21},
        {0x18, 0x00},
        {0x8E, 0xF6},
        {0x52, 0x62},
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
    {
        "Slow Strings",
        {0x61, 0x61},
        {0x1D, 0x1D},
        {0x8C, 0x8C},
        {0x9E, 0x9E},
        {0, 0},
        0, 1
    },
    {
        "Synth Strings",
        {0x71, 0x71},
        {0x1A, 0x1A},
        {0x8F, 0x8F},
        {0x7A, 0x7A},
        {0, 0},
        1, 1
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
    },
    {
        "Trombone",
        {0x21, 0x21},
        {0x1A, 0x00},
        {0x88, 0xF8},
        {0x66, 0x68},
        {0, 0},
        3, 0
    },
    {
        "French Horn",
        {0x21, 0x21},
        {0x18, 0x00},
        {0x86, 0xF6},
        {0x68, 0x6A},
        {0, 0},
        2, 0
    },
    {
        "Synth Brass",
        {0x71, 0x31},
        {0x16, 0x00},
        {0x8F, 0xF8},
        {0x56, 0x66},
        {0, 0},
        5, 0
    },
    
    // Woodwinds
    {
        "Flute",
        {0x21, 0x21},
        {0x14, 0x00},
        {0x84, 0xF4},
        {0x7A, 0x7C},
        {0, 0},
        1, 0
    },
    {
        "Clarinet",
        {0x21, 0x21},
        {0x16, 0x00},
        {0x86, 0xF6},
        {0x76, 0x78},
        {0, 0},
        2, 0
    },
    {
        "Oboe",
        {0x21, 0x21},
        {0x18, 0x00},
        {0x88, 0xF8},
        {0x74, 0x76},
        {0, 0},
        3, 0
    },
    
    // Organ
    {
        "Church Organ",
        {0x21, 0x21},
        {0x0C, 0x0C},
        {0x8F, 0x8F},
        {0x6E, 0x6E},
        {0, 0},
        0, 1
    },
    {
        "Jazz Organ",
        {0x31, 0x31},
        {0x1A, 0x1A},
        {0x85, 0x85},
        {0x72, 0x72},
        {0, 0},
        1, 1
    },
    
    // Guitar
    {
        "Overdriven Guitar",
        {0x31, 0x21},
        {0x1A, 0x0F},
        {0x8F, 0xF8},
        {0x54, 0x37},
        {0, 0},
        6, 0
    },
    {
        "Bright Guitar",
        {0x31, 0x21},
        {0x15, 0x0A},
        {0x8F, 0xF6},
        {0x54, 0x37},
        {0, 0},
        6, 0
    },
    {
        "Clean Guitar",
        {0x21, 0x21},
        {0x12, 0x12},
        {0x86, 0xF6},
        {0x65, 0x67},
        {0, 0},
        4, 0
    },
    {
        "Muted Guitar",
        {0x21, 0x21},
        {0x0E, 0x0E},
        {0x84, 0xF4},
        {0x72, 0x74},
        {0, 0},
        3, 0
    },
    
    // Drums & Percussion
    {
        "Bass Drum",
        {0x31, 0x31},
        {0x1F, 0x00},
        {0x8F, 0xF8},
        {0x23, 0x25},
        {0, 0},
        7, 0
    },
    {
        "Snare Drum",
        {0x31, 0x31},
        {0x1C, 0x00},
        {0x8F, 0xF6},
        {0x45, 0x47},
        {0, 0},
        5, 0
    },
    
    // Ethnic
    {
        "Kalimba",
        {0x21, 0x21},
        {0x0A, 0x00},
        {0x8A, 0xFA},
        {0x85, 0x87},
        {0, 0},
        2, 0
    },
    {
        "Koto",
        {0x21, 0x21},
        {0x0C, 0x00},
        {0x88, 0xF8},
        {0x76, 0x78},
        {0, 0},
        3, 0
    },
    
    // Synth Effects
    {
        "Crystal",
        {0x71, 0x31},
        {0x1E, 0x00},
        {0x8F, 0xFF},
        {0x9A, 0x9C},
        {0, 0},
        1, 0
    },
    {
        "Atmosphere",
        {0x61, 0x61},
        {0x1F, 0x1F},
        {0x8E, 0x8E},
        {0xAE, 0xAE},
        {0, 0},
        0, 1
    },
    {
        "Brightness",
        {0x71, 0x31},
        {0x1A, 0x00},
        {0x8F, 0xF8},
        {0x67, 0x77},
        {0, 0},
        2, 0
    },
    
    // Additional Piano variants
    {
        "Loud Piano",
        {0x21, 0x21},
        {0x00, 0x00},
        {0xF1, 0xF1},
        {0x77, 0x77},
        {0, 0},
        3, 0
    },
    {
        "Soft Piano",
        {0x21, 0x21},
        {0x0C, 0x0C},
        {0xC2, 0xC2},
        {0x7E, 0x7E},
        {0, 0},
        2, 0
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