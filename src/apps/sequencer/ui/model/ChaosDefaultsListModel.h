#pragma once

#include "ListModel.h"

class ChaosDefaultsListModel : public ListModel {
public:
    enum class Mode {
        Sequence,
        Pattern,
    };

    Mode rowToMode(int row) const {
        return row == 0 ? Mode::Sequence : Mode::Pattern;
    }

    int rows() const override {
        return 2;
    }

    int columns() const override {
        return 1;
    }

    void cell(int row, int column, StringBuilder &str) const override {
        if (column != 0) {
            return;
        }

        switch (rowToMode(row)) {
        case Mode::Sequence:
            str("Seq Layers to Vandalize");
            break;
        case Mode::Pattern:
            str("Pat Layers to Wreck");
            break;
        }
    }

    void edit(int row, int column, int value, bool shift) override {
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
