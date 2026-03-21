#include "RandomGenerator.h"

#include "core/utils/Random.h"
#include <bitset>
#include <cstddef>
#include <ctime>

RandomGenerator::RandomGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected) :
    Generator(builder),
    _params(params),
    _selected(selected)
{
    update();
}

const char *RandomGenerator::paramName(int index) const {
    switch (Param(index)) {
    case Param::Seed:   return "Seed";
    case Param::Smooth: return "Smooth";
    case Param::Bias:   return "Bias";
    case Param::Scale:  return "Range";
    case Param::Variation: return "Var";
    case Param::Last:   break;
    }
    return nullptr;
}

void RandomGenerator::editParam(int index, int value, bool shift) {
    switch (Param(index)) {
    case Param::Seed:
        if (value != 0) {
            randomizeSeed();
        }
        break;
    case Param::Smooth: setSmooth(smooth() + value); break;
    case Param::Bias:   setBias(bias() + value); break;
    case Param::Scale:  setScale(scale() + value); break;
    case Param::Variation: setVariation(variation() + value); break;
    case Param::Last:   break;
    }
}

void RandomGenerator::printParam(int index, StringBuilder &str) const {
    switch (Param(index)) {
    case Param::Seed:   str("%08X", seed()); break;
    case Param::Smooth: str("%d", smooth()); break;
    case Param::Bias:   str("%d", bias()); break;
    case Param::Scale:  str("%d%%", scale()); break;
    case Param::Variation: str("%d%%", variation()); break;
    case Param::Last:   break;
    }
}

void RandomGenerator::init() {
    _params = Params();
    randomizeSeed();
    update();
}

void RandomGenerator::randomizeParams() {
    randomizeSeed();

    Random rng(_params.seed ^ 0x6C078965u);
    setSmooth(int(rng.nextRange(11)));
    setBias(int(rng.nextRange(21)) - 10);
    setScale(int(rng.nextRange(101)));
    setVariation(100);
}

void RandomGenerator::randomizeSeed() {
    static uint32_t entropy = 0;

    if (entropy == 0) {
        entropy = uint32_t(time(NULL)) ^ uint32_t(clock()) ^ 0xA341316Cu;
    }

    entropy = entropy * 1664525u + 1013904223u + uint32_t(clock());
    Random rng(entropy);
    _params.seed = rng.next();
    entropy = rng.next();
}

void RandomGenerator::update() {
    Random rng(_params.seed);

    int size = _pattern.size();

    for (int i = 0; i < size; ++i) {
        if (_selected[i]) {
            _pattern[i] = rng.nextRange(255);
        } else {
            _pattern[i] = 0;
        }
    }

    for (int iteration = 0; iteration < _params.smooth; ++iteration) {
        for (int i = 0; i < size; ++i) {
            if (_selected[i]) {
                _pattern[i] = (4 * _pattern[i] + _pattern[(i - 1 + size) % size] + _pattern[(i + 1) % size] + 3) / 6;
            }
        }
    }

    int bias = (_params.bias * 255) / 10;
    int scale = _params.scale;
    float variation = _params.variation * 0.01f;

    for (int i = 0; i < size; ++i) {
        if (_selected[i]) {
            int value = _pattern[i];
            // value = ((value - 127) * scale) / 10 + 127 + bias;
            value = ((value + bias - 127) * scale) / 10 + 127;
            _pattern[i] = clamp(value, 0, 255);
        } 
    }

    for (size_t i = 0; i < _pattern.size(); ++i) {
        if (_selected[i]) {
            float original = _builder.originalValue(i);
            float generated = _pattern[i] * (1.f / 255.f);
            float blended = original + (generated - original) * variation;
            _builder.setValue(i, blended);
        }
    }
}

int RandomGenerator::displayValue(int index) const {
    float value = showingPreview() ? _builder.value(index) : _builder.originalValue(index);
    return clamp(int(std::round(value * 255.f)), 0, 255);
}
