#pragma once

#include "Config.h"

#include "Generator.h"

#include "core/math/Math.h"
#include <bitset>

class RandomGenerator : public Generator {
public:
    enum class Param {
        Seed,
        Smooth,
        Bias,
        Scale,
        Variation,
        Last
    };

    struct Params {
        uint32_t seed = 0;
        uint8_t smooth = 0;
        int8_t bias = 0;
        uint8_t scale = 5;
        uint8_t variation = 50;
    };

    RandomGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected);

    Mode mode() const override { return Mode::Random; }

    int paramCount() const override { return int(Param::Last); }
    const char *paramName(int index) const override;
    void editParam(int index, int value, bool shift) override;
    void printParam(int index, StringBuilder &str) const override;

    void init() override;
    void update() override;

    void randomizeSeed();

    // seed

    int seed() const { return _params.seed; }
    void setSeed(uint32_t seed) { _params.seed = seed; }

    // smooth

    int smooth() const { return _params.smooth; }
    void setSmooth(int smooth) { _params.smooth = clamp(smooth, 0, 10); }

    // bias

    int bias() const { return _params.bias; }
    void setBias(int bias) { _params.bias = clamp(bias, -10, 10); }

    // scale

    int scale() const { return _params.scale; }
    void setScale(int scale) { _params.scale = clamp(scale, 0, 100); }

    // variation

    int variation() const { return _params.variation; }
    void setVariation(int variation) { _params.variation = clamp(variation, 0, 100); }

    // pattern

    const GeneratorPattern &pattern() const { return _pattern; }
    int displayValue(int index) const;

private:
    Params &_params;
    GeneratorPattern _pattern;
    std::bitset<CONFIG_STEP_COUNT> &_selected;
};
