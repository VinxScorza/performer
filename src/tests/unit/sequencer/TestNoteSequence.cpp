#include "UnitTest.h"

#include "apps/sequencer/model/ClipBoard.h"
#include "apps/sequencer/model/ModelUtils.h"
#include "apps/sequencer/model/Project.h"

#include <array>
#include <bitset>

UNIT_TEST("NoteSequence") {

    CASE("shiftSteps rotates full 64-step range right") {
        std::array<int, 64> steps = {};
        steps[0] = 10;
        steps[63] = 99;

        ModelUtils::shiftSteps(steps, 0, 64, 1);

        expectEqual(99, steps[0]);
        expectEqual(10, steps[1]);
        expectEqual(0, steps[63]);
    }

    CASE("shiftSteps rotates full 64-step range left") {
        std::array<int, 64> steps = {};
        steps[0] = 10;
        steps[63] = 99;

        ModelUtils::shiftSteps(steps, 0, 64, -1);

        expectEqual(0, steps[0]);
        expectEqual(99, steps[62]);
        expectEqual(10, steps[63]);
    }

    CASE("shiftSteps rotates subrange ending at step 64 right") {
        std::array<int, 64> steps = {};
        steps[60] = 1;
        steps[63] = 2;

        ModelUtils::shiftSteps(steps, 60, 64, 1);

        expectEqual(2, steps[60]);
        expectEqual(1, steps[61]);
        expectEqual(0, steps[63]);
    }

    CASE("shiftSteps moves selected steps right inside non-zero subrange") {
        std::array<int, 64> steps = {};
        for (int i = 16; i < 20; ++i) {
            steps[i] = i - 15;
        }

        std::bitset<64> selected;
        selected.set(17);
        selected.set(18);

        ModelUtils::shiftSteps(steps, selected, 16, 20, 1);

        expectEqual(1, steps[16]);
        expectEqual(4, steps[17]);
        expectEqual(2, steps[18]);
        expectEqual(3, steps[19]);
    }

    CASE("shiftSteps moves selected steps left inside non-zero subrange") {
        std::array<int, 64> steps = {};
        for (int i = 16; i < 20; ++i) {
            steps[i] = i - 15;
        }

        std::bitset<64> selected;
        selected.set(17);
        selected.set(18);

        ModelUtils::shiftSteps(steps, selected, 16, 20, -1);

        expectEqual(2, steps[16]);
        expectEqual(3, steps[17]);
        expectEqual(1, steps[18]);
        expectEqual(4, steps[19]);
    }

    CASE("duplicateSelectedSteps copies contiguous selection after source range") {
        std::array<int, 64> steps = {};
        for (int i = 10; i <= 14; ++i) {
            steps[i] = i;
        }

        std::bitset<64> selected;
        for (int i = 10; i <= 14; ++i) {
            selected.set(i);
        }

        auto duplicated = ModelUtils::duplicateSelectedSteps(steps, selected);

        for (int i = 15; i <= 19; ++i) {
            expectEqual(i - 5, steps[i]);
            expectTrue(duplicated[i]);
        }
        expectEqual(size_t(5), duplicated.count());
    }

    CASE("duplicateSelectedSteps preserves gaps in non-contiguous selection") {
        std::array<int, 64> steps = {};
        steps[10] = 1;
        steps[12] = 2;
        steps[14] = 3;

        std::bitset<64> selected;
        selected.set(10);
        selected.set(12);
        selected.set(14);

        auto duplicated = ModelUtils::duplicateSelectedSteps(steps, selected);

        expectEqual(1, steps[15]);
        expectEqual(0, steps[16]);
        expectEqual(2, steps[17]);
        expectEqual(0, steps[18]);
        expectEqual(3, steps[19]);
        expectTrue(duplicated[15]);
        expectTrue(!duplicated[16]);
        expectTrue(duplicated[17]);
        expectTrue(!duplicated[18]);
        expectTrue(duplicated[19]);
        expectEqual(size_t(3), duplicated.count());
    }

    CASE("duplicateSelectedSequenceSteps returns empty when destination is out of range") {
        NoteSequence sequence;
        sequence.step(63).setNote(12);

        std::bitset<CONFIG_STEP_COUNT> selected;
        selected.set(63);

        auto duplicated = ModelUtils::duplicateSelectedSequenceSteps(sequence, selected);

        expectEqual(size_t(0), duplicated.count());
        expectEqual(12, sequence.step(63).note());
    }

    CASE("selected step clipboard requires destination selection") {
        Project project;
        project.clear();
        ClipBoard clipBoard(project);

        NoteSequence src;
        NoteSequence dst;
        for (int i = 10; i <= 14; ++i) {
            src.step(i).setNote(i);
        }
        dst.step(0).setNote(42);

        std::bitset<CONFIG_STEP_COUNT> srcSelected;
        for (int i = 10; i <= 14; ++i) {
            srcSelected.set(i);
        }
        std::bitset<CONFIG_STEP_COUNT> dstSelected;

        clipBoard.copyNoteSequenceSteps(src, srcSelected);

        expectTrue(clipBoard.pasteNoteSequenceStepsRequiresDestination(dstSelected));
        clipBoard.pasteNoteSequenceSteps(dst, dstSelected);
        expectEqual(42, dst.step(0).note());

        for (int i = 15; i <= 19; ++i) {
            dstSelected.set(i);
        }
        expectTrue(!clipBoard.pasteNoteSequenceStepsRequiresDestination(dstSelected));
        clipBoard.pasteNoteSequenceSteps(dst, dstSelected);
        for (int i = 15; i <= 19; ++i) {
            expectEqual(i - 5, dst.step(i).note());
        }
    }

}
