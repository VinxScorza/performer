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
        switch (row) {
        case 0:
            return Generator::Mode::Random;
        case 1:
            return _allowAcid ? Generator::Mode::Acid : Generator::Mode::Euclidean;
        case 2:
            return _allowAcid ? Generator::Mode::Chaos : Generator::Mode::InitLayer;
        case 3:
            return _allowAcid ? Generator::Mode::Euclidean : Generator::Mode::Random;
        case 4:
            return Generator::Mode::InitLayer;
        default:
            return Generator::Mode::Random;
        }
    }

    virtual int rows() const override {
        return _allowAcid ? 5 : 3;
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

private:
    bool _allowAcid = false;
};
