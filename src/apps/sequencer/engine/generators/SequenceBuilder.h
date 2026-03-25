#pragma once

#include "model/NoteSequence.h"
#include "model/CurveSequence.h"
#include "model/StochasticSequence.h"
#include "model/LogicSequence.h"
#include "model/ArpSequence.h"

#include <bitset>

class SequenceBuilder {
public:
    virtual void revert() = 0;
    virtual void apply() = 0;
    virtual void showOriginal() = 0;
    virtual void showPreview() = 0;
    virtual bool showingPreview() const = 0;

    // original sequence

    virtual int originalLength() const = 0;
    virtual float originalValue(int index) const = 0;

    // edit sequence

    virtual int length() const = 0;
    virtual void setLength(int length) = 0;

    virtual float value(int index) const = 0;
    virtual void setValue(int index, float value) = 0;

    virtual void clearSteps() = 0;
    virtual void copyStep(int fromIndex, int toIndex) = 0;

    virtual void clearLayer() = 0;
};

template<typename T>
class SequenceBuilderImpl : public SequenceBuilder {
public:
    SequenceBuilderImpl(T &sequence, typename T::Layer layer) :
        _edit(sequence),
        _original(sequence),
        _preview(sequence),
        _layer(layer),
        _range(T::layerRange(layer)),
        _default(T::layerDefaultValue(layer))
    {}

    void revert() override {
        _edit = _original;
        _preview = _original;
        _showingPreview = false;
    }

    void apply() override {
        _edit = _preview;
        _original = _preview;
        _showingPreview = true;
    }

    void showOriginal() override {
        _edit = _original;
        _showingPreview = false;
    }

    void showPreview() override {
        _edit = _preview;
        _showingPreview = true;
    }

    bool showingPreview() const override {
        return _showingPreview;
    }

    int originalLength() const override {
        return _original.lastStep() - _original.firstStep() + 1;
    }

    float originalValue(int index) const override {
        int layerValue = _original.step(_original.firstStep() + index).layerValue(_layer);
        return float(layerValue - _range.min) / (_range.max - _range.min);
    }

    int length() const override {
        return _preview.lastStep() - _preview.firstStep() + 1;
    }

    void setLength(int length) override {
        _preview.setFirstStep(0);
        _preview.setLastStep(length - 1);
    }

    float value(int index) const override {
        int layerValue = _preview.step(_preview.firstStep() + index).layerValue(_layer);
        return float(layerValue - _range.min) / (_range.max - _range.min);
    }

    void setValue(int index, float value) override {
        int layerValue = std::round(value * (_range.max - _range.min) + _range.min);
        _preview.step(_preview.firstStep() + index).setLayerValue(_layer, layerValue);
    }

    void clearSteps() override {
        _preview.clearSteps();
    }

    void copyStep(int fromIndex, int toIndex) override {
        _preview.step(_preview.firstStep() + toIndex) = _original.step(_original.firstStep() + fromIndex);
    }

    void clearLayer() override {
        for (auto &step : _preview.steps()) {
            step.setLayerValue(_layer, _default);
        }
    }

private:
    T &_edit;
    T _original;
    T _preview;
    typename T::Layer _layer;
    Types::LayerRange _range;
    int _default;
    bool _showingPreview = false;
};

typedef SequenceBuilderImpl<NoteSequence> NoteSequenceBuilder;
typedef SequenceBuilderImpl<CurveSequence> CurveSequenceBuilder;
typedef SequenceBuilderImpl<StochasticSequence> StochasticSequenceBuilder;
typedef SequenceBuilderImpl<LogicSequence> LogicSequenceBuilder;
typedef SequenceBuilderImpl<ArpSequence> ArpSequenceBuilder;

class AcidSequenceBuilder : public SequenceBuilder {
public:
    enum class ApplyMode : uint8_t {
        Layer,
        Phrase,
    };

    AcidSequenceBuilder(NoteSequence &sequence, NoteSequence::Layer layer, ApplyMode applyMode, std::bitset<CONFIG_STEP_COUNT> &selected) :
        _edit(sequence),
        _original(sequence),
        _preview(sequence),
        _layer(layer),
        _applyMode(applyMode),
        _selected(selected)
    {}

    void revert() override {
        _edit = _original;
        _preview = _original;
        _showingPreview = false;
    }

    void apply() override {
        _edit = _preview;
        _original = _preview;
        _showingPreview = true;
    }

    void showOriginal() override {
        _edit = _original;
        _showingPreview = false;
    }

    void showPreview() override {
        _edit = _preview;
        _showingPreview = true;
    }

    bool showingPreview() const override {
        return _showingPreview;
    }

    int originalLength() const override {
        return _original.lastStep() - _original.firstStep() + 1;
    }

    float originalValue(int index) const override {
        return displayValue(index, false) * (1.f / 255.f);
    }

    int length() const override {
        return _preview.lastStep() - _preview.firstStep() + 1;
    }

    void setLength(int length) override {
        _preview.setFirstStep(0);
        _preview.setLastStep(length - 1);
    }

    float value(int index) const override {
        return displayValue(index, true) * (1.f / 255.f);
    }

    void setValue(int index, float value) override {
        const int stepIndex = _preview.firstStep() + index;
        if (stepIndex < 0 || stepIndex >= int(_preview.steps().size())) {
            return;
        }

        auto &step = _preview.step(stepIndex);

        if (_applyMode == ApplyMode::Phrase) {
            step.setGate(value >= 0.5f);
            if (!step.gate()) {
                step.setSlide(false);
            }
            return;
        }

        const auto range = NoteSequence::layerRange(_layer);
        const int layerValue = std::round(value * (range.max - range.min) + range.min);
        step.setLayerValue(_layer, layerValue);
    }

    void clearSteps() override {
        _preview.clearSteps();
    }

    void copyStep(int fromIndex, int toIndex) override {
        _preview.step(_preview.firstStep() + toIndex) = _original.step(_original.firstStep() + fromIndex);
    }

    void clearLayer() override {
        if (_applyMode == ApplyMode::Phrase) {
            for (int i = 0; i < int(_preview.steps().size()); ++i) {
                if (isTargetStep(i)) {
                    auto &step = _preview.step(i);
                    step.setGate(false);
                    step.setSlide(false);
                }
            }
            return;
        }

        const int defaultValue = NoteSequence::layerDefaultValue(_layer);
        for (int i = 0; i < int(_preview.steps().size()); ++i) {
            if (isTargetStep(i)) {
                _preview.step(i).setLayerValue(_layer, defaultValue);
            }
        }
    }

    void resetPreview() {
        _preview = _original;
    }

    ApplyMode applyMode() const { return _applyMode; }
    NoteSequence::Layer layer() const { return _layer; }

    bool hasSelection() const { return _selected.any(); }

    bool isTargetStep(int stepIndex) const {
        if (_selected.any()) {
            return _selected[stepIndex];
        }
        return stepIndex >= _original.firstStep() && stepIndex <= _original.lastStep();
    }

    int collectTargetSteps(std::array<int, CONFIG_STEP_COUNT> &indices) const {
        int count = 0;
        for (int i = 0; i < int(indices.size()); ++i) {
            if (isTargetStep(i)) {
                indices[count++] = i;
            }
        }
        return count;
    }

    const NoteSequence &originalSequence() const { return _original; }
    const NoteSequence &previewSequence() const { return _preview; }
    NoteSequence &previewSequence() { return _preview; }

    int displayValue(int index, bool preview) const {
        const auto &sequence = preview ? _preview : _original;
        const auto &step = sequence.step(index);

        auto noteValue = [&] () {
            if (!step.gate()) {
                return 0;
            }

            const auto range = NoteSequence::layerRange(NoteSequence::Layer::Note);
            return clamp(int(std::round((step.note() - range.min) * 255.f / float(range.max - range.min))), 0, 255);
        };

        if (_applyMode == ApplyMode::Phrase) {
            return noteValue();
        }

        switch (_layer) {
        case NoteSequence::Layer::Gate:
            return step.gate() ? 255 : 0;
        case NoteSequence::Layer::Slide:
            return step.gate() && step.slide() ? 255 : 0;
        case NoteSequence::Layer::Note:
            return noteValue();
        default:
            return noteValue();
        }
    }

private:
    NoteSequence &_edit;
    NoteSequence _original;
    NoteSequence _preview;
    NoteSequence::Layer _layer;
    ApplyMode _applyMode;
    std::bitset<CONFIG_STEP_COUNT> &_selected;
    bool _showingPreview = false;
};

class ChaosSequenceBuilder : public SequenceBuilder {
public:
    ChaosSequenceBuilder(NoteSequence &sequence, std::bitset<CONFIG_STEP_COUNT> &selected) :
        _edit(sequence),
        _original(sequence),
        _preview(sequence),
        _selected(selected)
    {}

    void revert() override {
        _edit = _original;
        _preview = _original;
        _showingPreview = false;
    }

    void apply() override {
        _edit = _preview;
        _original = _preview;
        _showingPreview = true;
    }

    void showOriginal() override {
        _edit = _original;
        _showingPreview = false;
    }

    void showPreview() override {
        _edit = _preview;
        _showingPreview = true;
    }

    bool showingPreview() const override {
        return _showingPreview;
    }

    int originalLength() const override {
        return _original.lastStep() - _original.firstStep() + 1;
    }

    float originalValue(int index) const override {
        return _original.step(_original.firstStep() + index).gate() ? 1.f : 0.f;
    }

    int length() const override {
        return _preview.lastStep() - _preview.firstStep() + 1;
    }

    void setLength(int length) override {
        _preview.setFirstStep(0);
        _preview.setLastStep(length - 1);
    }

    float value(int index) const override {
        return _preview.step(_preview.firstStep() + index).gate() ? 1.f : 0.f;
    }

    void setValue(int index, float value) override {
        _preview.step(_preview.firstStep() + index).setGate(value >= 0.5f);
    }

    void clearSteps() override {
        _preview.clearSteps();
    }

    void copyStep(int fromIndex, int toIndex) override {
        _preview.step(_preview.firstStep() + toIndex) = _original.step(_original.firstStep() + fromIndex);
    }

    void clearLayer() override {
        _preview = _original;
    }

    void resetPreview() {
        _preview = _original;
    }

    bool hasSelection() const { return _selected.any(); }

    bool isTargetStep(int stepIndex) const {
        if (_selected.any()) {
            return _selected[stepIndex];
        }
        return stepIndex >= _original.firstStep() && stepIndex <= _original.lastStep();
    }

    const NoteSequence &originalSequence() const { return _original; }
    const NoteSequence &previewSequence() const { return _preview; }
    NoteSequence &previewSequence() { return _preview; }

private:
    NoteSequence &_edit;
    NoteSequence _original;
    NoteSequence _preview;
    std::bitset<CONFIG_STEP_COUNT> &_selected;
    bool _showingPreview = false;
};
