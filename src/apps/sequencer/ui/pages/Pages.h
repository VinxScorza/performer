#pragma once

#include "Config.h"

#include "BusyPage.h"
#include "AcidModeSelectPage.h"
#include "ChaosScopeSelectPage.h"
#include "ChaosDefaultsPage.h"
#include "ChaosDefaultsSelectPage.h"
#include "ClockSetupPage.h"
#include "ConfirmationPage.h"
#include "ContextMenuPage.h"
#include "CurveSequenceEditPage.h"
#include "CurveSequencePage.h"
#include "FileSelectPage.h"
#include "GeneratorPage.h"
#include "GeneratorSelectPage.h"
#include "LayoutPage.h"
#include "MidiOutputPage.h"
#include "MonitorPage.h"
#include "NoteSequenceEditPage.h"
#include "NoteSequencePage.h"
#include "OverviewPage.h"
#include "PatternPage.h"
#include "PerformerPage.h"
#include "ProjectPage.h"
#include "QuickEditPage.h"
#include "RoutingPage.h"
#include "SongPage.h"
#include "StartupPage.h"
#include "StochasticSequenceEditPage.h"
#include "StochasticSequencePage.h"
#include "LogicSequenceEditPage.h"
#include "LogicSequencePage.h"
#include "ArpSequenceEditPage.h"
#include "ArpSequencePage.h"
#include "SystemPage.h"
#include "TempoPage.h"
#include "TextInputPage.h"
#include "TopPage.h"
#include "TrackPage.h"
#include "UserScalePage.h"
#include "WreckPatternWarningPage.h"

#ifdef CONFIG_ENABLE_INTRO
#include "IntroPage.h"
#endif
#ifdef CONFIG_ENABLE_ASTEROIDS
#include "AsteroidsPage.h"
#endif

struct Pages {
    TopPage top;
    // main pages
    ProjectPage project;
    LayoutPage layout;
    TrackPage track;
    NoteSequencePage noteSequence;
    CurveSequencePage curveSequence;
    StochasticSequencePage stochasticSequence;
    NoteSequenceEditPage noteSequenceEdit;
    CurveSequenceEditPage curveSequenceEdit;
    StochasticSequenceEditPage stochasticSequenceEdit;
    LogicSequencePage logicSequence;
    LogicSequenceEditPage logicSequenceEdit;
    ArpSequencePage arpSequence;
    ArpSequenceEditPage arpSequenceEdit;
    PatternPage pattern;
    PerformerPage performer;
    SongPage song;
    RoutingPage routing;
    MidiOutputPage midiOutput;
    UserScalePage userScale;
    OverviewPage overview;
    SystemPage system;
    // modal pages
    TempoPage tempo;
    ClockSetupPage clockSetup;
    MonitorPage monitor;
    FileSelectPage fileSelect;
    ContextMenuPage contextMenu;
    QuickEditPage quickEdit;
    // generator pages
    GeneratorPage generator;
    GeneratorSelectPage generatorSelect;
    AcidModeSelectPage acidModeSelect;
    ChaosScopeSelectPage chaosScopeSelect;
    ChaosDefaultsSelectPage chaosDefaultsSelect;
    ChaosDefaultsPage chaosDefaults;
    WreckPatternWarningPage wreckPatternWarning;
    // helper pages
    TextInputPage textInput;
    ConfirmationPage confirmation;
    BusyPage busy;
    // misc
    StartupPage startup;
#ifdef CONFIG_ENABLE_INTRO
    IntroPage intro;
#endif
#ifdef CONFIG_ENABLE_ASTEROIDS
    AsteroidsPage asteroids;
#endif

    Pages(PageManager &manager, PageContext &context) :
        top(manager, context),
        // main pages
        project(manager, context),
        layout(manager, context),
        track(manager, context),
        noteSequence(manager, context),
        curveSequence(manager, context),
        stochasticSequence(manager, context),
        noteSequenceEdit(manager, context),
        curveSequenceEdit(manager, context),
        stochasticSequenceEdit(manager, context),
        logicSequence(manager, context),
        logicSequenceEdit(manager, context),
        arpSequence(manager, context),
        arpSequenceEdit(manager, context),
        pattern(manager, context),
        performer(manager, context),
        song(manager, context),
        routing(manager, context),
        midiOutput(manager, context),
        userScale(manager, context),
        overview(manager, context),
        system(manager, context),
        // modal pages
        tempo(manager, context),
        clockSetup(manager, context),
        monitor(manager, context),
        fileSelect(manager, context),
        contextMenu(manager, context),
        quickEdit(manager, context),
        // generator pages
        generator(manager, context),
        generatorSelect(manager, context),
        acidModeSelect(manager, context),
        chaosScopeSelect(manager, context),
        chaosDefaultsSelect(manager, context),
        chaosDefaults(manager, context),
        wreckPatternWarning(manager, context),
        // helper pages
        textInput(manager, context),
        confirmation(manager, context),
        busy(manager, context),
        // misc
        startup(manager, context)
#ifdef CONFIG_ENABLE_INTRO
        ,intro(manager, context)
#endif
#ifdef CONFIG_ENABLE_ASTEROIDS
        ,asteroids(manager, context)
#endif
    {}
};
