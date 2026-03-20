#pragma once

#include "Config.h"

#include "ListModel.h"

#include "engine/generators/Generator.h"

class GeneratorSelectListModel : public ListModel {
public:
    static Generator::Mode rowToMode(int row) {
        switch (row) {
        case 0: return Generator::Mode::Random;
        case 1: return Generator::Mode::Euclidean;
        case 2: return Generator::Mode::InitLayer;
        default: return Generator::Mode::Random;
        }
    }

    virtual int rows() const override {
        return int(Generator::Mode::Last);
    }

    virtual int columns() const override {
        return 1;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            str(Generator::modeName(rowToMode(row)));
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

};
