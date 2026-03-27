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
static Generator::Mode activeGeneratorMode = Generator::Mode::Last;

void Generator::destroyActive() {
    switch (activeGeneratorMode) {
    case Mode::Euclidean:
        generatorContainer.destroy(&generatorContainer.as<EuclideanGenerator>());
        break;
    case Mode::Random:
        generatorContainer.destroy(&generatorContainer.as<RandomGenerator>());
        break;
    case Mode::Acid:
        generatorContainer.destroy(&generatorContainer.as<AcidGenerator>());
        break;
    case Mode::Chaos:
        generatorContainer.destroy(&generatorContainer.as<ChaosGenerator>());
        break;
    case Mode::InitLayer:
    case Mode::Last:
        break;
    }

    activeGeneratorMode = Mode::Last;
}

static void initSequence(SequenceBuilder &builder, const std::bitset<CONFIG_STEP_COUNT> &selected) {
    builder.clearSteps(selected);
}

Generator *Generator::execute(Generator::Mode mode, SequenceBuilder &builder, std::bitset<CONFIG_STEP_COUNT> &selected) {
    destroyActive();

    switch (mode) {
    case Mode::InitLayer:
        initSequence(builder, selected);
        return nullptr;
    case Mode::Euclidean:
        activeGeneratorMode = mode;
        return generatorContainer.create<EuclideanGenerator>(builder, euclideanParams);
    case Mode::Random:
        activeGeneratorMode = mode;
        return generatorContainer.create<RandomGenerator>(builder, randomParams, selected);
    case Mode::Acid:
        activeGeneratorMode = mode;
        return generatorContainer.create<AcidGenerator>(builder, acidParams, selected);
    case Mode::Chaos:
        activeGeneratorMode = mode;
        return generatorContainer.create<ChaosGenerator>(builder, chaosParams, selected);
    case Mode::Last:
        break;
    }

    return nullptr;
}
