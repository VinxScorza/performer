#pragma once

#include "Config.h"

#include "Generator.h"
#include "SequenceBuilder.h"

#include "core/math/Math.h"

#include <array>
#include <bitset>

class AcidGenerator : public Generator {
public:
    enum class Param {
        Seed,
        Density,
        Slide,
        Range,
        Variation,
        Last
    };

    struct Params {
        uint32_t seed = 0;
        uint8_t density = 55;
        uint8_t slide = 25;
        uint8_t range = 35;
        uint8_t variation = 100;
    };

    AcidGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected);

    Mode mode() const override { return Mode::Acid; }

    int paramCount() const override { return int(Param::Last); }
    const char *paramName(int index) const override;
    void editParam(int index, int value, bool shift) override;
    void printParam(int index, StringBuilder &str) const override;

    void init() override;
    void randomizeParams() override;
    void update() override;

    void randomizeSeed();

    int density() const { return _params.density; }
    void setDensity(int density) { _params.density = clamp(density, 0, 100); }

    int slide() const { return _params.slide; }
    void setSlide(int slide) { _params.slide = clamp(slide, 0, 100); }

    int range() const { return _params.range; }
    void setRange(int range) { _params.range = clamp(range, 0, 100); }

    int variation() const { return _params.variation; }
    void setVariation(int variation) { _params.variation = clamp(variation, 0, 100); }

    int displayValue(int index) const;

private:
    int countGatedSteps(const NoteSequence &sequence, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) const;
    int averageOriginalNote(const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) const;
    int desiredGateCount(int targetCount) const;
    int desiredSlideCount(int targetCount, int candidateCount) const;
    int melodicSpan() const;
    int maxStepDelta() const;
    bool keepOriginal(class Random &rng) const;
    int generatedGateScore(class Random &rng, int position, int runLength, int motifLength, int motifGateBias) const;
    int nextNoteDelta(class Random &rng, int motifLength, int motifBias, int maxStep) const;

    void updateLayerGate(class Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount);
    void updateLayerNote(class Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount);
    void updateLayerSlide(class Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount);
    void updatePhrase(class Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount);

    Params &_params;
    AcidSequenceBuilder &_acidBuilder;
    GeneratorPattern _pattern;
};
