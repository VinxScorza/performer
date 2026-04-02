#pragma once

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"

#include <array>
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <vector>

namespace ModelUtils {

template<typename Enum>
static Enum clampedEnum(Enum value) {
    return Enum(clamp(int(value), 0, int(Enum::Last) - 1));
}

static int adjusted(int value, int offset, int min, int max) {
    return clamp(value + offset, min, max);
}

template<typename Enum>
static Enum adjustedEnum(Enum value, int offset) {
    return Enum(adjusted(int(value), offset, 0, int(Enum::Last) - 1));
}

int adjustedByStep(int value, int offset, int step, bool shift);
int adjustedByPowerOfTwo(int value, int offset, bool shift);
int adjustedByDivisor(int value, int offset, bool shift);

void printYesNo(StringBuilder &str, bool value);
void printDivisor(StringBuilder &str, int value);
void printDivisorAbbr(StringBuilder &str, int value);

int divisorToIndex(int divisor);
int indexToDivisor(int index);

static int clampDivisor(int divisor) {
    return clamp(divisor, 1, 768);
}

template<typename Step, size_t N>
static void shiftSteps(std::array<Step, N> &steps, int direction) {
    if (direction == 1) {
        for (int i = int(steps.size()) - 2; i >= 0; --i) {
            std::swap(steps[i], steps[i + 1]);
        }
    } else if (direction == -1) {
        for (int i = 0; i < int(steps.size()) - 1; ++i) {
            std::swap(steps[i], steps[i + 1]);
        }
    }
}

template<typename Step, size_t N>
static void shiftSteps(std::array<Step, N> &steps, int first, int last, int direction)
{
    if (last - first <= 1) {
        return;
    }

    if (direction == 1) {
        for (int i = last - 2; i >= first; --i) {
            std::swap(steps[i], steps[i + 1]);
        }
    } else if (direction == -1) {
        for (int i = first; i < last - 1; ++i) {
            std::swap(steps[i], steps[i + 1]);
        }
    }
}

template<typename Step, size_t N>
static void shiftSteps(std::array<Step, N> &steps, const std::bitset<N> &selected, int first, int last, int direction)
{
    // [first...last-1] : size=last-first
    // direction L = -1 ; R = 1

    int distance = last - first;
    if (distance <= 1) {
        return;
    }

    int starting_point = -1; // from where we'll swap. -1 means everything is selected

    auto wrap = [first, last, distance] (int index) {
        while (index < first) {
            index += distance;
        }
        while (index >= last) {
            index -= distance;
        }
        return index;
    };

    if (direction == -1) {
        // Find a starting point (the closest step to the left that we don't shift)

        for(int idx=first; idx<last;idx++) {
            if (!selected[idx]) {
                starting_point = idx;
                break;;
            }
        }

        if (starting_point == - 1) {
            shiftSteps(steps, first, last, direction);
            return;
        }

        // scan and swap the whole range, from left to right
        for (int i = 1; i < distance; ++i) {
            int idx = wrap(starting_point + i);
            int previous_idx = wrap(idx - 1);

            if (selected[idx])
                std::swap(steps[previous_idx], steps[idx]);
        }
    }
    else if (direction == 1) {
        // Find a starting point (the closest step to the right that we don't shift)
        for(int idx=last-1; idx>=0;idx--) {
            if (!selected[idx]) {
                starting_point = idx;
                break;;
            }
        }

        if (starting_point == - 1) {
            shiftSteps(steps, first, last, direction);
            return;
        }

        // scan and swap the whole range, from right to left
        for (int i = 1; i < distance; ++i) {
            int idx = wrap(starting_point - i);
            int next_idx = wrap(idx + 1);

            if (selected[idx])
                std::swap(steps[next_idx], steps[idx]);
        }
    }

    return;
}

template<typename Step, size_t N>
static void duplicateSteps(std::array<Step, N> &steps, int firstStep, int lastStep) {
    for (int src = firstStep; src <= lastStep; ++src) {
        int dst = src + (lastStep - firstStep + 1);
        if (dst < int(steps.size())) {
            steps[dst] = steps[src];
        }
    }
}

template<typename Step, size_t N>
static void copySteps(
    const std::array<Step, N> &src, const std::bitset<N> &srcSelected,
    std::array<Step, N> &dst, const std::bitset<N> &dstSelected
) {
    auto nextSelected = [] (const std::bitset<N> &selected, int index) {
        if (selected.none()) {
            return (index + 1) % int(N);
        } else {
            do {
                index = (index + 1) % int(N);
            } while (!selected[index]);
            return index;
        }
    };

    int srcIndex = -1;

    for (size_t dstIndex = 0; dstIndex < N; ++dstIndex) {
        if (dstSelected.none() || dstSelected[dstIndex]) {
            srcIndex = nextSelected(srcSelected, srcIndex);
            dst[dstIndex] = src[srcIndex];
        }
    }
}

} // namespace ModelUtils
