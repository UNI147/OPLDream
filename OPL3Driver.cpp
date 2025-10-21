void OPL3Driver::loadPatch(uint8_t midiChannel, const OPL3Patch& patch) {
    int oplChannel = allocateOPLChannel(midiChannel);
    channelMap[midiChannel].patch = patch;
    channelMap[midiChannel].oplChannel = oplChannel;

    // Записываем параметры патча в регистры OPL3
    uint8_t baseReg = (oplChannel < 9) ? 0x00 : 0x100;
    uint8_t ch = oplChannel % 9;

    // Регистры для оператора 1 и 2
    uint8_t op1_offset = getOperatorOffset(ch, 0); // Вспом. функция из таблицы в руководстве
    uint8_t op2_offset = getOperatorOffset(ch, 1);

    emulator.writeRegister(baseReg + 0x20 + op1_offset, patch.trem_vib_sus_ksr_multi[0]);
    emulator.writeRegister(baseReg + 0x40 + op1_offset, patch.ksl_outputLevel[0]);
    emulator.writeRegister(baseReg + 0x60 + op1_offset, patch.attackDecay[0]);
    emulator.writeRegister(baseReg + 0x80 + op1_offset, patch.sustainRelease[0]);
    emulator.writeRegister(baseReg + 0xE0 + op1_offset, patch.waveform[0]);

    emulator.writeRegister(baseReg + 0x20 + op2_offset, patch.trem_vib_sus_ksr_multi[1]);
    emulator.writeRegister(baseReg + 0x40 + op2_offset, patch.ksl_outputLevel[1]);
    emulator.writeRegister(baseReg + 0x60 + op2_offset, patch.attackDecay[1]);
    emulator.writeRegister(baseReg + 0x80 + op2_offset, patch.sustainRelease[1]);
    emulator.writeRegister(baseReg + 0xE0 + op2_offset, patch.waveform[1]);

    // Регистр Cx: Feedback/Synth Type
    uint8_t cvalue = (patch.feedback << 1) | patch.synthType;
    emulator.writeRegister(baseReg + 0xC0 + ch, cvalue);
}