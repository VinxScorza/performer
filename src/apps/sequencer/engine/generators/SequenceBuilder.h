#pragma once

#include "model/Project.h"
#include "model/NoteSequence.h"
#include "model/CurveSequence.h"
#include "model/StochasticSequence.h"
#include "model/LogicSequence.h"
#include "model/ArpSequence.h"

#include "core/utils/Random.h"

#include <algorithm>
#include <bitset>
#include <vector>

class SequenceBuilder {
public:
    virtual ~SequenceBuilder() = default;

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

    virtual void clearSteps(const std::bitset<CONFIG_STEP_COUNT> &selected) = 0;
    virtual void copyStep(int fromIndex, int toIndex) = 0;

    virtual void clearLayer(const std::bitset<CONFIG_STEP_COUNT> &selected) = 0;
    virtual void applyEntropy(uint32_t seed, int amount, const std::bitset<CONFIG_STEP_COUNT> &selected) = 0;
};

template<typename T>
inline void restoreClearedStepDefaults(T &, int) {}

template<>
inline void restoreClearedStepDefaults<StochasticSequence>(StochasticSequence &sequence, int stepIndex) {
    if (stepIndex >= 0 && stepIndex < 12) {
        sequence.step(stepIndex).setNote(stepIndex);
    }
}

template<>
inline void restoreClearedStepDefaults<ArpSequence>(ArpSequence &sequence, int stepIndex) {
    if (stepIndex >= 0 && stepIndex < 12) {
        sequence.step(stepIndex).setNote(stepIndex);
    }
}

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

    void clearSteps(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        if (!selected.any()) {
            for (int i = _preview.firstStep(); i <= _preview.lastStep(); ++i) {
                _preview.step(i).clear();
                restoreClearedStepDefaults(_preview, i);
            }
            return;
        }

        for (int i = 0; i < int(_preview.steps().size()); ++i) {
            if (selected[i]) {
                _preview.step(i).clear();
                restoreClearedStepDefaults(_preview, i);
            }
        }
    }

    void copyStep(int fromIndex, int toIndex) override {
        _preview.step(_preview.firstStep() + toIndex) = _original.step(_original.firstStep() + fromIndex);
    }

    void clearLayer(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        for (int i = 0; i < int(_preview.steps().size()); ++i) {
            const bool targetStep = selected.any() ? selected[i] : (i >= _preview.firstStep() && i <= _preview.lastStep());
            if (!targetStep) {
                continue;
            }
            _preview.step(i).setLayerValue(_layer, _default);
        }
    }

    void applyEntropy(uint32_t seed, int amount, const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        const int blend = clamp(amount, 0, 100);
        const float t = blend * 0.01f;

        Random rng(seed);
        for (int stepIndex = 0; stepIndex < int(_preview.steps().size()); ++stepIndex) {
            const bool targetStep = selected.any() ? selected[stepIndex] : (stepIndex >= _preview.firstStep() && stepIndex <= _preview.lastStep());
            if (!targetStep) {
                continue;
            }

            const auto &originalStep = _original.step(stepIndex);
            auto &previewStep = _preview.step(stepIndex);

            for (int layerIndex = 0; layerIndex < int(T::Layer::Last); ++layerIndex) {
                auto layer = static_cast<typename T::Layer>(layerIndex);
                const auto range = T::layerRange(layer);
                const int originalValue = originalStep.layerValue(layer);
                const int randomValue = range.min + int(rng.nextRange(range.max - range.min + 1));

                int value = int(std::round(originalValue + (randomValue - originalValue) * t));
                if (value == originalValue && randomValue != originalValue && blend > 0) {
                    value += randomValue > originalValue ? 1 : -1;
                }

                value = clamp(value, range.min, range.max);
                previewStep.setLayerValue(layer, value);
            }
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

    void clearSteps(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        if (!selected.any()) {
            if (_applyMode == ApplyMode::Phrase) {
                for (int i = _preview.firstStep(); i <= _preview.lastStep(); ++i) {
                    auto &step = _preview.step(i);
                    step.clear();
                    step.setGate(false);
                    step.setSlide(false);
                }
            } else {
                for (int i = _preview.firstStep(); i <= _preview.lastStep(); ++i) {
                    _preview.step(i).clear();
                }
            }
            return;
        }

        for (int i = 0; i < int(_preview.steps().size()); ++i) {
            if (!selected[i]) {
                continue;
            }

            auto &step = _preview.step(i);
            step.clear();
            if (_applyMode == ApplyMode::Phrase) {
                step.setGate(false);
                step.setSlide(false);
            }
        }
    }

    void copyStep(int fromIndex, int toIndex) override {
        _preview.step(_preview.firstStep() + toIndex) = _original.step(_original.firstStep() + fromIndex);
    }

    void clearLayer(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        if (_applyMode == ApplyMode::Phrase) {
            for (int i = 0; i < int(_preview.steps().size()); ++i) {
                if (selected.any() ? selected[i] : isTargetStep(i)) {
                    auto &step = _preview.step(i);
                    step.setGate(false);
                    step.setSlide(false);
                }
            }
            return;
        }

        const int defaultValue = NoteSequence::layerDefaultValue(_layer);
        for (int i = 0; i < int(_preview.steps().size()); ++i) {
            if (selected.any() ? selected[i] : isTargetStep(i)) {
                _preview.step(i).setLayerValue(_layer, defaultValue);
            }
        }
    }

    void applyEntropy(uint32_t, int, const std::bitset<CONFIG_STEP_COUNT> &) override {
        // Entropy is not supported for Acid builder paths.
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
    enum class Scope {
        Sequence,
        Pattern
    };

    ChaosSequenceBuilder(Project &project, std::bitset<CONFIG_STEP_COUNT> &selected, Scope scope) :
        _project(project),
        _selected(selected),
        _selectedTrackIndex(project.selectedTrackIndex()),
        _patternIndex(project.selectedPatternIndex()),
        _scope(scope)
    {
        int slot = 0;
        for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
            const auto &track = _project.track(trackIndex);
            if (track.trackMode() != Track::TrackMode::Note) {
                continue;
            }
            if (_scope == Scope::Sequence && trackIndex != _selectedTrackIndex) {
                continue;
            }

            _trackIndices[slot] = trackIndex;
            auto &backup = _tracks[slot];
            const auto &sequence = _project.noteSequence(trackIndex, _patternIndex);

            if (_selected.any()) {
                for (int stepIndex = 0; stepIndex < CONFIG_STEP_COUNT; ++stepIndex) {
                    if (_selected[stepIndex]) {
                        backup.targetSteps.push_back(uint8_t(stepIndex));
                    }
                }
            } else {
                for (int stepIndex = sequence.firstStep(); stepIndex <= sequence.lastStep(); ++stepIndex) {
                    backup.targetSteps.push_back(uint8_t(stepIndex));
                }
            }

            backup.originalSteps.reserve(backup.targetSteps.size());
            for (uint8_t stepIndex : backup.targetSteps) {
                backup.originalSteps.push_back(sequence.step(stepIndex));
            }
            if (trackIndex == _selectedTrackIndex) {
                _selectedTrackSlot = slot;
            }
            ++slot;
        }
        _trackCount = slot;
        if (_selectedTrackSlot < 0 && _trackCount > 0) {
            _selectedTrackSlot = 0;
        }
    }

    void setScope(Scope scope) {
        _scope = scope;
    }

    Scope scope() const {
        return _scope;
    }

    void revert() override {
        for (int i = 0; i < _trackCount; ++i) {
            restoreOriginalSteps(i);
        }
        _showingPreview = false;
    }

    void apply() override {
        for (int i = 0; i < _trackCount; ++i) {
            if (targetTrack(i)) {
                captureCurrentSteps(i);
            } else {
                restoreOriginalSteps(i);
            }
        }
        _showingPreview = true;
    }

    void showOriginal() override {
        for (int i = 0; i < _trackCount; ++i) {
            restoreOriginalSteps(i);
        }
        _showingPreview = false;
    }

    void showPreview() override {
        _showingPreview = true;
    }

    bool showingPreview() const override {
        return _showingPreview;
    }

    int originalLength() const override {
        const auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        return sequence.lastStep() - sequence.firstStep() + 1;
    }

    float originalValue(int index) const override {
        const auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        return sequence.step(sequence.firstStep() + index).gate() ? 1.f : 0.f;
    }

    int length() const override {
        const auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        return sequence.lastStep() - sequence.firstStep() + 1;
    }

    void setLength(int length) override {
        auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        sequence.setFirstStep(0);
        sequence.setLastStep(length - 1);
    }

    float value(int index) const override {
        const auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        return sequence.step(sequence.firstStep() + index).gate() ? 1.f : 0.f;
    }

    void setValue(int index, float value) override {
        auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        sequence.step(sequence.firstStep() + index).setGate(value >= 0.5f);
    }

    void clearSteps(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        for (int trackSlot = 0; trackSlot < _trackCount; ++trackSlot) {
            if (!targetTrack(trackSlot)) {
                continue;
            }

            auto &backup = _tracks[trackSlot];
            for (size_t i = 0; i < backup.targetSteps.size(); ++i) {
                if (!selected.any() || selected[backup.targetSteps[i]]) {
                    auto &sequence = _project.noteSequence(_trackIndices[trackSlot], _patternIndex);
                    sequence.step(backup.targetSteps[i]).clear();
                }
            }
        }
    }

    void copyStep(int fromIndex, int toIndex) override {
        auto &sequence = _project.noteSequence(_selectedTrackIndex, _patternIndex);
        sequence.step(sequence.firstStep() + toIndex) = sequence.step(sequence.firstStep() + fromIndex);
    }

    void clearLayer(const std::bitset<CONFIG_STEP_COUNT> &selected) override {
        (void)selected;
        for (int i = 0; i < _trackCount; ++i) {
            restoreOriginalSteps(i);
        }
    }

    void applyEntropy(uint32_t, int, const std::bitset<CONFIG_STEP_COUNT> &) override {
        // Entropy is not supported for Chaos builder paths.
    }

    void resetToOriginal() { showOriginal(); }

    bool hasSelection() const { return _selected.any(); }

    bool isTargetStep(int stepIndex) const {
        if (_selected.any()) {
            return _selected[stepIndex];
        }
        if (_selectedTrackSlot < 0) {
            return false;
        }
        const auto &targetSteps = _tracks[_selectedTrackSlot].targetSteps;
        return std::find(targetSteps.begin(), targetSteps.end(), uint8_t(stepIndex)) != targetSteps.end();
    }

    int noteTrackCount() const { return _trackCount; }

    bool targetTrack(int trackSlot) const {
        return _scope == Scope::Pattern || trackSlot == _selectedTrackSlot;
    }

    int targetStepCount(int trackSlot) const { return int(_tracks[trackSlot].targetSteps.size()); }
    const NoteSequence::Step &originalStep(int trackSlot, int targetIndex) const { return _tracks[trackSlot].originalSteps[targetIndex]; }
    NoteSequence::Step &liveStep(int trackSlot, int targetIndex) {
        auto &sequence = _project.noteSequence(_trackIndices[trackSlot], _patternIndex);
        return sequence.step(_tracks[trackSlot].targetSteps[targetIndex]);
    }

private:
    struct TrackBackup {
        std::vector<uint8_t> targetSteps;
        std::vector<NoteSequence::Step> originalSteps;
    };

    void restoreOriginalSteps(int trackSlot) {
        auto &sequence = _project.noteSequence(_trackIndices[trackSlot], _patternIndex);
        const auto &backup = _tracks[trackSlot];
        for (size_t i = 0; i < backup.targetSteps.size(); ++i) {
            sequence.step(backup.targetSteps[i]) = backup.originalSteps[i];
        }
    }

    void captureCurrentSteps(int trackSlot) {
        auto &sequence = _project.noteSequence(_trackIndices[trackSlot], _patternIndex);
        auto &backup = _tracks[trackSlot];
        for (size_t i = 0; i < backup.targetSteps.size(); ++i) {
            backup.originalSteps[i] = sequence.step(backup.targetSteps[i]);
        }
    }

    Project &_project;
    std::array<int, CONFIG_TRACK_COUNT> _trackIndices = {};
    std::array<TrackBackup, CONFIG_TRACK_COUNT> _tracks = {};
    std::bitset<CONFIG_STEP_COUNT> &_selected;
    int _selectedTrackIndex = 0;
    int _patternIndex = 0;
    int _trackCount = 0;
    int _selectedTrackSlot = -1;
    Scope _scope = Scope::Sequence;
    bool _showingPreview = false;
};
