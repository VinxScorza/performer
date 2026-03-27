#pragma once

#include "ListModel.h"

#include "engine/generators/ChaosGenerator.h"

class ChaosScopeSelectListModel : public ListModel {
public:
    ChaosGenerator::Scope rowToScope(int row) const {
        return row == 0 ? ChaosGenerator::Scope::Sequence : ChaosGenerator::Scope::Pattern;
    }

    virtual int rows() const override {
        return 2;
    }

    virtual int columns() const override {
        return 1;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column != 0) {
            return;
        }

        switch (rowToScope(row)) {
        case ChaosGenerator::Scope::Sequence:
            str("Vandalize Sequence");
            break;
        case ChaosGenerator::Scope::Pattern:
            str("Wreck Pattern");
            break;
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
        (void)row;
        (void)column;
        (void)value;
        (void)shift;
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {
        (void)defaultScale;
        (void)force;
    }
};
