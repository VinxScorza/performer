#pragma once

#include "Config.h"

#include "SequenceBuilder.h"

#include "core/utils/StringBuilder.h"

#include <array>

typedef std::array<uint8_t, CONFIG_STEP_COUNT> GeneratorPattern;

class Generator {
public:
    enum class Mode {
        InitLayer,
        InitSteps,
        Euclidean,
        Random,
        Acid,
        Chaos,
        ChaosEntropy,
        Last
    };

    static const char *modeName(Mode mode) {
        switch (mode) {
        case Mode::InitLayer:   return "Init Layer";
        case Mode::InitSteps:   return "Init Steps";
        case Mode::Euclidean:   return "Euclidean";
        case Mode::Random:      return "Random";
        case Mode::Acid:        return "Acid";
        case Mode::Chaos:       return "Chaos";
        case Mode::ChaosEntropy:return "Entropy";
        case Mode::Last:        break;
        }
        return nullptr;
    }

    Generator(SequenceBuilder &builder) :
        _builder(builder)
    {}

    virtual ~Generator() = default;

    virtual Mode mode() const = 0;
    const char *name() const { return modeName(mode()); }

    // parameters

    virtual int paramCount() const = 0;
    virtual const char *paramName(int index) const = 0;
    virtual void editParam(int index, int value, bool shift) = 0;
    virtual void printParam(int index, StringBuilder &str) const = 0;

    virtual void init() {}
    virtual void randomizeParams() {}

    virtual void revert() {
        _builder.revert();
    }

    virtual void apply() {
        _builder.apply();
    }

    virtual void showOriginal() {
        _builder.showOriginal();
    }

    virtual void showPreview() {
        _builder.showPreview();
    }

    bool showingPreview() const {
        return _builder.showingPreview();
    }

    virtual void update() = 0;

    static Generator *execute(Generator::Mode mode, SequenceBuilder &builder, std::bitset<CONFIG_STEP_COUNT> &selected);
    static void destroyActive();

protected:
    SequenceBuilder &_builder;
};
