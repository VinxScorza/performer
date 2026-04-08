#include "UnitTest.h"

#include "apps/sequencer/engine/generators/ChaosGenerator.h"
#include "apps/sequencer/engine/generators/EuclideanGenerator.h"
#include "apps/sequencer/engine/generators/SequenceBuilder.h"
#include "apps/sequencer/model/Project.h"

#include <bitset>

UNIT_TEST("GeneratorSemantics") {

    CASE("Chaos init preserves seed and scope while resetting generator defaults") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Note);

        std::bitset<CONFIG_STEP_COUNT> selected;
        ChaosSequenceBuilder builder(project, selected, ChaosSequenceBuilder::Scope::Pattern);

        ChaosGenerator::Params params;
        params.seed = 0x12345678u;
        params.amount = 37;
        params.targetMask = (1u << int(ChaosGenerator::Target::Gate)) |
                            (1u << int(ChaosGenerator::Target::Slide));
        params.scope = ChaosGenerator::Scope::Pattern;

        ChaosGenerator generator(builder, params, selected);
        generator.init();

        expectEqual(uint32_t(0x12345678u), generator.seed());
        expectTrue(generator.patternScope());
        expectEqual(100, generator.amount());
        expectTrue(generator.allTargetsEnabled());
    }

    CASE("Euclidean randomize params rerolls values while keeping them in valid ranges") {
        NoteSequence sequence;
        sequence.clear();

        NoteSequenceBuilder builder(sequence, NoteSequence::Layer::Gate);

        EuclideanGenerator::Params params;
        params.steps = 16;
        params.beats = 4;
        params.offset = 0;

        EuclideanGenerator generator(builder, params);

        const int initialSteps = generator.steps();
        const int initialBeats = generator.beats();
        const int initialOffset = generator.offset();

        bool changed = false;

        for (int i = 0; i < 8; ++i) {
            generator.randomizeParams();
            generator.update();

            expectTrue(generator.steps() >= 1);
            expectTrue(generator.steps() <= CONFIG_STEP_COUNT);
            expectTrue(generator.beats() >= 1);
            expectTrue(generator.beats() <= generator.steps());
            expectTrue(generator.offset() >= 0);
            expectTrue(generator.offset() < CONFIG_STEP_COUNT);

            if (generator.steps() != initialSteps ||
                generator.beats() != initialBeats ||
                generator.offset() != initialOffset) {
                changed = true;
            }
        }

        expectTrue(changed);
    }
}
