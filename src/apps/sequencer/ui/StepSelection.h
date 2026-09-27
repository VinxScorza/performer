#pragma once

#include "Key.h"

#include <bitset>
#include <functional>

#include <cstdint>
#include <cstdlib>

template<size_t N>
class StepSelection {
public:
    void keyDown(KeyEvent &event, int stepOffset) {
        const auto &key = event.key();

        if (key.pageModifier()) {
            return;
        }

        if (key.isStep()) {
            if (_mode == Mode::Immediate && key.shiftModifier()) {
                // switch to persist mode
                _selected.reset();
                _mode = Mode::Persist;
                _first = -1;
            }
            if (_mode == Mode::Persist && !key.shiftModifier()) {
                // switch to immediate mode
                _selected.reset();
                _mode = Mode::Immediate;
                _first = -1;
            }

            int stepIndex = stepOffset + key.step();

            if (_selected.none()) {
                _first = stepIndex;
            }

            _selected.flip(stepIndex);

            event.consume();
        }
    }

    void keyUp(KeyEvent &event, int stepOffset) {
        const auto &key = event.key();

        if (key.pageModifier()) {
            return;
        }

        if (key.isStep() && _mode == Mode::Immediate) {
            int stepIndex = stepOffset + key.step();

            _selected.reset(stepIndex);

            if (_first >= 0 && !_selected[_first]) {
                _first = firstSetIndex();
            }

            event.consume();
        }
    }

    void keyPress(KeyPressEvent &event, int stepOffset) {
        const auto &key = event.key();

        if (key.pageModifier() || !key.shiftModifier()) {
            return;
        }

        if (key.isShift() && event.count() == 2) {
            if (none()) {
                selectAll();
            } else if (_mode == Mode::Persist) {
                clear();
            }
            event.consume();
        }

        if (key.isStep() && event.count() == 2 && _mode == Mode::Persist) {
            int stepIndex = stepOffset + key.step();
            int otherStep = otherPressedStepKey(key.state(), key.step());
            if (otherStep >= 0) {
                int firstIndex = stepOffset + otherStep;
                int interval = stepIndex - firstIndex;
                for (int i = firstIndex; i >= 0 && i < int(_selected.size()); i += interval) {
                    _selected.set(i);
                }
                event.consume();
            } else {
                selectEqualSteps(stepIndex);
                event.consume();
            }
        }
    }

    void clear() {
        _selected.reset();
        _mode = Mode::Immediate;
        _first = -1;
    }

    void selectAll() {
        _selected.set();
        _mode = Mode::Persist;
        _first = 0;
    }

    void setPersisted(const std::bitset<N> &selected) {
        _selected = selected;
        _mode = selected.any() ? Mode::Persist : Mode::Immediate;
        _first = firstSetIndex();
    }

    void shiftLeft(int firstStep = 0, int lastStep = N) {
        rotateL(_selected, 1, firstStep, lastStep);
    }

    void shiftRight(int firstStep = 0, int lastStep = N) {
        rotateR(_selected, 1, firstStep, lastStep);
    }

    inline void rotateR(std::bitset<N>& b, unsigned m, int firstStep, int lastStep) {
        std::bitset<N> r;
        for (int i=firstStep; i < lastStep; ++i) {
            int index = i+m;
            if (i==lastStep-1) {    
                index = firstStep;
            }
            r[index] = b[i];
        }
        b = r;
    }

    inline void rotateL(std::bitset<N>& b, unsigned m, int firstStep, int lastStep) {
        std::bitset<N> r;
        for (int i=firstStep; i < lastStep; ++i) {
            int index = i-m;
             if (index < firstStep) {
                index = lastStep-1;
            }
            r[index] = b[i];
        }
        b = r;
    }

    void selectEqualSteps(int stepIndex) {
        _mode = Mode::Persist;
        for (int i = 0; i < int(_selected.size()); ++i) {
            if (i == stepIndex || (_stepCompare && _stepCompare(stepIndex, i))) {
                _selected.set(i);
            }
        }
    }

    void setStepCompare(std::function<bool(int, int)> stepCompare) {
        _stepCompare = stepCompare;
    }

    bool isPersisted() const {
        return _mode == Mode::Persist;
    }

    int first() const {
        return _first;
    }

    int at(int index) const {
        return _selected[index];
    }

    int firstSetIndex() const {
        for (size_t i = 0; i < _selected.size(); ++i) {
            if (_selected[i]) {
                return i;
            }
        }
        return -1;
    }

    int lastSetIndex() const {
        int last = -1;
        for (size_t i = 0; i < _selected.size(); ++i) {
            if (_selected[i]) {
                last = i;
            }
        }
        return last;
    }

    bool none() const {
        return _selected.none();
    }

    bool any() const {
        return _selected.any();
    }

    size_t size() const {
        return _selected.size();
    }

    size_t count() const {
        return _selected.count();
    }

    const std::bitset<N> &selected() const { return _selected; }
    std::bitset<N> &selected() { return _selected; }

    bool operator[](int index) const {
        return _selected[index];
    }

private:
    int otherPressedStepKey(const KeyState &keyState, int step) const {
        bool found = false;
        int other = -1;
        for (int i = 0; i < 16; ++i) {
            if (i != step && keyState[MatrixMap::fromStep(i)]) {
                if (found) {
                    return -1;
                } else {
                    other = i;
                    found = true;
                }
            }
        }
        return other;
    }

    enum class Mode : uint8_t {
        Immediate,
        Persist,
    };

    Mode _mode = Mode::Immediate;
    std::bitset<N> _selected;
    int _first = -1;
    int8_t _lastPressedIndex;
    std::function<bool(int, int)> _stepCompare;
};
