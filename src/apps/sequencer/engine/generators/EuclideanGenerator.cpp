#include "EuclideanGenerator.h"

#include "core/utils/Random.h"

#include <ctime>

EuclideanGenerator::EuclideanGenerator(SequenceBuilder &builder, Params& params) :
    Generator(builder),
    _params(params)
{
    update();
}

const char *EuclideanGenerator::paramName(int index) const {
    switch (Param(index)) {
    case Param::Steps:  return "Steps";
    case Param::Beats:  return "Beats";
    case Param::Offset: return "Offset";
    case Param::Last:   break;
    }
    return nullptr;
}

void EuclideanGenerator::editParam(int index, int value, bool shift) {
    switch (Param(index)) {
    case Param::Steps:  setSteps(steps() + value); break;
    case Param::Beats:  setBeats(beats() + value); break;
    case Param::Offset: setOffset(offset() + value); break;
    case Param::Last:   break;
    }
}

void EuclideanGenerator::printParam(int index, StringBuilder &str) const {
    switch (Param(index)) {
    case Param::Steps:  str("%d", steps()); break;
    case Param::Beats:  str("%d", beats()); break;
    case Param::Offset: str("%d", offset()); break;
    case Param::Last:   break;
    }
}

void EuclideanGenerator::init()
{
    setSteps(DefaultSteps);
    setBeats(DefaultBeats);
    setOffset(DefaultOffset);
    update();
}

void EuclideanGenerator::randomizeParams() {
    static uint32_t entropy = 0;

    if (entropy == 0) {
        entropy = uint32_t(time(NULL)) ^ uint32_t(clock()) ^ 0x91E10DA5u;
    }

    entropy = entropy * 1664525u + 1013904223u + uint32_t(clock());
    Random rng(entropy);

    setSteps(1 + int(rng.nextRange(CONFIG_STEP_COUNT)));
    setBeats(1 + int(rng.nextRange(steps())));
    setOffset(int(rng.nextRange(CONFIG_STEP_COUNT)));

    entropy = rng.next();
}

void EuclideanGenerator::update()  {
    _pattern = Rhythm::euclidean(_params.beats, _params.steps).shifted(_params.offset);

    _builder.setLength(_params.steps);

    for (size_t i = 0; i < CONFIG_STEP_COUNT; ++i) {
        _builder.setValue(i, _pattern[i % _pattern.size()] ? 1.f : 0.f);
    }
}
