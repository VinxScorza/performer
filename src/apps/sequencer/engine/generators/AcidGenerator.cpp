#include "AcidGenerator.h"

#include "core/utils/Random.h"

#include <ctime>
#include <utility>

namespace {
struct SlideCandidate {
    int stepIndex = -1;
    int score = 0;
};

struct GateCandidate {
    int stepIndex = -1;
    int score = 0;
};
}

AcidGenerator::AcidGenerator(SequenceBuilder &builder, Params &params, std::bitset<CONFIG_STEP_COUNT> &selected) :
    Generator(builder),
    _params(params),
    _acidBuilder(static_cast<AcidSequenceBuilder &>(builder))
{
    (void)selected;
    update();
}

int AcidGenerator::paramCount() const {
    if (_acidBuilder.applyMode() == AcidSequenceBuilder::ApplyMode::Phrase) {
        return int(Param::Last);
    }

    switch (_acidBuilder.layer()) {
    case NoteSequence::Layer::Gate:
    case NoteSequence::Layer::Slide:
    case NoteSequence::Layer::Note:
        return 3;
    default:
        return int(Param::Last);
    }
}

AcidGenerator::Param AcidGenerator::visibleParam(int index) const {
    if (_acidBuilder.applyMode() == AcidSequenceBuilder::ApplyMode::Phrase) {
        return Param(index);
    }

    switch (_acidBuilder.layer()) {
    case NoteSequence::Layer::Gate:
        switch (index) {
        case 0: return Param::Seed;
        case 1: return Param::Density;
        case 2: return Param::Variation;
        default: break;
        }
        break;
    case NoteSequence::Layer::Slide:
        switch (index) {
        case 0: return Param::Seed;
        case 1: return Param::Slide;
        case 2: return Param::Variation;
        default: break;
        }
        break;
    case NoteSequence::Layer::Note:
        switch (index) {
        case 0: return Param::Seed;
        case 1: return Param::Range;
        case 2: return Param::Variation;
        default: break;
        }
        break;
    default:
        return Param(index);
    }

    return Param::Last;
}

const char *AcidGenerator::paramName(int index) const {
    switch (visibleParam(index)) {
    case Param::Seed:      return "Seed";
    case Param::Density:   return "Dens";
    case Param::Slide:     return "Slide";
    case Param::Range:     return "Range";
    case Param::Variation: return "Var";
    case Param::Last:      break;
    }
    return nullptr;
}

void AcidGenerator::editParam(int index, int value, bool shift) {
    (void)shift;

    switch (visibleParam(index)) {
    case Param::Seed:
        if (value != 0) {
            randomizeSeed();
        }
        break;
    case Param::Density:
        setDensity(density() + value);
        break;
    case Param::Slide:
        setSlide(slide() + value);
        break;
    case Param::Range:
        setRange(range() + value);
        break;
    case Param::Variation:
        setVariation(variation() + value);
        break;
    case Param::Last:
        break;
    }
}

void AcidGenerator::printParam(int index, StringBuilder &str) const {
    switch (visibleParam(index)) {
    case Param::Seed:
        str("%08X", _params.seed);
        break;
    case Param::Density:
        str("%d%%", density());
        break;
    case Param::Slide:
        str("%d%%", slide());
        break;
    case Param::Range:
        str("%d%%", range());
        break;
    case Param::Variation:
        str("%d%%", variation());
        break;
    case Param::Last:
        break;
    }
}

void AcidGenerator::init() {
    _params = Params();
    randomizeSeed();
    update();
}

void AcidGenerator::randomizeParams() {
    randomizeSeed();

    Random rng(_params.seed ^ 0xB5297A4Du);
    setDensity(int(rng.nextRange(101)));
    setSlide(int(rng.nextRange(101)));
    setRange(int(rng.nextRange(101)));
    setVariation(100);
}

void AcidGenerator::randomizeSeed() {
    static uint32_t entropy = 0;

    if (entropy == 0) {
        entropy = uint32_t(time(NULL)) ^ uint32_t(clock()) ^ 0xA341316Cu;
    }

    entropy = entropy * 1664525u + 1013904223u + uint32_t(clock());
    Random rng(entropy);
    _params.seed = rng.next();
    entropy = rng.next();
}

void AcidGenerator::randomizeContextParams() {
    randomizeSeed();

    Random rng(_params.seed ^ 0x68E31DA4u);

    for (int i = 1; i < paramCount(); ++i) {
        switch (visibleParam(i)) {
        case Param::Density:
            setDensity(int(rng.nextRange(101)));
            break;
        case Param::Slide:
            setSlide(int(rng.nextRange(101)));
            break;
        case Param::Range:
            setRange(int(rng.nextRange(101)));
            break;
        case Param::Variation:
        case Param::Seed:
        case Param::Last:
            break;
        }
    }
}

int AcidGenerator::countGatedSteps(const NoteSequence &sequence, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) const {
    int count = 0;
    for (int i = 0; i < targetCount; ++i) {
        if (sequence.step(targetSteps[i]).gate()) {
            ++count;
        }
    }
    return count;
}

int AcidGenerator::averageOriginalNote(const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) const {
    const auto &sequence = _acidBuilder.originalSequence();

    int sum = 0;
    int count = 0;
    for (int i = 0; i < targetCount; ++i) {
        const auto &step = sequence.step(targetSteps[i]);
        if (step.gate()) {
            sum += step.note();
            ++count;
        }
    }

    if (count == 0) {
        return 0;
    }

    return sum / count;
}

int AcidGenerator::desiredGateCount(int targetCount) const {
    return clamp((density() * targetCount + 50) / 100, 0, targetCount);
}

int AcidGenerator::desiredSlideCount(int targetCount, int candidateCount) const {
    return clamp((slide() * targetCount + 50) / 100, 0, candidateCount);
}

int AcidGenerator::melodicSpan() const {
    return (range() * 24 + 50) / 100;
}

int AcidGenerator::maxStepDelta() const {
    return (range() * 12) / 100;
}

bool AcidGenerator::keepOriginal(Random &rng) const {
    return variation() < 100 && rng.nextRange(100) >= uint32_t(variation());
}

int AcidGenerator::generatedGateScore(Random &rng, int position, int runLength, int motifLength, int motifGateBias) const {
    int score = 64;
    score += motifGateBias;
    score += (position % 4 == 0) ? 18 : ((position % 4 == 2) ? 8 : 0);
    score += (position % motifLength == 0) ? 10 : 0;
    score += runLength == 1 ? 12 : 0;
    score -= runLength >= 2 ? 24 : 0;
    score += int(rng.nextRange(25)) - 12;
    return score;
}

int AcidGenerator::nextNoteDelta(Random &rng, int motifLength, int motifBias, int maxStep) const {
    maxStep = clamp(maxStep, 0, 12);
    int choice = int(rng.nextRange(100));
    int delta = 0;

    if (maxStep <= 0) {
        return 0;
    }

    if (maxStep == 1) {
        if (choice < 70) {
            return 0;
        }
        return rng.nextBinary() ? -1 : 1;
    }

    if (choice < 22) {
        delta = 0;
    } else if (choice < 58) {
        delta = rng.nextBinary() ? -1 : 1;
    } else {
        const int upperSmall = std::min(maxStep, 2 + (range() / 6));
        const int upperLarge = std::min(maxStep, 3 + (range() / 4) + (motifLength > 4 ? 1 : 0));

        if (choice < 84 || upperLarge <= 2) {
            int magnitude = 2;
            if (upperSmall > 2) {
                magnitude = 2 + int(rng.nextRange(upperSmall - 1));
            }
            delta = rng.nextBinary() ? -magnitude : magnitude;
        } else {
            int magnitude = std::max(3, upperSmall);
            if (upperLarge > magnitude) {
                magnitude += int(rng.nextRange(upperLarge - magnitude + 1));
            }
            delta = rng.nextBinary() ? -magnitude : magnitude;
        }
    }

    if ((motifBias & 1) == 0 && delta != 0 && rng.nextRange(100) < 35) {
        delta += delta > 0 ? 1 : -1;
    }

    return delta;
}

void AcidGenerator::updateLayerGate(Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) {
    auto &preview = _acidBuilder.previewSequence();
    const auto &original = _acidBuilder.originalSequence();

    if (density() <= 0) {
        for (int i = 0; i < targetCount; ++i) {
            auto &step = preview.step(targetSteps[i]);
            step.setGate(false);
            step.setSlide(false);
        }
        return;
    }

    if (density() >= 100) {
        for (int i = 0; i < targetCount; ++i) {
            preview.step(targetSteps[i]).setGate(true);
        }
        return;
    }

    const int motifLength = 2 + int(rng.nextRange(4));
    std::array<GateCandidate, CONFIG_STEP_COUNT> candidates = {};
    int candidateCount = 0;
    int runLength = 0;

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        const auto &originalStep = original.step(stepIndex);
        auto &candidate = candidates[candidateCount++];

        const int motifBias = int((rng.next() >> ((i % motifLength) * 3)) & 0x1f) - 10;
        int score = generatedGateScore(rng, i, runLength, motifLength, motifBias);
        if (density() > 0 && density() < 100 && keepOriginal(rng) && originalStep.gate()) {
            score += 18;
        }
        candidate.stepIndex = stepIndex;
        candidate.score = score;

        runLength = originalStep.gate() ? runLength + 1 : 0;
    }

    for (int i = 0; i < targetCount; ++i) {
        auto &step = preview.step(targetSteps[i]);
        step.setGate(false);
        step.setSlide(false);
    }

    const int desiredGates = desiredGateCount(targetCount);
    for (int i = 0; i < desiredGates; ++i) {
        int bestIndex = i;
        for (int j = i + 1; j < candidateCount; ++j) {
            if (candidates[j].score > candidates[bestIndex].score) {
                bestIndex = j;
            }
        }
        std::swap(candidates[i], candidates[bestIndex]);
        preview.step(candidates[i].stepIndex).setGate(true);
    }
}

void AcidGenerator::updateLayerNote(Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) {
    auto &preview = _acidBuilder.previewSequence();
    const auto &original = _acidBuilder.originalSequence();

    if (countGatedSteps(preview, targetSteps, targetCount) == 0) {
        return;
    }

    const int anchor = averageOriginalNote(targetSteps, targetCount);
    const int span = melodicSpan();
    const int minNote = clamp(anchor - span, NoteSequence::Note::Min, NoteSequence::Note::Max);
    const int maxNote = clamp(anchor + span, NoteSequence::Note::Min, NoteSequence::Note::Max);
    const int maxStep = maxStepDelta();
    const int motifLength = 2 + int(rng.nextRange(4));

    int currentNote = clamp(anchor, minNote, maxNote);

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        auto &step = preview.step(stepIndex);
        const auto &originalStep = original.step(stepIndex);

        if (!step.gate()) {
            continue;
        }

        const int motifBias = int((rng.next() >> ((i % motifLength) * 2)) & 0x0f);
        currentNote = clamp(currentNote + nextNoteDelta(rng, motifLength, motifBias, maxStep), minNote, maxNote);

        int note = currentNote;
        if (keepOriginal(rng) && originalStep.gate()) {
            note = originalStep.note();
        }

        step.setNote(note);
    }
}

void AcidGenerator::updateLayerSlide(Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) {
    auto &preview = _acidBuilder.previewSequence();
    const auto &original = _acidBuilder.originalSequence();

    if (slide() <= 0) {
        for (int i = 0; i < targetCount; ++i) {
            preview.step(targetSteps[i]).setSlide(false);
        }
        return;
    }

    const int motifLength = 2 + int(rng.nextRange(4));
    std::array<SlideCandidate, CONFIG_STEP_COUNT> candidates = {};
    int candidateCount = 0;

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        auto &step = preview.step(stepIndex);
        const auto &originalStep = original.step(stepIndex);
        step.setSlide(false);

        if (step.gate()) {
            int nextIndex = -1;
            for (int j = i + 1; j < targetCount; ++j) {
                if (preview.step(targetSteps[j]).gate()) {
                    nextIndex = targetSteps[j];
                    break;
                }
            }

            if (nextIndex >= 0) {
                const int interval = std::abs(preview.step(nextIndex).note() - step.note());
                const int motifBias = int((rng.next() >> ((i % motifLength) * 2)) & 0x0f);
                int score = 64;
                score += interval <= 1 ? 30 : (interval <= 3 ? 16 : -8);
                score += (motifBias % 3 == 0) ? 10 : 0;
                score += motifLength > 4 ? 4 : 0;
                score += int(rng.nextRange(33)) - 16;
                if (slide() > 0 && slide() < 100 && keepOriginal(rng) && originalStep.slide()) {
                    score += 18;
                }

                candidates[candidateCount].stepIndex = stepIndex;
                candidates[candidateCount].score = score;
                ++candidateCount;
            }
        }
    }

    if (candidateCount == 0) {
        return;
    }

    const int desiredSlides = slide() >= 100 ? candidateCount : desiredSlideCount(targetCount, candidateCount);

    for (int i = 0; i < desiredSlides; ++i) {
        int bestIndex = i;
        for (int j = i + 1; j < candidateCount; ++j) {
            if (candidates[j].score > candidates[bestIndex].score) {
                bestIndex = j;
            }
        }
        std::swap(candidates[i], candidates[bestIndex]);
        preview.step(candidates[i].stepIndex).setSlide(true);
    }
}

void AcidGenerator::updatePhrase(Random &rng, const std::array<int, CONFIG_STEP_COUNT> &targetSteps, int targetCount) {
    auto &preview = _acidBuilder.previewSequence();
    const auto &original = _acidBuilder.originalSequence();

    if (density() <= 0) {
        for (int i = 0; i < targetCount; ++i) {
            auto &step = preview.step(targetSteps[i]);
            step.setGate(false);
            step.setSlide(false);
        }
        return;
    }

    const int motifLength = 3 + int(rng.nextRange(4));
    const int anchor = averageOriginalNote(targetSteps, targetCount);
    const int span = melodicSpan();
    const int minNote = clamp(anchor - span, NoteSequence::Note::Min, NoteSequence::Note::Max);
    const int maxNote = clamp(anchor + span, NoteSequence::Note::Min, NoteSequence::Note::Max);
    const int maxStep = maxStepDelta();

    int runLength = 0;
    int currentNote = clamp(anchor, minNote, maxNote);

    std::array<bool, CONFIG_STEP_COUNT> gateState = {};
    std::array<int, CONFIG_STEP_COUNT> noteState = {};
    std::array<GateCandidate, CONFIG_STEP_COUNT> gateCandidates = {};
    int gateCandidateCount = 0;

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        const auto &originalStep = original.step(stepIndex);
        auto &step = preview.step(stepIndex);

        const int motifGateBias = int((rng.next() >> ((i % motifLength) * 3)) & 0x1f) - 10;
        int score = generatedGateScore(rng, i, runLength, motifLength, motifGateBias);
        if (density() > 0 && density() < 100 && keepOriginal(rng) && originalStep.gate()) {
            score += 18;
        }
        gateCandidates[gateCandidateCount].stepIndex = i;
        gateCandidates[gateCandidateCount].score = score;
        ++gateCandidateCount;

        gateState[i] = false;
        step.setGate(false);
        step.setSlide(false);
        noteState[i] = originalStep.note();
        runLength = originalStep.gate() ? runLength + 1 : 0;
    }

    const int desiredGates = desiredGateCount(targetCount);
    for (int i = 0; i < desiredGates; ++i) {
        int bestIndex = i;
        for (int j = i + 1; j < gateCandidateCount; ++j) {
            if (gateCandidates[j].score > gateCandidates[bestIndex].score) {
                bestIndex = j;
            }
        }
        std::swap(gateCandidates[i], gateCandidates[bestIndex]);
        gateState[gateCandidates[i].stepIndex] = true;
    }

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        const auto &originalStep = original.step(stepIndex);
        auto &step = preview.step(stepIndex);

        if (!gateState[i]) {
            step.setGate(false);
            step.setSlide(false);
            noteState[i] = originalStep.note();
            continue;
        }

        step.setGate(true);

        const int motifNoteBias = int((rng.next() >> ((i % motifLength) * 2)) & 0x0f);
        currentNote = clamp(currentNote + nextNoteDelta(rng, motifLength, motifNoteBias, maxStep), minNote, maxNote);

        int note = currentNote;
        if (keepOriginal(rng) && originalStep.gate()) {
            note = originalStep.note();
        }

        noteState[i] = note;
        step.setNote(note);
    }

    if (slide() <= 0) {
        for (int i = 0; i < targetCount; ++i) {
            preview.step(targetSteps[i]).setSlide(false);
        }
        return;
    }

    std::array<SlideCandidate, CONFIG_STEP_COUNT> candidates = {};
    int candidateCount = 0;

    for (int i = 0; i < targetCount; ++i) {
        const int stepIndex = targetSteps[i];
        auto &step = preview.step(stepIndex);
        const auto &originalStep = original.step(stepIndex);

        if (!gateState[i]) {
            step.setSlide(false);
            continue;
        }

        int next = -1;
        for (int j = i + 1; j < targetCount; ++j) {
            if (gateState[j]) {
                next = j;
                break;
            }
        }

        if (next >= 0) {
            const int interval = std::abs(noteState[next] - noteState[i]);
            const int motifSlideBias = int((rng.next() >> ((i % motifLength) * 2)) & 0x0f);
            int score = 64;
            score += interval <= 1 ? 30 : (interval <= 3 ? 16 : -8);
            score += (motifSlideBias % 3 == 0) ? 10 : 0;
            score += motifLength > 4 ? 4 : 0;
            score += int(rng.nextRange(33)) - 16;
            if (slide() > 0 && slide() < 100 && keepOriginal(rng) && originalStep.slide()) {
                score += 18;
            }
            candidates[candidateCount].stepIndex = stepIndex;
            candidates[candidateCount].score = score;
            ++candidateCount;
        }
    }

    if (candidateCount == 0) {
        return;
    }

    const int desiredSlides = slide() >= 100 ? candidateCount : desiredSlideCount(targetCount, candidateCount);

    for (int i = 0; i < desiredSlides; ++i) {
        int bestIndex = i;
        for (int j = i + 1; j < candidateCount; ++j) {
            if (candidates[j].score > candidates[bestIndex].score) {
                bestIndex = j;
            }
        }
        std::swap(candidates[i], candidates[bestIndex]);
        preview.step(candidates[i].stepIndex).setSlide(true);
    }
}

void AcidGenerator::update() {
    _acidBuilder.resetPreview();

    Random rng(_params.seed);
    std::array<int, CONFIG_STEP_COUNT> targetSteps = {};
    const int targetCount = _acidBuilder.collectTargetSteps(targetSteps);

    if (targetCount == 0) {
        return;
    }

    switch (_acidBuilder.applyMode()) {
    case AcidSequenceBuilder::ApplyMode::Layer:
        switch (_acidBuilder.layer()) {
        case NoteSequence::Layer::Gate:
            updateLayerGate(rng, targetSteps, targetCount);
            break;
        case NoteSequence::Layer::Slide:
            updateLayerSlide(rng, targetSteps, targetCount);
            break;
        case NoteSequence::Layer::Note:
        default:
            updateLayerNote(rng, targetSteps, targetCount);
            break;
        }
        break;
    case AcidSequenceBuilder::ApplyMode::Phrase:
        updatePhrase(rng, targetSteps, targetCount);
        break;
    }

    for (size_t i = 0; i < _pattern.size(); ++i) {
        _pattern[i] = displayValue(i);
    }
}

int AcidGenerator::displayValue(int index) const {
    return _acidBuilder.displayValue(index, showingPreview());
}
