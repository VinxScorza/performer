#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/LogicSequence.h"
#include "model/Scale.h"

class LogicSequenceListModel : public RoutableListModel {
public:
    static constexpr int ScaleSelectionCount = 1 + 20 + CONFIG_USER_SCALE_COUNT;

    enum Item {
        Name,
        FirstStep,
        LastStep,
        RunMode,
        Divisor,
        ResetMeasure,
        Scale,
        RootNote,
        Last
    };

    LogicSequenceListModel()
    {
        _scales[0] = -1;
        for (int i = 1; i < ScaleSelectionCount; ++i) {
            _scales[i] = i-1;
        }

        for (int i = 0; i < 8; ++i) {
            _selectedScale[i] = 0;
        }
    }

    void setSequence(LogicSequence *sequence) {
        _sequence = sequence;
        if (sequence != nullptr) {
            int trackIndex = _sequence->trackIndex();
            _selectedScale[trackIndex] = sequence->scale()+1;
        }
    }

    virtual int rows() const override {
        return _sequence ? Last : 0;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            formatName(Item(row), str);
        } else if (column == 1) {
            formatValue(Item(row), str);
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
        if (column == 1) {
            editValue(Item(row), value, shift);
        }
    }

    virtual int indexedCount(int row) const override {
        return indexedCountValue(Item(row));
    }

    virtual int indexed(int row) const override {
        return indexedValue(Item(row));
    }

    virtual void setIndexed(int row, int index) override {
        if (index >= 0 && index < indexedCount(row)) {
            setIndexedValue(Item(row), index);
        }
    }

    virtual Routing::Target routingTarget(int row) const override {
        switch (Item(row)) {
        case Divisor:
            return Routing::Target::Divisor;
        case FirstStep:
            return Routing::Target::FirstStep;
        case LastStep:
            return Routing::Target::LastStep;
        case RunMode:
            return Routing::Target::RunMode;
        case Scale:
            return Routing::Target::Scale;
        case RootNote:
            return Routing::Target::RootNote;
        default:
            return Routing::Target::None;
        }
    }

    void setSelectedScale(int defaultScale, bool force = false) override {
        if (_editScale || force) {
            _sequence->editScale(_scales[_selectedScale[_sequence->trackIndex()]], false, defaultScale);
        }
        _editScale = !_editScale;
    }

private:
    static const char *itemName(Item item) {
        switch (item) {
        case Name:              return "Name";
        case FirstStep:         return "First Step";
        case LastStep:          return "Last Step";
        case RunMode:           return "Run Mode";
        case Divisor:           return "Divisor";
        case ResetMeasure:      return "Reset Measure";
        case Scale:             return "Scale";
        case RootNote:          return "Root Note";
        case Last:              break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case Name:
             str(_sequence->name());
             break;
        case FirstStep:
            _sequence->printFirstStep(str);
            break;
        case LastStep:
            _sequence->printLastStep(str);
            break;
        case RunMode:
            _sequence->printRunMode(str);
            break;
        case Divisor:
            _sequence->printDivisor(str);
            break;
        case ResetMeasure:
            _sequence->printResetMeasure(str);
            break;
        case Scale: {
                int trackIndex = _sequence->trackIndex();
                bool isRouted = Routing::isRouted(Routing::Target::Scale, trackIndex);
                if (isRouted) {
                    _sequence->printScale(str);
                } else {
                    auto name = _scales[_selectedScale[trackIndex]] < 0 ? "Default" : Scale::name(_scales[_selectedScale[trackIndex]]);
                    str(name);
                }
            }
            break;
        case RootNote:
            _sequence->printRootNote(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Name:
            break;
        case FirstStep:
            _sequence->editFirstStep(value, shift);
            break;
        case LastStep:
            _sequence->editLastStep(value, shift);
            break;
        case RunMode:
            _sequence->editRunMode(value, shift);
            break;
        case Divisor:
            _sequence->editDivisor(value, shift);
            break;
        case ResetMeasure:
            _sequence->editResetMeasure(value, shift);
            break;
        case Scale: {
                int trackIndex = _sequence->trackIndex();
                bool isRouted = Routing::isRouted(Routing::Target::Scale, trackIndex);
                if (!isRouted) {
                    int trackIndex = _sequence->trackIndex();
                    _selectedScale[trackIndex] = clamp(_selectedScale[trackIndex] + value, 0, ScaleSelectionCount - 1);
                }
            }
            break;
        case RootNote:
            _sequence->editRootNote(value, shift);
            break;
        case Last:
            break;
        }
    }

    int indexedCountValue(Item item) const {
        switch (item) {
        case Name:
            break;
        case FirstStep:
        case LastStep:
            return 16;
        case RunMode:
            return int(Types::RunMode::Last);
        case Divisor:
        case ResetMeasure:
            return 16;
        case Scale:
            return Scale::Count + 1;
        case RootNote:
            return 12 + 1;
        case Last:
            break;
        }
        return -1;
    }

    int indexedValue(Item item) const {
        switch (item) {
        case Name:
            break;
        case FirstStep:
            return _sequence->firstStep();
        case LastStep:
            return _sequence->lastStep();
        case RunMode:
            return int(_sequence->runMode());
        case Divisor:
            return _sequence->indexedDivisor();
        case ResetMeasure:
            return _sequence->resetMeasure();
        case Scale:
            return _sequence->indexedScale();
        case RootNote:
            return _sequence->indexedRootNote();
        case Last:
            break;
        }
        return -1;
    }

    void setIndexedValue(Item item, int index) {
        switch (item) {
        case Name:
            break;
        case FirstStep:
            return _sequence->setFirstStep(index);
        case LastStep:
            return _sequence->setLastStep(index);
        case RunMode:
            return _sequence->setRunMode(Types::RunMode(index));
        case Divisor:
            return _sequence->setIndexedDivisor(index);
        case ResetMeasure:
            return _sequence->setResetMeasure(index);
        case Scale:
            return _sequence->setIndexedScale(index);
        case RootNote:
            return _sequence->setIndexedRootNote(index);
        case Last:
            break;
        }
    }

    LogicSequence *_sequence;
    private:
        std::array<int, ScaleSelectionCount> _scales;
        std::array<int, 8> _selectedScale;
        bool _editScale = false;

};
