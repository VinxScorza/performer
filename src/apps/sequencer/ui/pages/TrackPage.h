#pragma once

#include "ListPage.h"

#include "ui/model/NoteTrackListModel.h"
#include "ui/model/CurveTrackListModel.h"
#include "ui/model/MidiCvTrackListModel.h"
#include "ui/model/StochasticTrackListModel.h"
#include "ui/model/LogicTrackListModel.h"
#include "ui/model/ArpTrackListModel.h"


class TrackPage : public ListPage {
public:
    TrackPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyPress(KeyPressEvent &event) override;

private:
    void setTrack(Track &track);

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initTrackSetup();
    void copyTrackSetup();
    void pasteTrackSetup();
    void initRoute();

    RoutableListModel *_listModel;
    NoteTrackListModel _noteTrackListModel;
    CurveTrackListModel _curveTrackListModel;
    MidiCvTrackListModel _midiCvTrackListModel;
    StochasticTrackListModel _stochasticTrackListModel;
    LogicTrackListModel _logicTrackListModel;
    ArpTrackListModel _arpTrackListModel;

    NoteTrack *_noteTrack;
    CurveTrack *_curveTrack;
    MidiCvTrack *_midiCvTrack;
    StochasticTrack *_stochasticTrack;
    LogicTrack *_logicTrack;
    ArpTrack *_arpTrack;
};
