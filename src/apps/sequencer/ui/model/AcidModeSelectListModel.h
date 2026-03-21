#pragma once

#include "ListModel.h"

#include "engine/generators/SequenceBuilder.h"

class AcidModeSelectListModel : public ListModel {
public:
    void setAllowLayer(bool allowLayer) {
        _allowLayer = allowLayer;
    }

    AcidSequenceBuilder::ApplyMode rowToMode(int row) const {
        if (_allowLayer) {
            return row == 0 ? AcidSequenceBuilder::ApplyMode::Phrase : AcidSequenceBuilder::ApplyMode::Layer;
        }
        return AcidSequenceBuilder::ApplyMode::Phrase;
    }

    virtual int rows() const override {
        return _allowLayer ? 2 : 1;
    }

    virtual int columns() const override {
        return 1;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column != 0) {
            return;
        }

        switch (rowToMode(row)) {
        case AcidSequenceBuilder::ApplyMode::Layer:
            str("Layer");
            break;
        case AcidSequenceBuilder::ApplyMode::Phrase:
            str("Phrase");
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

private:
    bool _allowLayer = true;
};
