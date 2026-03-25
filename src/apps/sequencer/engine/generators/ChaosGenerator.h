#pragma once

#include "Config.h"

#include "Generator.h"
#include "SequenceBuilder.h"

#include "core/math/Math.h"

#include <bitset>
#include <cstdint>

class ChaosGenerator : public Generator {
public:
    enum class Target {
        Gate,
        GateOffset,
        GateProbability,
        Retrigger,
        Length,
        LengthVariationRange,
        LengthVariationProbability,
        RetriggerProbability,
        Note,
        Slide,
        NoteVariationRange,
        NoteVariationProbability,
        BypassScale,
        Condition,
        Last
    };

    struct Params {
        uint32_t seed = 0;
        uint8_t amount = 100;
        uint16_t targetMask = (1u << int(Target::Last)) - 1u;
    };

    ChaosGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected);

    Mode mode() const override { return Mode::Chaos; }

    int paramCount() const override { return 2; }
    const char *paramName(int index) const override;
    void editParam(int index, int value, bool shift) override;
    void printParam(int index, StringBuilder &str) const override;

    void init() override;
    void randomizeParams() override;
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
        _params.targetMask = enabled ? ((1u << int(Target::Last)) - 1u) : 0u;
    }

    bool allTargetsEnabled() const {
        return _params.targetMask == ((1u << int(Target::Last)) - 1u);
    }

    static const char *targetCellLabel(Target target);

private:
    int blendRandomValue(NoteSequence::Layer layer, int originalValue, class Random &rng) const;
    bool blendRandomBool(bool originalValue, class Random &rng) const;

    Params &_params;
    ChaosSequenceBuilder &_chaosBuilder;
};
