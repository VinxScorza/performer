#include "UserSettings.h"

void UserSettings::set(int key, int value) {
    _settings[key]->setValue(value);
}

void UserSettings::shift(int key, int shift) {
    _settings[key]->shiftValue(shift);
}

BaseSetting *UserSettings::_get(const std::string& key) {
    for (auto &setting : _settings) {
        if (setting->getKey() == key) {
            return setting;
        }
    }
    return nullptr;
}

BaseSetting *UserSettings::get(int key) {
    return _settings[key];
}

std::vector<BaseSetting *> UserSettings::all() {
    return _settings;
}

void UserSettings::clear() {
    for (auto &setting : _settings) {
        setting->reset();
    }
}

void UserSettings::write(VersionedSerializedWriter &writer) const {
    for (auto &setting : _settings) {
        setting->write(writer);
    }
}

void UserSettings::read(VersionedSerializedReader &reader) {
    for (auto &setting : _settings) {
        if (setting->getKey() == SettingChaosSeqLayers) {
            reader.read(dynamic_cast<ChaosSeqLayersSetting *>(setting)->getValue(), 3);
        } else if (setting->getKey() == SettingChaosPatLayers) {
            reader.read(dynamic_cast<ChaosPatLayersSetting *>(setting)->getValue(), 3);
        } else if (setting->getKey() == SettingEntropyLayers) {
            reader.read(dynamic_cast<EntropyLayersSetting *>(setting)->getValue(), 5);
        } else if (setting->getKey() == SettingTriggerLength) {
            reader.read(dynamic_cast<TriggerLengthSetting *>(setting)->getValue(), 8);
        } else if (setting->getKey() == SettingChaosPivotNote) {
            reader.read(dynamic_cast<ChaosPivotNoteSetting *>(setting)->getValue(), 6);
        } else if (setting->getKey() == SettingChaosSpan) {
            reader.read(dynamic_cast<ChaosSpanSetting *>(setting)->getValue(), 6);
        } else if (setting->getKey() == SettingMenuWrap) {
            reader.read(dynamic_cast<MenuWrapSetting *>(setting)->getValue(), 4);
        } else {
            setting->read(reader);
        }
    }

    // Removed legacy "Chaos Cumul" setting in settings data version 9.
    // Consume the old trailing byte to keep hash-compatible reads for v7-v8 data.
    reader.skip<uint8_t>(7, 9);
}
