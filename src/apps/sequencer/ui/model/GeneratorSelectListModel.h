#pragma once

#include "Config.h"

#include "ListModel.h"

#include "engine/generators/Generator.h"

class GeneratorSelectListModel : public ListModel {
public:
    void setAllowAcid(bool allowAcid) {
        _allowAcid = allowAcid;
    }

    Generator::Mode rowToMode(int row) const {
        if (_allowAcid) {
            switch (row) {
            case 0:
                return Generator::Mode::Random;
            case 1:
                return Generator::Mode::Acid;
            case 2:
                return Generator::Mode::Chaos;
            case 3:
                return Generator::Mode::Euclidean;
            case 4:
                return Generator::Mode::InitSteps;
            default:
                return Generator::Mode::Random;
            }
        }

        switch (row) {
        case 0:
            return Generator::Mode::Random;
        case 1:
            return Generator::Mode::ChaosEntropy;
        case 2:
            return Generator::Mode::Euclidean;
        case 3:
            return Generator::Mode::InitSteps;
        default:
            return Generator::Mode::Random;
        }
    }

    virtual int rows() const override {
        return _allowAcid ? 5 : 4;
    }

    virtual int columns() const override {
        return 1;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            if (_allowAcid && row == 1) {
                str("Acid (Layer/Phrase)");
                return;
            }
            if (_allowAcid && row == 2) {
                str("Chaos (Vandalize/Wreck)");
                return;
            }
            if (!_allowAcid && row == 1) {
                str("Chaos (Entropy)");
                return;
            }
            str(Generator::modeName(rowToMode(row)));
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

private:
    bool _allowAcid = false;
};
