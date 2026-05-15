#pragma once

#include "Config.h"

#include "ListModel.h"

#include "model/UserSettings.h"

class SettingsListModel : public ListModel {
public:
    SettingsListModel(UserSettings &userSettings) :
        _userSettings(userSettings)
    {}

    int rows() const override {
        return visibleSettingCount() + 1;
    }

    int columns() const override {
        return 2;
    }

    void cell(int row, int column, StringBuilder &str) const override {
        if (isChaosDefaultsRow(row)) {
            if (column == 0) {
                str("Chaos Defaults");
            } else if (column == 1) {
                str("open");
            }
            return;
        }

        int settingRow = settingRowFromVisibleRow(row);
        if (settingRow < 0) {
            return;
        }

        if (column == 0) {
            str("%s", _userSettings.get(settingRow)->getMenuItem().c_str());
        } else if (column == 1) {
            str("%s", _userSettings.get(settingRow)->getMenuItemKey().c_str());
        }
    }

    void edit(int row, int column, int value, bool shift) override {
        if (isChaosDefaultsRow(row)) {
            return;
        }

        int settingRow = settingRowFromVisibleRow(row);
        if (settingRow >= 0 && column == 1) {
            _userSettings.shift(settingRow, value);
        }
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

    bool isChaosDefaultsRow(int row) const {
        return row == visibleSettingCount();
    }

    void setIsLaunchopad(bool value) {
        _isLaunchpad = value;
    }

private:
    bool hideSetting(BaseSetting *setting) const {
        const auto key = setting->getKey();
        if (key == SettingChaosSeqLayers || key == SettingChaosPatLayers || key == SettingEntropyLayers ||
            key == SettingChaosPivotNote || key == SettingChaosSpan) {
            return true;
        }
        if (!_isLaunchpad && (key == SettingLaunchpadStyle || key == SettingLaunchpadNoteStyle)) {
            return true;
        }
        return false;
    }

    int visibleSettingCount() const {
        int count = 0;
        for (auto *setting : _userSettings.all()) {
            if (!hideSetting(setting)) {
                ++count;
            }
        }
        return count;
    }

    int settingRowFromVisibleRow(int row) const {
        int visibleRow = 0;
        const auto settings = _userSettings.all();
        for (int i = 0; i < int(settings.size()); ++i) {
            if (hideSetting(settings[i])) {
                continue;
            }
            if (visibleRow == row) {
                return i;
            }
            ++visibleRow;
        }
        return -1;
    }

    UserSettings &_userSettings;

    bool _isLaunchpad = false;
};
