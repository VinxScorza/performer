#pragma once

#include "ArpTrack.h"
#include "Config.h"

#include "CurveTrack.h"
#include "LogicSequence.h"
#include "LogicTrack.h"
#include "MidiCvTrack.h"
#include "StochasticSequence.h"
#include "StochasticTrack.h"
#include "Track.h"
#include "NoteSequence.h"
#include "CurveSequence.h"
#include "ArpSequence.h"
#include "Project.h"
#include "UserScale.h"

#include "core/utils/Container.h"

#include <bitset>

class ClipBoard {
public:
    typedef std::bitset<CONFIG_STEP_COUNT> SelectedSteps;

    ClipBoard(Project &project);

    void clear();

    void copyTrack(const Track &track);
    void copyNoteSequence(const NoteSequence &noteSequence);
    void copyNoteSequenceSteps(const NoteSequence &noteSequence, const SelectedSteps &selectedSteps);
    void copyNoteSequenceSteps(NoteSequence &noteSequence, const SelectedSteps &selectedSteps);
    void copyCurveSequence(const CurveSequence &curveSequence);
    void copyCurveSequenceSteps(const CurveSequence &curveSequence, const SelectedSteps &selectedSteps);
    void copyStochasticSequence(const StochasticSequence &noteSequence);
    void copyStochasticSequenceSteps(const StochasticSequence &noteSequence, const SelectedSteps &selectedSteps);
    void copyLogicSequence(const LogicSequence &noteSequence);
    void copyLogicSequenceSteps(const LogicSequence &noteSequence, const SelectedSteps &selectedSteps);
    void copyArpSequence(const ArpSequence &noteSequence);
    void copyArpSequenceSteps(const ArpSequence &noteSequence, const SelectedSteps &selectedSteps);
    void copyPattern(int patternIndex);
    void copyUserScale(const UserScale &userScale);

    void pasteTrack(Track &track) const;
    void pasteNoteSequence(NoteSequence &noteSequence) const;
    void pasteNoteSequenceSteps(NoteSequence &noteSequence, const SelectedSteps &selectedSteps) const;
    void pasteCurveSequence(CurveSequence &curveSequence) const;
    void pasteCurveSequenceSteps(CurveSequence &curveSequence, const SelectedSteps &selectedSteps) const;
    void pasteStochasticSequence(StochasticSequence &noteSequence) const;
    void pasteStochasticSequenceSteps(StochasticSequence &noteSequence, const SelectedSteps &selectedSteps) const;
    void pasteLogicSequence(LogicSequence &noteSequence) const;
    void pasteLogicSequenceSteps(LogicSequence &noteSequence, const SelectedSteps &selectedSteps) const;
    void pasteArpSequence(ArpSequence &noteSequence) const;
    void pasteArpSequenceSteps(ArpSequence &noteSequence, const SelectedSteps &selectedSteps) const;    
    void pastePattern(int patternIndex) const;
    void pasteUserScale(UserScale &userScale) const;

    bool canPasteTrack() const;
    bool canPasteNoteSequence() const;
    bool canPasteNoteSequenceSteps() const;
    bool pasteNoteSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const;
    bool canPasteCurveSequence() const;
    bool canPasteCurveSequenceSteps() const;
    bool pasteCurveSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const;
    bool canPasteStochasticSequence() const;
    bool canPasteStochasticSequenceSteps() const;
    bool pasteStochasticSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const;
    bool canPasteLogicSequence() const;
    bool canPasteLogicSequenceSteps() const;
    bool pasteLogicSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const;
    bool canPasteArpSequence() const;
    bool canPasteArpSequenceSteps() const;    
    bool pasteArpSequenceStepsRequiresDestination(const SelectedSteps &selectedSteps) const;
    bool canPastePattern() const;
    bool canPasteUserScale() const;

private:
    enum class Type : uint8_t {
        None,
        Track,
        NoteSequence,
        NoteSequenceSteps,
        CurveSequence,
        CurveSequenceSteps,
        StochasticSequence,
        StochasticSequenceSteps,
        LogicSequence,
        LogicSequenceSteps,
        ArpSequence,
        ArpSequenceSteps,        
        Pattern,
        UserScale,
    };

    struct NoteSequenceSteps {
        NoteSequence sequence;
        SelectedSteps selected;
    };

    struct CurveSequenceSteps {
        CurveSequence sequence;
        SelectedSteps selected;
    };

    struct StochasticSequenceSteps {
        StochasticSequence sequence;
        SelectedSteps selected;
    };

    struct LogicSequenceSteps {
        LogicSequence sequence;
        SelectedSteps selected;
    };

    struct ArpSequenceSteps {
        ArpSequence sequence;
        SelectedSteps selected;
    };

    struct Pattern {
        struct {
            Track::TrackMode trackMode;
            union {
                NoteSequence note;
                CurveSequence curve;
                StochasticSequence stochastic;
                LogicSequence logic;
                ArpSequence arp;
            } data;
        } sequences[CONFIG_TRACK_COUNT];
    };

    Project &_project;
    Type _type = Type::None;
    Container<Track, NoteTrack, CurveTrack, MidiCvTrack, StochasticTrack, LogicTrack, ArpTrack, NoteSequence, NoteSequenceSteps, CurveSequence, CurveSequenceSteps, StochasticSequence, StochasticSequenceSteps, LogicSequence, LogicSequenceSteps, ArpSequence, ArpSequenceSteps, Pattern, UserScale> _container;
};
