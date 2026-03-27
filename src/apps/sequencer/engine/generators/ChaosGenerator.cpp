#include "ChaosGenerator.h"

#include "core/utils/Random.h"

#include <ctime>

ChaosGenerator::ChaosGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &/*selected*/) :
    Generator(builder),
    _params(params),
    _chaosBuilder(static_cast<ChaosSequenceBuilder &>(builder))
{
    update();
}

const char *ChaosGenerator::paramName(int index) const {
    switch (index) {
    case 0: return "Seed";
    case 1: return "Amt";
    default: break;
    }
    return nullptr;
}

void ChaosGenerator::editParam(int index, int value, bool shift) {
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

void ChaosGenerator::printParam(int index, StringBuilder &str) const {
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

void ChaosGenerator::init() {
    _params = Params();
    randomizeSeed();
    update();
}

void ChaosGenerator::randomizeParams() {
    randomizeSeed();
}

void ChaosGenerator::showOriginal() {
    _chaosBuilder.showOriginal();
}

void ChaosGenerator::showPreview() {
    _chaosBuilder.showOriginal();
    applyPreviewToTargetTracks();
    _chaosBuilder.showPreview();
}

void ChaosGenerator::randomizeSeed() {
    static uint32_t entropy = 0;

    if (entropy == 0) {
        entropy = uint32_t(time(NULL)) ^ uint32_t(clock()) ^ 0x9E3779B9u;
    }

    entropy = entropy * 1664525u + 1013904223u + uint32_t(clock());
    Random rng(entropy);
    _params.seed = rng.next();
    entropy = rng.next();
}

const char *ChaosGenerator::targetCellLabel(Target target) {
    switch (target) {
    case Target::Gate:                       return "Gate";
    case Target::GateOffset:                 return "G Offset";
    case Target::GateProbability:            return "G Prob";
    case Target::Retrigger:                  return "Retrig";
    case Target::Length:                     return "Length";
    case Target::LengthVariationRange:       return "L Range";
    case Target::LengthVariationProbability: return "L Prob";
    case Target::RetriggerProbability:       return "R Prob";
    case Target::Note:                       return "Note";
    case Target::Slide:                      return "N Slide";
    case Target::NoteVariationRange:         return "N Range";
    case Target::NoteVariationProbability:   return "N Prob";
    case Target::BypassScale:                return "N Bypass";
    case Target::Condition:                  return "Cond";
    case Target::Last:                       break;
    }
    return "";
}

bool ChaosGenerator::blendRandomBool(bool originalValue, Random &rng) const {
    if (_params.amount == 0) {
        return originalValue;
    }

    if (rng.nextRange(100) >= _params.amount) {
        return originalValue;
    }

    return rng.nextRange(2) != 0;
}

int ChaosGenerator::blendRandomValue(NoteSequence::Layer layer, int originalValue, Random &rng) const {
    const auto range = NoteSequence::layerRange(layer);
    const int randomValue = range.min + int(rng.nextRange(range.max - range.min + 1));
    const float t = _params.amount * 0.01f;

    if (t <= 0.f) {
        return originalValue;
    }

    int value = int(std::round(originalValue + (randomValue - originalValue) * t));
    if (value == originalValue && randomValue != originalValue) {
        value += randomValue > originalValue ? 1 : -1;
    }

    return clamp(value, range.min, range.max);
}

void ChaosGenerator::updateTrack(int trackSlot, Random &rng) const {
    for (int targetIndex = 0; targetIndex < _chaosBuilder.targetStepCount(trackSlot); ++targetIndex) {
        auto &step = _chaosBuilder.liveStep(trackSlot, targetIndex);
        const auto &originalStep = _chaosBuilder.originalStep(trackSlot, targetIndex);

        if (targetEnabled(Target::Gate)) {
            step.setGate(blendRandomBool(originalStep.gate(), rng));
        }
        if (targetEnabled(Target::GateOffset)) {
            step.setGateOffset(blendRandomValue(NoteSequence::Layer::GateOffset, originalStep.gateOffset(), rng));
        }
        if (targetEnabled(Target::GateProbability)) {
            step.setGateProbability(blendRandomValue(NoteSequence::Layer::GateProbability, originalStep.gateProbability(), rng));
        }
        if (targetEnabled(Target::Retrigger)) {
            step.setRetrigger(blendRandomValue(NoteSequence::Layer::Retrigger, originalStep.retrigger(), rng));
        }
        if (targetEnabled(Target::RetriggerProbability)) {
            step.setRetriggerProbability(blendRandomValue(NoteSequence::Layer::RetriggerProbability, originalStep.retriggerProbability(), rng));
        }
        if (targetEnabled(Target::Length)) {
            step.setLength(blendRandomValue(NoteSequence::Layer::Length, originalStep.length(), rng));
        }
        if (targetEnabled(Target::LengthVariationRange)) {
            step.setLengthVariationRange(blendRandomValue(NoteSequence::Layer::LengthVariationRange, originalStep.lengthVariationRange(), rng));
        }
        if (targetEnabled(Target::LengthVariationProbability)) {
            step.setLengthVariationProbability(blendRandomValue(NoteSequence::Layer::LengthVariationProbability, originalStep.lengthVariationProbability(), rng));
        }
        if (targetEnabled(Target::Note)) {
            step.setNote(blendRandomValue(NoteSequence::Layer::Note, originalStep.note(), rng));
        }
        if (targetEnabled(Target::Slide)) {
            step.setSlide(blendRandomBool(originalStep.slide(), rng));
        }
        if (targetEnabled(Target::NoteVariationRange)) {
            step.setNoteVariationRange(blendRandomValue(NoteSequence::Layer::NoteVariationRange, originalStep.noteVariationRange(), rng));
        }
        if (targetEnabled(Target::NoteVariationProbability)) {
            step.setNoteVariationProbability(blendRandomValue(NoteSequence::Layer::NoteVariationProbability, originalStep.noteVariationProbability(), rng));
        }
        if (targetEnabled(Target::BypassScale)) {
            step.setBypassScale(blendRandomBool(originalStep.bypassScale(), rng));
        }
        if (targetEnabled(Target::Condition)) {
            step.setCondition(Types::Condition(blendRandomValue(NoteSequence::Layer::Condition, int(originalStep.condition()), rng)));
        }
    }
}

void ChaosGenerator::applyPreviewToTargetTracks() const {
    if (patternScope()) {
        Random seedRng(_params.seed);
        for (int trackSlot = 0; trackSlot < _chaosBuilder.noteTrackCount(); ++trackSlot) {
            if (!_chaosBuilder.targetTrack(trackSlot)) {
                continue;
            }
            Random rng(seedRng.next());
            updateTrack(trackSlot, rng);
        }
        return;
    }

    Random rng(_params.seed);
    for (int trackSlot = 0; trackSlot < _chaosBuilder.noteTrackCount(); ++trackSlot) {
        if (_chaosBuilder.targetTrack(trackSlot)) {
            updateTrack(trackSlot, rng);
            break;
        }
    }
}

void ChaosGenerator::update() {
    _chaosBuilder.setScope(patternScope() ? ChaosSequenceBuilder::Scope::Pattern : ChaosSequenceBuilder::Scope::Sequence);
    if (_chaosBuilder.showingPreview()) {
        showPreview();
    }
}
