#include "Generator.h"

#include "AcidGenerator.h"
#include "ChaosGenerator.h"
#include "EuclideanGenerator.h"
#include "RandomGenerator.h"

#include "core/utils/Container.h"

static Container<EuclideanGenerator, RandomGenerator, AcidGenerator, ChaosGenerator> generatorContainer;
static EuclideanGenerator::Params euclideanParams;
static RandomGenerator::Params randomParams;
static AcidGenerator::Params acidParams;
static ChaosGenerator::Params chaosParams;

static void initSequence(SequenceBuilder &builder, const std::bitset<CONFIG_STEP_COUNT> &selected) {
    builder.clearSteps(selected);
}

Generator *Generator::execute(Generator::Mode mode, SequenceBuilder &builder, std::bitset<CONFIG_STEP_COUNT> &selected) {
    switch (mode) {
    case Mode::InitLayer:
        initSequence(builder, selected);
        return nullptr;
    case Mode::Euclidean:
        return generatorContainer.create<EuclideanGenerator>(builder, euclideanParams);
    case Mode::Random:
        return generatorContainer.create<RandomGenerator>(builder, randomParams, selected);
    case Mode::Acid:
        return generatorContainer.create<AcidGenerator>(builder, acidParams, selected);
    case Mode::Chaos:
        return generatorContainer.create<ChaosGenerator>(builder, chaosParams, selected);
    case Mode::Last:
        break;
    }

    return nullptr;
}
