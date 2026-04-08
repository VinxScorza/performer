#pragma once

#include "StepSelection.h"

#include <bitset>
#include <cstddef>

template <size_t N>
inline std::bitset<N> selectedOrAllSteps(const StepSelection<N> &stepSelection) {
    auto selected = stepSelection.selected();
    if (!selected.any()) {
        selected.set();
    }
    return selected;
}
