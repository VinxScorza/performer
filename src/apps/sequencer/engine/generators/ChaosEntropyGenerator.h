#pragma once

#include "Config.h"

#include "EntropyTargets.h"
#include "Generator.h"

#include "core/math/Math.h"

#include <bitset>
#include <cstdint>

class ChaosEntropyGenerator : public Generator {
public:
    using Target = EntropyTarget;

    struct Params {
        uint32_t seed = 0;
        uint8_t amount = 100;
        uint16_t targetMask = DefaultEntropyTargetMask;
    };

    ChaosEntropyGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected);

    Mode mode() const override { return Mode::ChaosEntropy; }

    int paramCount() const override { return 2; }
    const char *paramName(int index) const override;
    void editParam(int index, int value, bool shift) override;
    void printParam(int index, StringBuilder &str) const override;

    void init() override;
    void randomizeParams() override;
    void showOriginal() override;
    void showPreview() override;
    void update() override;

    void randomizeSeed();

    int amount() const { return _params.amount; }
    void setAmount(int amount) { _params.amount = clamp(amount, 0, 100); }

    uint32_t seed() const { return _params.seed; }

    bool targetEnabled(Target target) const {
        return (_params.targetMask >> int(target)) & 0x1;
    }

    void setTargetEnabled(Target target, bool enabled) {
        if (enabled) {
            _params.targetMask |= (1u << int(target));
        } else {
            _params.targetMask &= ~(1u << int(target));
        }
    }

    void toggleTarget(Target target) {
        setTargetEnabled(target, !targetEnabled(target));
    }

    void setAllTargets(bool enabled) {
        _params.targetMask = enabled ? DefaultEntropyTargetMask : 0u;
    }

    void setTargetMask(uint16_t targetMask) {
        _params.targetMask = targetMask & DefaultEntropyTargetMask;
    }

    bool allTargetsEnabled() const {
        return _params.targetMask == DefaultEntropyTargetMask;
    }

    static const char *targetCellLabel(Target target);

private:
    Params &_params;
    std::bitset<CONFIG_STEP_COUNT> &_selected;
};
