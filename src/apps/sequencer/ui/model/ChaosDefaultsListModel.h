#pragma once

#include "ListModel.h"

class ChaosDefaultsListModel : public ListModel {
public:
    enum class Mode {
        Sequence,
        Pattern,
        Entropy,
    };

    Mode rowToMode(int row) const {
        switch (row) {
        case 0:
            return Mode::Sequence;
        case 1:
            return Mode::Pattern;
        default:
            return Mode::Entropy;
        }
    }

    int rows() const override {
        return 3;
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
        case Mode::Entropy:
            str("EntropyLayerToUnleash");
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
