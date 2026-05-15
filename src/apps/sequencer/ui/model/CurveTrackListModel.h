#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/CurveTrack.h"

class CurveTrackListModel : public RoutableListModel {
public:
    void setTrack(CurveTrack &track) {
        _track = &track;
    }

    virtual int rows() const override {
        return Last;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            formatName(Item(row), str);
        } else if (column == 1) {
            formatValue(Item(row), str);
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
        if (column == 1) {
            editValue(Item(row), value, shift);
        }
    }

    virtual Routing::Target routingTarget(int row) const override {
        switch (Item(row)) {
        case SlideTime:
            return Routing::Target::SlideTime;
        case Offset:
            return Routing::Target::Offset;
        case Rotate:
            return Routing::Target::Rotate;
        case ShapeProbabilityBias:
            return Routing::Target::ShapeProbabilityBias;
        case GateProbabilityBias:
            return Routing::Target::GateProbabilityBias;
        default:
            return Routing::Target::None;
        }
    }

private:
    enum Item {
        TrackName,
        PlayMode,
        FillMode,
        MuteMode,
        GateOutputMode,
        SlideTime,
        Offset,
        Rotate,
        ShapeProbabilityBias,
        GateProbabilityBias,
        PatternFollow,
        CurveCvInput,
        Min,
        Max,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case TrackName:             return "Name";
        case PlayMode:              return "Play Mode";
        case FillMode:              return "Fill Mode";
        case MuteMode:              return "Mute Mode";
        case GateOutputMode:        return "Gate Out Mode";
        case SlideTime:             return "Slide Time";
        case Offset:                return "Offset";
        case Rotate:                return "Rotate";
        case ShapeProbabilityBias:  return "Shape P. Bias";
        case GateProbabilityBias:   return "Gate P. Bias";
        case PatternFollow:         return "Pattern Follow";
        case CurveCvInput:          return "Curve CV Input";
        case Min:                   return "Min";
        case Max:                   return "Max";                   
        case Last:                  break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case TrackName:
            str(_track->name());
            break;
        case PlayMode:
            _track->printPlayMode(str);
            break;
        case FillMode:
            _track->printFillMode(str);
            break;
        case MuteMode:
            _track->printMuteMode(str);
            break;
        case GateOutputMode:
            _track->printGateOutputMode(str);
            break;
        case SlideTime:
            _track->printSlideTime(str);
            break;
        case Offset:
            _track->printOffset(str);
            break;
        case Rotate:
            _track->printRotate(str);
            break;
        case ShapeProbabilityBias:
            _track->printShapeProbabilityBias(str);
            break;
        case GateProbabilityBias:
            _track->printGateProbabilityBias(str);
            break;
        case PatternFollow:
            _track->printPatternFollow(str);
            break;
        case CurveCvInput:
            _track->printCurveCvInput(str);
            break;
        case Min:
            _track->printMin(str);
            break;
        case Max:
            _track->printMax(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case TrackName:
            break;
        case PlayMode:
            _track->editPlayMode(value, shift);
            break;
        case FillMode:
            _track->editFillMode(value, shift);
            break;
        case MuteMode:
            _track->editMuteMode(value, shift);
            break;
        case GateOutputMode:
            _track->editGateOutputMode(value, shift);
            break;
        case SlideTime:
            _track->editSlideTime(value, shift);
            break;
        case Offset:
            _track->editOffset(value, shift);
            break;
        case Rotate:
            _track->editRotate(value, shift);
            break;
        case ShapeProbabilityBias:
            _track->editShapeProbabilityBias(value, shift);
            break;
        case GateProbabilityBias:
            _track->editGateProbabilityBias(value, shift);
            break;
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case CurveCvInput:
            _track->editCurveCvInput(value, shift);
            break;
        case Min:
            _track->editMin(value, shift);
            break;
        case Max:
            _track->editMax(value, shift);
            break;
        case Last:
            break;
        }
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

    CurveTrack *_track;
};
