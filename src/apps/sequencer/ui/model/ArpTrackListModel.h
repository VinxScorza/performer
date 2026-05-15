#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/ArpTrack.h"
#include <vector>

class ArpTrackListModel : public RoutableListModel {
public:
    enum Item {
        TrackName,
        PlayMode,
        FillMode,
        FillMuted,
        CvUpdateMode,
        GateOutputMode,
        SlideTime,
        Octave,
        Transpose,
        GateProbabilityBias,
        RetriggerProbabilityBias,
        LengthBias,
        NoteProbabilityBias,
        PatternFollow,
        ArpeggiatorMode,
        ArpeggiatorHold,
        ArpeggiatorOctaves,
        ArpeggiatorGateLength,
        ArpeggiatorDivisor,
        Last
    };

    ArpTrackListModel() {
        for (int i = 0; i< 8; ++i) {
            _selectedTrack[i] = -1;
        }
    }

    void setTrack(ArpTrack &track) {
        _track = &track;
    }

    void setAvailableLogicTracks(std::vector<int> availableLogicTracks) {
        _availableLogicTracks = availableLogicTracks;
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
        case Octave:
            return Routing::Target::Octave;
        case Transpose:
            return Routing::Target::Transpose;
        case GateProbabilityBias:
            return Routing::Target::GateProbabilityBias;
        case RetriggerProbabilityBias:
            return Routing::Target::RetriggerProbabilityBias;
        case LengthBias:
            return Routing::Target::LengthBias;
        case NoteProbabilityBias:
            return Routing::Target::NoteProbabilityBias;
        default:
            return Routing::Target::None;
        }
    }

private:
    static const char *itemName(Item item) {
        switch (item) {
        case TrackName: return "Name";
        case PlayMode:  return "Play Mode";
        case FillMode:  return "Fill Mode";
        case FillMuted: return "Fill Muted";
        case CvUpdateMode:  return "CV Update Mode";
        case GateOutputMode: return "Gate Out Mode";
        case SlideTime: return "Slide Time";
        case Octave:    return "Octave";
        case Transpose: return "Transpose";
        case GateProbabilityBias: return "Gate P. Bias";
        case RetriggerProbabilityBias: return "Retrig P. Bias";
        case LengthBias: return "Length Bias";
        case NoteProbabilityBias: return "Note P. Bias";
        case PatternFollow: return "Pattern Follow";
        case ArpeggiatorMode:       return "Mode";
        case ArpeggiatorHold:       return "Hold";
        case ArpeggiatorOctaves:    return "Octaves";
        case ArpeggiatorDivisor:    return "Rate";
        case ArpeggiatorGateLength: return "Gate Length";
        case Last:      break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        const auto &arpeggiator = _track->arpeggiator();
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
        case FillMuted:
            _track->printFillMuted(str);
            break;
        case CvUpdateMode:
            _track->printCvUpdateMode(str);
            break;
        case GateOutputMode:
            _track->printGateOutputMode(str);
            break;
        case SlideTime:
            _track->printSlideTime(str);
            break;
        case Octave:
            _track->printOctave(str);
            break;
        case Transpose:
            _track->printTranspose(str);
            break;
        case GateProbabilityBias:
            _track->printGateProbabilityBias(str);
            break;
        case RetriggerProbabilityBias:
            _track->printRetriggerProbabilityBias(str);
            break;
        case LengthBias:
            _track->printLengthBias(str);
            break;
        case NoteProbabilityBias:
            _track->printNoteProbabilityBias(str);
            break;
        case PatternFollow:
            _track->printPatternFollow(str);
            break;
        case ArpeggiatorMode:
            arpeggiator.printMode(str);
            break;
         case ArpeggiatorHold:
            arpeggiator.printHold(str);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.printOctaves(str);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.printDivisor(str);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.printGateLength(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        auto &arpeggiator = _track->arpeggiator();
        switch (item) {

        case TrackName:
            break;
        case PlayMode:
            break;
        case FillMode:
            _track->editFillMode(value, shift);
            break;
        case FillMuted:
            _track->editFillMuted(value, shift);
            break;
        case CvUpdateMode:
            _track->editCvUpdateMode(value, shift);
            break;
        case GateOutputMode:
            _track->editGateOutputMode(value, shift);
            break;
        case SlideTime:
            _track->editSlideTime(value, shift);
            break;
        case Octave:
            _track->editOctave(value, shift);
            break;
        case Transpose:
            _track->editTranspose(value, shift);
            break;
        case GateProbabilityBias:
            _track->editGateProbabilityBias(value, shift);
            break;
        case RetriggerProbabilityBias:
            _track->editRetriggerProbabilityBias(value, shift);
            break;
        case LengthBias:
            _track->editLengthBias(value, shift);
            break;
        case NoteProbabilityBias:
            _track->editNoteProbabilityBias(value, shift);
            break;
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case ArpeggiatorMode:
            arpeggiator.editMode(value, shift);
            break;
         case ArpeggiatorHold:
            arpeggiator.editHold(value, shift);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.editOctaves(value, shift);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.editDivisor(value, shift);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.editGateLength(value, shift);
            break;
        case Last:
            break;
        }
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

    ArpTrack *_track;

    std::vector<int> _availableLogicTracks;
    int _selectedTrack[8];
};
