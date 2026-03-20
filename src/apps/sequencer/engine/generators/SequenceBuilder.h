#pragma once

#include "model/NoteSequence.h"
#include "model/CurveSequence.h"
#include "model/StochasticSequence.h"
#include "model/LogicSequence.h"
#include "model/ArpSequence.h"

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
