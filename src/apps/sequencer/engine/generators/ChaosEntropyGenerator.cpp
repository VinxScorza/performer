#include "ChaosEntropyGenerator.h"

#include "core/utils/Random.h"

#include <ctime>

ChaosEntropyGenerator::ChaosEntropyGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected) :
    Generator(builder),
    _params(params),
    _selected(selected)
{
    update();
}

const char *ChaosEntropyGenerator::paramName(int index) const {
    switch (index) {
    case 0: return "Seed";
    case 1: return "Amt";
    default: break;
    }
    return nullptr;
}

void ChaosEntropyGenerator::editParam(int index, int value, bool shift) {
    (void)shift;
    switch (index) {
    case 0:
        if (value != 0) {
            randomizeSeed();
        }
        break;
    case 1:
        setAmount(amount() + value);
        break;
    default:
        break;
    }
}

void ChaosEntropyGenerator::printParam(int index, StringBuilder &str) const {
    switch (index) {
    case 0:
        str("%08X", seed());
        break;
    case 1:
        str("%d%%", amount());
        break;
    default:
        break;
    }
}

void ChaosEntropyGenerator::init() {
    const uint32_t currentSeed = _params.seed;
    _params = Params();
    _params.seed = currentSeed;
    update();
}

void ChaosEntropyGenerator::randomizeParams() {
    randomizeSeed();
}

void ChaosEntropyGenerator::showOriginal() {
    _builder.showOriginal();
}

void ChaosEntropyGenerator::showPreview() {
    _builder.showOriginal();
    _builder.applyEntropy(_params.seed, _params.amount, _selected);
    _builder.showPreview();
}

void ChaosEntropyGenerator::update() {
    if (_builder.showingPreview()) {
        showPreview();
    }
}

void ChaosEntropyGenerator::randomizeSeed() {
    static uint32_t entropy = 0;

    if (entropy == 0) {
        entropy = uint32_t(time(NULL)) ^ uint32_t(clock()) ^ 0xD1B54A32u;
    }

    entropy = entropy * 1664525u + 1013904223u + uint32_t(clock());
    Random rng(entropy);
    _params.seed = rng.next();
    entropy = rng.next();
}
