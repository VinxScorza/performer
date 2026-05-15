#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/NoteTrack.h"
#include <vector>

class NoteTrackListModel : public RoutableListModel {
public:

    NoteTrackListModel() {
        for (int i = 0; i< 8; ++i) {
            _selectedTrack[i] = -1;
        }
    }

    void setTrack(NoteTrack &track) {
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
        case Rotate:
            return Routing::Target::Rotate;
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
        Rotate,
        GateProbabilityBias,
        RetriggerProbabilityBias,
        LengthBias,
        NoteProbabilityBias,
        PatternFollow,
        LogicTrack,
        LogicTrackInput,
        Last
    };

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
        case Rotate:    return "Rotate";
        case GateProbabilityBias: return "Gate P. Bias";
        case RetriggerProbabilityBias: return "Retrig P. Bias";
        case LengthBias: return "Length Bias";
        case NoteProbabilityBias: return "Note P. Bias";
        case PatternFollow: return "Pattern Follow";
        case LogicTrack: return "Logic Track";
        case LogicTrackInput: return "Logic Track In";
        case Last:      break;
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
        case Rotate:
            _track->printRotate(str);
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
        case LogicTrack:
            _track->printLogicTrack(str);
            break;
        case LogicTrackInput:
            _track->printLogicTrackInput(str);
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
        case Rotate:
            _track->editRotate(value, shift);
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
        case LogicTrack: {

                if (_availableLogicTracks.size() == 0) {
                    break;
                }
                
                if (_track->logicTrackInput() != -1) {
                    break;
                }
                if (value == -1 && _selectedTrack[_track->trackIndex()] == -1) {
                    break;
                }

                if (value == -1 && _selectedTrack[_track->trackIndex()] == _availableLogicTracks.front()) {
                    _track->setLogicTrack(-1);
                    _selectedTrack[_track->trackIndex()] = -1;
                    break;
                }

                if (value == 1 && _selectedTrack[_track->trackIndex()] == _availableLogicTracks.back()) {
                    break;
                }

                if (value == 1) {
                    for (int i = 0; i < 8; ++i ) {
                        if (std::find(_availableLogicTracks.begin(), _availableLogicTracks.end(), i) != _availableLogicTracks.end() && i != _selectedTrack[_track->trackIndex()]) {
                            _track->setLogicTrack(i);
                            _selectedTrack[_track->trackIndex()] = i;
                            break;
                        }
                    }
                } else {
                    for (int i = 7; i > 0; --i ) {
                        if (std::find(_availableLogicTracks.begin(), _availableLogicTracks.end(), i) != _availableLogicTracks.end() && i != _selectedTrack[_track->trackIndex()]) {
                            _track->setLogicTrack(i);
                            _selectedTrack[_track->trackIndex()] = i;
                            break;
                        }
                    }
                }

                
            }
            break;
            case LogicTrackInput:
                if (_track->logicTrack()==-1) {
                    return;
                }
                _track->editLogicTrackInput(value, shift);
                break;
        case Last:
            break;
        }
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

    NoteTrack *_track;

    std::vector<int> _availableLogicTracks;
    int _selectedTrack[8];
};
