#include "UnitTest.h"

#include "apps/sequencer/model/ModelUtils.h"

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

}
