#include "LaunchpadController.h"

#include "LaunchpadDevice.h"
#include "ui/ControllerManager.h"
#include "ui/pages/NoteSequenceEditPage.h"
#include "ui/pages/GeneratorPage.h"
#include "ui/pages/Pages.h"
#include "core/Debug.h"
#include "os/os.h"
#include <algorithm>
#include <functional>
#include <map>
#include <ratio>
#include <set>

#define CALL_MODE_FUNCTION(_mode_, _function_, ...)                         \
    switch (_mode_) {                                                       \
    case Mode::Sequence:    sequence##_function_(__VA_ARGS__);      break;  \
    case Mode::Pattern:     pattern##_function_(__VA_ARGS__);       break;  \
    case Mode::Performer:   performer##_function_(__VA_ARGS__);     break;  \
    }

#define BUTTON(_name_, _row_, _col_)        \
    struct _name_ {                         \
        static constexpr int row = _row_;   \
        static constexpr int col = _col_;   \
    };

// Global buttons
BUTTON(Navigate, LaunchpadDevice::FunctionRow, 0)
BUTTON(Left, LaunchpadDevice::FunctionRow, 1)
BUTTON(Right, LaunchpadDevice::FunctionRow, 2)
BUTTON(Up, LaunchpadDevice::FunctionRow, 3)
BUTTON(Down, LaunchpadDevice::FunctionRow, 4)
BUTTON(Play, LaunchpadDevice::FunctionRow, 6)
BUTTON(Shift, LaunchpadDevice::FunctionRow, 7)

// Sequence page buttons
BUTTON(Layer, LaunchpadDevice::FunctionRow, 1)
BUTTON(FirstStep, LaunchpadDevice::FunctionRow, 2)
BUTTON(LastStep, LaunchpadDevice::FunctionRow, 3)
BUTTON(RunMode, LaunchpadDevice::FunctionRow, 4)
BUTTON(FollowMode, LaunchpadDevice::FunctionRow, 5);

// Pattern page buttons
BUTTON(Latch, LaunchpadDevice::FunctionRow, 1)
BUTTON(Sync, LaunchpadDevice::FunctionRow, 2)

// Sequence and pattern page buttons
BUTTON(Fill, LaunchpadDevice::FunctionRow, 6)

struct LayerMapItem {
    uint8_t row;
    uint8_t col;
};

enum class PerforModeLayers {
        SequenceLength,
        Overview,
    };

static const LayerMapItem noteSequenceLayerMap[] = {
    [int(NoteSequence::Layer::Gate)]                        =  { 0, 0 },
    [int(NoteSequence::Layer::GateProbability)]             =  { 1, 0 },
    [int(NoteSequence::Layer::GateOffset)]                  =  { 2, 0 },
    [int(NoteSequence::Layer::Retrigger)]                   =  { 0, 1 },
    [int(NoteSequence::Layer::RetriggerProbability)]        =  { 1, 1 },
    [int(NoteSequence::Layer::StageRepeats)]                =  { 2, 1 },
    [int(NoteSequence::Layer::StageRepeatsMode)]            =  { 3, 1 },
    [int(NoteSequence::Layer::Length)]                      =  { 0, 2 },
    [int(NoteSequence::Layer::LengthVariationRange)]        =  { 1, 2 },
    [int(NoteSequence::Layer::LengthVariationProbability)]  =  { 2, 2 },
    [int(NoteSequence::Layer::Note)]                        =  { 0, 3 },
    [int(NoteSequence::Layer::NoteVariationRange)]          =  { 1, 3 },
    [int(NoteSequence::Layer::NoteVariationProbability)]    =  { 2, 3 },
    [int(NoteSequence::Layer::Slide)]                       =  { 3, 3 },
    [int(NoteSequence::Layer::BypassScale)]                 =  { 4, 3 },
    [int(NoteSequence::Layer::Condition)]                   =  { 0, 4 },


};

static constexpr int noteSequenceLayerMapSize = sizeof(noteSequenceLayerMap) / sizeof(noteSequenceLayerMap[0]);

static const LayerMapItem logicSequenceLayerMap[] = {
    [int(LogicSequence::Layer::Gate)]                        =  { 0, 0 },
    [int(LogicSequence::Layer::GateLogic)]                   =  { 1, 0 },
    [int(LogicSequence::Layer::GateProbability)]             =  { 2, 0 },
    [int(LogicSequence::Layer::GateOffset)]                  =  { 3, 0 },
    [int(LogicSequence::Layer::Retrigger)]                   =  { 0, 1 },
    [int(LogicSequence::Layer::RetriggerProbability)]        =  { 1, 1 },
    [int(LogicSequence::Layer::StageRepeats)]                =  { 2, 1 },
    [int(LogicSequence::Layer::StageRepeatsMode)]            =  { 3, 1 },
    [int(LogicSequence::Layer::Length)]                      =  { 0, 2 },
    [int(LogicSequence::Layer::LengthVariationRange)]        =  { 1, 2 },
    [int(LogicSequence::Layer::LengthVariationProbability)]  =  { 2, 2 },
    [int(LogicSequence::Layer::NoteLogic)]                   =  { 0, 3 },
    [int(LogicSequence::Layer::NoteVariationRange)]          =  { 1, 3 },
    [int(LogicSequence::Layer::NoteVariationProbability)]    =  { 2, 3 },
    [int(LogicSequence::Layer::Slide)]                       =  { 3, 3 },
    [int(LogicSequence::Layer::Condition)]                   =  { 0, 4 },


};

static constexpr int logicSequenceLayerMapSize = sizeof(logicSequenceLayerMap) / sizeof(logicSequenceLayerMap[0]);

static const LayerMapItem curveSequenceLayerMap[] = {
    [int(CurveSequence::Layer::Shape)]                      =  { 0, 0 },
    [int(CurveSequence::Layer::ShapeVariation)]             =  { 1, 0 },
    [int(CurveSequence::Layer::ShapeVariationProbability)]  =  { 2, 0 },
    [int(CurveSequence::Layer::Min)]                        =  { 0, 1 },
    [int(CurveSequence::Layer::Max)]                        =  { 0, 2 },
    [int(CurveSequence::Layer::Gate)]                       =  { 0, 3 },
    [int(CurveSequence::Layer::GateProbability)]            =  { 2, 3 },
    [int(CurveSequence::Layer::GateOffset)]                 =  { 1, 3 },
    [int(CurveSequence::Layer::GateLength)]                 =  { 3, 3 },
};

static const LayerMapItem performLayerMap[] = {
    [int(PerforModeLayers::SequenceLength)]                 =  { 0, 0 },
    [int(PerforModeLayers::Overview)]                       =  { 0, 1 },
};

static constexpr int performLayerMapSize = sizeof(performLayerMap) / sizeof(performLayerMap[0]);

static constexpr int curveSequenceLayerMapSize = sizeof(curveSequenceLayerMap) / sizeof(curveSequenceLayerMap[0]);


static const LayerMapItem stochasticSequenceLayerMap[] = {
    [int(StochasticSequence::Layer::Gate)]                        =  { 0, 0 },
    [int(StochasticSequence::Layer::GateProbability)]             =  { 1, 0 },
    [int(StochasticSequence::Layer::GateOffset)]                  =  { 2, 0 },
    [int(StochasticSequence::Layer::Retrigger)]                   =  { 0, 1 },
    [int(StochasticSequence::Layer::RetriggerProbability)]        =  { 1, 1 },
    [int(StochasticSequence::Layer::StageRepeats)]                =  { 2, 1 },
    [int(StochasticSequence::Layer::StageRepeatsMode)]            =  { 3, 1 },
    [int(StochasticSequence::Layer::Length)]                      =  { 0, 2 },
    [int(StochasticSequence::Layer::LengthVariationRange)]        =  { 1, 2 },
    [int(StochasticSequence::Layer::LengthVariationProbability)]  =  { 2, 2 },
    [int(StochasticSequence::Layer::NoteVariationProbability)]    =  { 0, 3 },
    [int(StochasticSequence::Layer::NoteOctave)]                  =  { 1, 3 },
    [int(StochasticSequence::Layer::NoteOctaveProbability)]       =  { 2, 3 },    
    [int(StochasticSequence::Layer::Slide)]                       =  { 3, 3 },
    [int(StochasticSequence::Layer::Condition)]                   =  { 0, 4 },
};


static constexpr int stochasticSequenceLayerMapSize = sizeof(stochasticSequenceLayerMap) / sizeof(stochasticSequenceLayerMap[0]);

static const LayerMapItem arpSequenceLayerMap[] = {
    [int(ArpSequence::Layer::Gate)]                        =  { 0, 0 },
    [int(ArpSequence::Layer::GateProbability)]             =  { 1, 0 },
    [int(ArpSequence::Layer::GateOffset)]                  =  { 2, 0 },
    [int(ArpSequence::Layer::Retrigger)]                   =  { 0, 1 },
    [int(ArpSequence::Layer::RetriggerProbability)]        =  { 1, 1 },
    [int(ArpSequence::Layer::Length)]                      =  { 0, 2 },
    [int(ArpSequence::Layer::LengthVariationRange)]        =  { 1, 2 },
    [int(ArpSequence::Layer::LengthVariationProbability)]  =  { 2, 2 },
    [int(ArpSequence::Layer::Note)]                        =  { 0, 3 },
    [int(ArpSequence::Layer::NoteVariationRange)]          =  { 1, 3 },
    [int(ArpSequence::Layer::NoteVariationProbability)]    =  { 2, 3 },
    [int(ArpSequence::Layer::NoteOctave)]                  =  { 3, 3 },
    [int(ArpSequence::Layer::NoteOctaveProbability)]       =  { 4, 3 },    
    [int(ArpSequence::Layer::Slide)]                       =  { 5, 3 },
    [int(ArpSequence::Layer::Condition)]                   =  { 0, 4 },
};


static constexpr int arpSequenceLayerMapSize = sizeof(arpSequenceLayerMap) / sizeof(arpSequenceLayerMap[0]);

struct RangeMap {
    int16_t min[2];
    int16_t max[2];
    int map(int value) const {
        return min[1] + ((value - min[0]) * (max[1] - min[1]) + (max[0] - min[0]) / 2) / (max[0] - min[0]);
    }
    int unmap(int value) const {
        return min[0] + ((value - min[1]) * (max[0] - min[0]) + (max[1] - min[1]) / 2) / (max[1] - min[1]);
    }
};

static const RangeMap curveMinMaxRangeMap = { { 0, 0 }, { 255, 7 } };

static const RangeMap *curveSequenceLayerRangeMap[] = {
    [int(CurveSequence::Layer::Shape)]                      = nullptr,
    [int(CurveSequence::Layer::ShapeVariation)]             = nullptr,
    [int(CurveSequence::Layer::ShapeVariationProbability)]  = nullptr,
    [int(CurveSequence::Layer::Min)]                        = &curveMinMaxRangeMap,
    [int(CurveSequence::Layer::Max)]                        = &curveMinMaxRangeMap,
    [int(CurveSequence::Layer::Gate)]                       = nullptr,
    [int(CurveSequence::Layer::GateProbability)]            = nullptr,
    [int(CurveSequence::Layer::GateOffset)]                 = nullptr,
    [int(CurveSequence::Layer::GateLength)]                 = nullptr,
};

UserSettings _userSettings;
int _style = 0;
int _patternChangeDefault = 0;
int _noteStyle = 0;

static const std::map<int8_t, int> nativationRowMap = {{'\x03', 0}, {'\x02', 1}, {'\x01', 2}, {'\x00', 3}, {'\xff', 4}, {'\xfe', 5}, {'\xfd', 6}, {'\xfe', 7}};
static const int noteGridValues[] = { 0,1,1,0,1,1,1,0, 1, 1, 1, 1, 1, 1, 1};
static const std::map<int, int> semitones = {{1, 1}, {2, 3}, {4, 6}, {5,8}, {6, 10 }};
static const std::map<int, int> tones = {{0,0}, {1,2}, {2, 4}, {3, 5}, {4, 7}, {5, 9}, {6, 11}};
static const std::map<int, int> octaveMap = { {0, -4}, {1, -3}, {2, -2}, {3, -1}, {4, 0}, {5, 1}, {6, 2}, {7, 3}};

int selectedNote = -1;
int fullSelectedNote = 0;
bool fullNoteSelected = false;
int selectedOctave = 0;

LaunchpadController::LaunchpadController(ControllerManager &manager, Model &model, Engine &engine, const ControllerInfo &info) :
    Controller(manager, model, engine),
    _project(model.project())
{
    if (info.productId == 0x0069) {
        // Launchpad Mini Mk2
        _device = _deviceContainer.create<LaunchpadMk2Device>();
    } else if (info.productId == 0x0051) {
        // Launchpad Pro
        _device = _deviceContainer.create<LaunchpadProDevice>();
    } else if (info.productId == 0x0113 || info.productId == 0x0103) {
        // Launchpad Mini Mk3 | Launchpad X
        _device = _deviceContainer.create<LaunchpadMk3Device>();
    } else if (info.productId == 0x0123) {
        // Launchpad Mk3 Pro
        _device = _deviceContainer.create<LaunchpadProMk3Device>();
    } else {
        _device = _deviceContainer.create<LaunchpadDevice>();
    }

    _device->setSendMidiHandler([this] (uint8_t cable, const MidiMessage &message) {
        return sendMidi(cable, message);
    });

    _device->setButtonHandler([this] (int row, int col, bool state) {
        // DBG("button %d/%d - %d", row, col, state);
        if (state) {
            buttonDown(row, col);
        } else {
            buttonUp(row, col);
        }
    });

    _device->initialize();

    setMode(Mode::Sequence);

    _userSettings = model.settings().userSettings();
}

LaunchpadController::~LaunchpadController() {
    _deviceContainer.destroy(_device);
}

void LaunchpadController::uiPageChanged() {
    if (_mode == Mode::Sequence && _generatorMode && !generatorModeSupported()) {
        cancelGeneratorMode();
    }
}

void LaunchpadController::update() {
     _style = _userSettings.get<LaunchpadStyleSetting>(SettingLaunchpadStyle)->getValue();
     _patternChangeDefault = _userSettings.get<PatternChange>(SettingPatternChange)->getValue();

     _noteStyle = _userSettings.get<LaunchpadNoteStyle>(SettingLaunchpadNoteStyle)->getValue();

    _device->clearLeds();

    CALL_MODE_FUNCTION(_mode, Draw)

    globalDraw();

    _device->syncLeds();
}

void LaunchpadController::recvMidi(uint8_t cable, const MidiMessage &message) {
    _device->recvMidi(cable, message);
}

void LaunchpadController::setMode(Mode mode) {
    CALL_MODE_FUNCTION(_mode, Exit)
    _mode = mode;
    CALL_MODE_FUNCTION(_mode, Enter)
}

//----------------------------------------
// Global handlers
//----------------------------------------

void LaunchpadController::globalDraw() {
    if (_generatorMode && generatorModeSupported()) {
        setFunctionLed(3, colorYellow());
    }

    if (buttonState<Shift>()) {
        for (int col = 0; col < 8; ++col) {
            bool selected = col == int(_mode) || col == 7;
            setFunctionLed(col, selected ? colorYellow() : colorOff());
        }
        if (_generatorMode && generatorModeSupported()) {
            setFunctionLed(3, colorYellow());
        }
        mirrorButton<Play>(_style);
    }
}

bool LaunchpadController::globalButton(const Button &button, ButtonAction action) {
    if (_mode == Mode::Sequence && _generatorMode && generatorModeSupported()) {
        if (button.is<Shift>()) {
            if (action == ButtonAction::Down) {
                _generatorApplyArmed = generatorModePreviewPage();
                _generatorApplyCanceled = false;
                return true;
            }
            if (action == ButtonAction::Up) {
                if (_generatorApplyArmed && !_generatorApplyCanceled && generatorModePreviewPage()) {
                    if (auto *pages = _manager.pages()) {
                        pages->generator.commit();
                        setGeneratorMode(false);
                    }
                }
                _generatorApplyArmed = false;
                _generatorApplyCanceled = false;
                return true;
            }
        }

        if (buttonState<Shift>() && button.isFunction() && button.function() == 3 && action == ButtonAction::Down) {
            _generatorApplyCanceled = true;
            setGeneratorMode(false);
            return true;
        }
    }

    if (action == ButtonAction::Down) {
        if (buttonState<Shift>() && button.isFunction()) {
            switch (button.function()) {
            case 0:
                if (_mode == Mode::Sequence && _generatorMode) {
                    cancelGeneratorMode();
                }
                setMode(Mode::Sequence);
                break;
            case 1:
                if (_mode == Mode::Sequence && _generatorMode) {
                    cancelGeneratorMode();
                }
                setMode(Mode::Pattern);
                break;
            case 2:
                if (_mode == Mode::Sequence && _generatorMode) {
                    cancelGeneratorMode();
                }
                setMode(Mode::Performer);
                break;
            case 3:
                if (_mode == Mode::Sequence && _project.selectedTrack().trackMode() == Track::TrackMode::Note) {
                    if (_generatorMode && generatorModeSupported()) {
                        setGeneratorMode(false);
                        return true;
                    }

                    if (!generatorModeSupported()) {
                        if (auto *pages = _manager.pages()) {
                            pages->top.setMode(TopPage::Mode::SequenceEdit);
                        }
                    }

                    setGeneratorMode(true);
                    return true;
                }
                break;
            case 6:
                _engine.togglePlay();
                break;
            }
            return true;
        }
        if (buttonState<Play>() && button.isGrid()) {

            if (_performSelectedLayer == 1) {
                    const auto &scale = Scale::get(0);
                    auto track = _project.track(button.row);
                    
                    int stepIndex = (_performNavigation.navigation.col*8)+button.col;
                    switch (track.trackMode()) {
                        case Track::TrackMode::Note: {
                                auto &sequence = track.noteTrack().sequence(_project.selectedPatternIndex());
                                if (sequence.step(stepIndex).note()==(scale.notesPerOctave()*5)) {
                                    sequence.step(stepIndex).setNote(0);
                                } else {
                                    sequence.step(stepIndex).setNote(scale.notesPerOctave()*5);
                                }
                            }
                            break;
                        default:
                            break;
                    }

                    return false;
                }


            if (_project.selectedTrack().trackMode() == Track::TrackMode::Note) {
                const auto &scale = Scale::get(0);

                auto &sequence = _project.selectedNoteSequence();
                switch (_project.selectedNoteSequenceLayer()) {
                    case NoteSequence::Layer::Gate:
                        if (sequence.step(button.gridIndex()).note()==(scale.notesPerOctave()*5)) {
                            sequence.step(button.gridIndex()).setNote(0);
                        } else {
                            sequence.step(button.gridIndex()).setNote(scale.notesPerOctave()*5);
                        }
                        break; 
                    case NoteSequence::Layer::Note: {
                            if (_noteStyle == 1) {
                                if (button.gridIndex()>15) {
                                    return true;
                                }
                                int stepIndex = button.gridIndex()+(sequence.section()*16);
                                if (sequence.step(stepIndex).note()==(scale.notesPerOctave()*5)) {
                                    sequence.step(stepIndex).setNote(0);
                                } else {
                                    sequence.step(stepIndex).setNote(scale.notesPerOctave()*5);
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }   
            }
        } 
    }
    return false;
}

//----------------------------------------
// Sequence mode
//----------------------------------------

void LaunchpadController::sequenceEnter() {
    selectedNote = 0;
    selectedOctave = 0;
}

void LaunchpadController::sequenceExit() {
}

void LaunchpadController::sequenceDraw() {
    sequenceUpdateNavigation();

    if (_generatorMode && !generatorModeSupported()) {
        setGeneratorMode(false);
    }

    mirrorButton<Navigate>(_style);

    // selected track
    if (buttonState<Shift>()) {
        drawTracksGateAndMute(_engine, _project.playState());
        sequenceDrawSequence();
        return;
    } else {
        drawTracksGateAndSelected(_engine, _project.selectedTrackIndex());
    }

    if (_generatorMode && generatorModeSupported()) {
        sequenceDrawGeneratorMode();
        return;
    }

    if (buttonState<Navigate>()) {
        navigationDraw(_sequence.navigation);
    } else if (buttonState<Layer>()) {
        mirrorButton<Layer>(_style);
        sequenceDrawLayer();
    } else if (buttonState<FirstStep>()) {
        mirrorButton<FirstStep>(_style);
        sequenceDrawStepRange(0);
    } else if (buttonState<LastStep>()) {
        mirrorButton<LastStep>(_style);
        sequenceDrawStepRange(1);
    } else if (buttonState<RunMode>()) {
        mirrorButton<RunMode>(_style);
        sequenceDrawRunMode();
    } else if (buttonState<FollowMode>()) {
        if (_project.selectedTrack().trackMode() == Track::TrackMode::Stochastic) {
            stochasticDrawRestProbability();
            return;
        }
        if (_project.selectedTrack().trackMode() == Track::TrackMode::Arp) {
            arpDrawRestProbability();
            return;
        }
        mirrorButton<FollowMode>(_style);
        sequenceDrawFollowMode();
    } else {
        mirrorButton<Fill>(_style);
        sequenceDrawSequence();
    }
}

void LaunchpadController::sequenceButton(const Button &button, ButtonAction action) {
    if (_generatorMode && generatorModeSupported() && sequenceButtonGeneratorMode(button, action)) {
        return;
    }

    if (action == ButtonAction::Down) {
        if (buttonState<Shift>()) {
            if (button.isScene()) {
                _project.playState().toggleMuteTrack(button.scene());
            }
        } else if (buttonState<Navigate>()) {
            navigationButtonDown(_sequence.navigation, button);
        } else if (buttonState<Layer>()) {
            
            if (button.isGrid()) {
                sequenceSetLayer(button.row, button.col);
            }
        } else if (buttonState<FirstStep>()) {
            if (button.isGrid()) {
                sequenceSetFirstStep(button.gridIndex());
                
            }
        } else if (buttonState<LastStep>()) {
            if (button.isGrid()) {
                sequenceSetLastStep(button.gridIndex());
            }
        } else if (buttonState<RunMode>()) {
            if (button.isGrid()) {
                sequenceSetRunMode(button.gridIndex());
            }
        } else if (buttonState<FollowMode>()) {
            if (button.isGrid()) {
                if (_project.selectedTrack().trackMode() == Track::TrackMode::Stochastic|| _project.selectedTrack().trackMode() == Track::TrackMode::Arp) {
                    sequenceSetRests(button);
                    return;
                }
                sequenceSetFollowMode(button.col);
            } else if (button.isScene()) {
                _project.playState().toggleSoloTrack(button.scene());
            }
        } else if (buttonState<Fill>()) {
            if (button.isScene()) {
                _project.playState().fillTrack(button.scene(), true);
            }
        } else {
            if (button.isGrid()) {
                if (_noteStyle == 0) {
                    sequenceEditStep(button.row, button.col);
                } else {
                    switch (_project.selectedTrack().trackMode()) {
                        case (Track::TrackMode::Note):{
                            if (_project.selectedNoteSequenceLayer()==NoteSequence::Layer::Note) {
                                manageCircuitKeyboard(button);    
                            } else {
                                sequenceEditStep(button.row, button.col);
                            }
                            break;
                        }
                        case Track::TrackMode::Curve:
                            sequenceEditStep(button.row, button.col);
                            break;
                         case (Track::TrackMode::Stochastic): {
                            if (_project.selectedStochasticSequenceLayer()==StochasticSequence::Layer::NoteVariationProbability) {
                                manageStochasticCircuitKeyboard(button);    
                            } else {
                                sequenceEditStep(button.row, button.col);
                            }
                            break;
                         }
                         case Track::TrackMode::Logic:
                            sequenceEditStep(button.row, button.col);
                            break;
                        case (Track::TrackMode::Arp): {
                            if (_project.selectedArpSequenceLayer()==ArpSequence::Layer::Note) {
                                manageArpCircuitKeyboard(button);    
                            } else {
                                sequenceEditStep(button.row, button.col);
                            }
                            break;
                         }
                        default:
                            break;
                        
                    }
                }
                
            } else if (button.isScene()) {
                if (generatorTrackSelectionLocked()) {
                    return;
                }
                _project.setSelectedTrackIndex(button.scene());
            }
        }
    } else if (action == ButtonAction::Up) {
        if (buttonState<Fill>()) {
            if (button.isScene()) {
                _project.playState().fillTrack(button.scene(), false);
            }
        }
        if (button.is<Fill>()) {
            _project.playState().fillAll(false);
        }
        if (button.isGrid()) {
            switch (_project.selectedTrack().trackMode()) {
                case (Track::TrackMode::Arp): {
                        if (_project.selectedArpSequenceLayer()==ArpSequence::Layer::Note) {
                            if (button.row >=3 && button.row <= 4) {
                                if (button.col == 7) { // exit if button is up/down track octave
                                    break;
                                }

                                // exit if grid button is not a note
                                if ((button.row == 3 && button.col == 0) || (button.row == 3 && button.col == 3)) {
                                    break;
                                }

                                auto &arpTrack = _project.selectedTrack().arpTrack();
                                if (arpTrack.midiKeyboard()) {
                                    const auto &sequence = _project.selectedArpSequence();
                                    const auto &scale = sequence.selectedScale(_project.scale());
                                    const Scale &bypasssScale = Scale::get(0);

                                    int ft = -1;
                                    if (button.row == 3) {
                                        ft = getMapValue(semitones, button.col);
                                    } else if (button.row == 4) {
                                        ft = getMapValue(tones, button.col);
                                    }
                                    if (scale.isNotePresent(ft)) {
                                        int noteIndex = scale.getNoteIndex(ft);
                                        selectedNote = noteIndex + (scale.notesPerOctave()*selectedOctave);
                                        fullNoteSelected = false;
                                    } else {
                                        fullNoteSelected = true;

                                    }
                                    int noteIndex = bypasssScale.getNoteIndex(ft);
                                    selectedNote = noteIndex; 
                                    if (sequence.step(selectedNote).gate()) {
                                        break;
                                    }

                                    auto &trackEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();
                                        
                                    trackEngine.removeNote(selectedNote);
                                    trackEngine.setKeyPressed(selectedNote, false);
                                }
                            }
                        }

                    }
                    break;
                default:
                    break;
             }
        }
    } else if (action == ButtonAction::DoublePress) {
        if (!buttonState<Shift>() &&
            !buttonState<Navigate>() &&
            !buttonState<Layer>() &&
            !buttonState<FirstStep>() &&
            !buttonState<LastStep>() &&
            !buttonState<RunMode>() &&
            !buttonState<Fill>() &&
            button.isGrid()) {
            // toggle gate

            switch (_project.selectedTrack().trackMode()) {
            default:
                break;
            case Track::TrackMode::Stochastic: {
                if (button.row >=3 && button.row <=4) {
                    if (button.col == 7) {
                        break;
                    }
                    if ((button.row == 3 && button.col == 0) || (button.row == 3 && button.col == 3)) {
                        break;
                    }
                    auto &sequence = _project.selectedStochasticSequence();
                    int octave = roundDownDivide(fullSelectedNote, 12);
                    int stepNoteCleared = fullSelectedNote - (octave * 12);
                    sequence.step(stepNoteCleared).toggleGate();
                    sequence.step(stepNoteCleared).setNoteOctave(selectedOctave);
                    }
                break;
            }
            case Track::TrackMode::Arp: {
                if (button.row >=3 && button.row <=4) {
                    if (button.col == 7) {
                        break;
                    }
                    if ((button.row == 3 && button.col == 0) || (button.row == 3 && button.col == 3)) {
                        break;
                    }
                    if (_project.selectedTrack().arpTrack().midiKeyboard()) {
                        break;
                    }
                    auto &sequence = _project.selectedArpSequence();
                    auto &trackEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();
                    if (sequence.step(selectedNote).gate()) {
                        trackEngine.removeNote(sequence.step(selectedNote).note());
                    } else {
                        trackEngine.addNote(sequence.step(selectedNote).note(), selectedNote, ArpTrackEngine::Type::Sequencer, selectedOctave);
                    }

                    sequence.step(selectedNote).toggleGate();
                    sequence.step(selectedNote).setNoteOctave(selectedOctave);
                    
                }
                break;
            }
            }
        }
    }
}

bool LaunchpadController::generatorModeSupported() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::NoteSequenceEdit ||
           _manager.uiPageKind() == ControllerManager::UiPageKind::Generator;
}

bool LaunchpadController::generatorModeEditPage() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::NoteSequenceEdit;
}

bool LaunchpadController::generatorModePreviewPage() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::Generator;
}

bool LaunchpadController::generatorTrackSelectionLocked() const {
    auto *pageManager = _manager.pageManager();
    auto *pages = _manager.pages();

    if (!pageManager || !pages) {
        return false;
    }

    auto *top = pageManager->top();
    return top == &pages->acidModeSelect ||
           top == &pages->chaosScopeSelect ||
           top == &pages->wreckPatternWarning;
}

void LaunchpadController::cancelGeneratorMode() {
    if (!_generatorMode) {
        return;
    }

    if (generatorModePreviewPage()) {
        if (auto *pages = _manager.pages()) {
            pages->generator.revert();
        }
    }

    setGeneratorMode(false);
}

LaunchpadController::LaunchpadGenerator LaunchpadController::generatorModeGrid(int gridIndex) const {
    switch (gridIndex) {
    case 0:
        return LaunchpadGenerator::Random;
    case 1:
        return LaunchpadGenerator::AcidPhrase;
    case 9:
        return LaunchpadGenerator::AcidLayer;
    case 2:
        return LaunchpadGenerator::Vandalize;
    case 10:
        return LaunchpadGenerator::Wreck;
    case 3:
        return LaunchpadGenerator::Euclidean;
    case 7:
        return LaunchpadGenerator::Init;
    default:
        return LaunchpadGenerator::None;
    }
}

void LaunchpadController::setGeneratorMode(bool active) {
    _generatorMode = active;
    _generatorApplyArmed = false;
    _generatorApplyCanceled = false;
}

void LaunchpadController::sequenceDrawGeneratorMode() {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            setGridLed(row, col, colorOff());
        }
    }

    const struct {
        int row;
        int col;
        LaunchpadGenerator generator;
    } slots[] = {
        { 0, 0, LaunchpadGenerator::Random },
        { 0, 1, LaunchpadGenerator::AcidPhrase },
        { 1, 1, LaunchpadGenerator::AcidLayer },
        { 0, 2, LaunchpadGenerator::Vandalize },
        { 1, 2, LaunchpadGenerator::Wreck },
        { 0, 3, LaunchpadGenerator::Euclidean },
        { 0, 7, LaunchpadGenerator::Init },
    };

    for (const auto &slot : slots) {
        setGridLed(slot.row, slot.col, _selectedGenerator == slot.generator ? colorGreen() : colorYellow(1));
    }

    bool previewPage = generatorModePreviewPage();
    bool showingPreview = false;
    bool resetState = false;
    if (previewPage) {
        if (auto *pages = _manager.pages()) {
            showingPreview = pages->generator.launchpadShowingPreview();
            resetState = pages->generator.launchpadResetState();
        }
    }

    setFunctionLed(3, colorYellow());
    setFunctionLed(4, previewPage ? (showingPreview ? colorGreen() : colorYellow()) : colorYellow(1));
    setFunctionLed(5, previewPage ? (resetState ? colorYellow() : colorRed()) : colorYellow(1));
    setFunctionLed(6, colorRed());
    setFunctionLed(7, previewPage ? colorGreen() : colorGreen(1));
}

bool LaunchpadController::sequenceButtonGeneratorMode(const Button &button, ButtonAction action) {
    if (action != ButtonAction::Down) {
        return button.isGrid() || button.isFunction() || button.isScene();
    }

    if (button.isGrid()) {
        LaunchpadGenerator generator = generatorModeGrid(button.gridIndex());
        if (generator != LaunchpadGenerator::None) {
            if (generatorModePreviewPage()) {
                if (generator == _selectedGenerator) {
                    if (generator != LaunchpadGenerator::Init) {
                        if (auto *pages = _manager.pages()) {
                            pages->generator.launchpadRandomize();
                        }
                    }
                } else {
                    if (auto *pages = _manager.pages()) {
                        pages->generator.revert();
                    }
                    sequenceOpenGenerator(generator);
                }
                return true;
            }

            if (generatorModeEditPage()) {
                sequenceOpenGenerator(generator);
                return true;
            }
        }
        return true;
    }

    if (button.isScene()) {
        cancelGeneratorMode();
        _project.setSelectedTrackIndex(button.scene());

        if (_project.selectedTrack().trackMode() == Track::TrackMode::Note) {
            if (!generatorModeSupported()) {
                if (auto *pages = _manager.pages()) {
                    pages->top.setMode(TopPage::Mode::SequenceEdit);
                }
            }
            setGeneratorMode(true);
        }

        return true;
    }

    if (!button.isFunction()) {
        cancelGeneratorMode();
        return false;
    }

    if ((button.function() >= 0 && button.function() <= 2) || button.function() == 3 || button.function() == 7) {
        return true;
    }

    if (button.function() == 4) {
        if (generatorModePreviewPage()) {
            if (auto *pages = _manager.pages()) {
                pages->generator.togglePreview();
            }
        }
        return true;
    }

    if (button.function() == 5) {
        if (generatorModePreviewPage()) {
            if (auto *pages = _manager.pages()) {
                pages->generator.init();
            }
        }
        return true;
    }

    if (button.function() == 6) {
        if (generatorModePreviewPage()) {
            if (auto *pages = _manager.pages()) {
                pages->generator.revert();
            }
        }
        setGeneratorMode(false);
        return true;
    }

    cancelGeneratorMode();
    return false;
}

void LaunchpadController::sequenceOpenGenerator(LaunchpadGenerator generator) {
    _selectedGenerator = generator;

    if (auto *pages = _manager.pages()) {
        switch (generator) {
        case LaunchpadGenerator::Random:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::Random);
            break;
        case LaunchpadGenerator::AcidPhrase:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::AcidPhrase);
            break;
        case LaunchpadGenerator::AcidLayer:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::AcidLayer);
            break;
        case LaunchpadGenerator::Vandalize:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::Vandalize);
            break;
        case LaunchpadGenerator::Wreck:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::Wreck);
            break;
        case LaunchpadGenerator::Euclidean:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::Euclidean);
            break;
        case LaunchpadGenerator::Init:
            pages->noteSequenceEdit.openLaunchpadGenerator(NoteSequenceEditPage::LaunchpadGenerator::Init);
            break;
        case LaunchpadGenerator::None:
            break;
        }
    }
}

void LaunchpadController::manageCircuitKeyboard(const Button &button) {
    auto &sequence = _project.selectedNoteSequence();
    const auto &scale = sequence.selectedScale(_project.scale());
    const Scale &bypasssScale = Scale::get(0);
    switch ( _project.selectedNoteSequenceLayer()) {
        case NoteSequence::Layer::Note:
            
            if (button.row >=3 && button.row <= 4) {
                int ft = -1;
                if (button.row == 3) {
                    ft = getMapValue(semitones, button.col);
                } else if (button.row == 4) {
                    ft = getMapValue(tones, button.col);
                }
                if (scale.isNotePresent(ft)) {
                    int noteIndex = scale.getNoteIndex(ft);
                    selectedNote = noteIndex + (scale.notesPerOctave()*selectedOctave);
                    if (button.col == 7) {
                        selectedNote = selectedNote + scale.notesPerOctave();

                    }
                    fullNoteSelected = false;
                } else {
                    fullNoteSelected = true;

                }
                int noteIndex = bypasssScale.getNoteIndex(ft);
                fullSelectedNote = noteIndex + (bypasssScale.notesPerOctave()*selectedOctave);
                if (button.col == 7) {
                    fullSelectedNote = fullSelectedNote + bypasssScale.notesPerOctave();
                }
                         
                    
                break;
            } else if (button.row >= 0 && button.row < 2) {
                auto &sequence = _project.selectedNoteSequence();
                const auto &scale = sequence.selectedScale(_project.scale());
                auto layer = _project.selectedNoteSequenceLayer();
                int ofs = _sequence.navigation.col * 16;
                int linearIndex = button.col + ofs + (button.row*8);
                if (isNoteKeyboardPressed(scale)) { 

                    auto step = sequence.step(linearIndex);
                    if (step.bypassScale()) {
                        sequence.step(linearIndex).setLayerValue(layer, fullSelectedNote);
                    } else {
                        sequence.step(linearIndex).setLayerValue(layer, selectedNote);
                    }
                    if (!sequence.step(linearIndex).gate()) {
                        sequence.step(linearIndex).toggleGate();    
                    }
                } else {
                    sequence.step(linearIndex).toggleGate();
                }
                break;
            } else if (button.row == 6) {
                switch (button.col) {
                    case 0:
                        selectedOctave = -4;
                        break;
                    case 1:
                        selectedOctave = -3;
                        break;
                    case 2:
                        selectedOctave = -2;
                        break;
                    case 3:
                        selectedOctave = -1;
                        break;
                    case 4:
                        selectedOctave = 0;
                        break;
                    case 5:
                        selectedOctave = 1;
                        break;
                    case 6:
                        selectedOctave = 2;
                        break;
                    case 7:
                        selectedOctave = 3;
                        break;
                    default:
                        break;
                }
                break;
            } else if (button.row == 7) {
                if (button.col <=3) {
                    Button btn = Button(3,button.col);
                    sequence.setSecion(button.col);
                    navigationButtonDown(_sequence.navigation, btn);  
                }
            }
        default:
            sequenceEditStep(button.row, button.col);
            break;
        break;
    }   
}

void LaunchpadController::manageStochasticCircuitKeyboard(const Button &button) {
    auto &sequence = _project.selectedStochasticSequence();
    const auto &scale = sequence.selectedScale(_project.scale());
        const Scale &bypasssScale = Scale::get(0);

    switch ( _project.selectedStochasticSequenceLayer()) {
        case StochasticSequence::Layer::NoteVariationProbability:
            
         if (button.row >=3 && button.row <= 4) {

                auto &stochasticTrack = _project.selectedTrack().stochasticTrack();
                auto currentOctave = stochasticTrack.octave();

                if (button.row == 3 && button.col == 7) {
                    stochasticTrack.setOctave(currentOctave+1);
                    return;
                } 
                if (button.row == 4 && button.col == 7) {
                    stochasticTrack.setOctave(currentOctave-1);
                    return;
                }

                int ft = -1;
                if (button.row == 3) {
                    ft = getMapValue(semitones, button.col);
                } else if (button.row == 4) {
                    ft = getMapValue(tones, button.col);
                }
                if (scale.isNotePresent(ft)) {
                    int noteIndex = scale.getNoteIndex(ft);
                    selectedNote = noteIndex + (scale.notesPerOctave()*selectedOctave);
                    if (button.col == 7) {
                        selectedNote = selectedNote + scale.notesPerOctave();

                    }
                    fullNoteSelected = false;
                } else {
                    fullNoteSelected = true;

                }
                int noteIndex = bypasssScale.getNoteIndex(ft);
                fullSelectedNote = noteIndex + (bypasssScale.notesPerOctave()*selectedOctave);                         
                    
                break;
            } else if (button.row >= 0 && button.row < 2) {
                auto &sequence = _project.selectedStochasticSequence();
                int linearIndex = button.col  + (button.row*8);
                int octave = roundDownDivide(fullSelectedNote, 12);
                int stepNoteCleared = fullSelectedNote - (octave*12);
                sequence.step(stepNoteCleared).setNoteVariationProbability(linearIndex);

                break;
            }  else if (button.row == 6) {
                switch (button.col) {
                    case 0:
                        selectedOctave = -4;
                        break;
                    case 1:
                        selectedOctave = -3;
                        break;
                    case 2:
                        selectedOctave = -2;
                        break;
                    case 3:
                        selectedOctave = -1;
                        break;
                    case 4:
                        selectedOctave = 0;
                        break;
                    case 5:
                        selectedOctave = 1;
                        break;
                    case 6:
                        selectedOctave = 2;
                        break;
                    case 7:
                        selectedOctave = 3;
                        break;
                    default:
                        break;
                }
                break;
            
            } else if (button.row == 7) {
                switch (button.col) {
                    case 0:
                        sequence.setUseLoop();
                        break;
                    case 1: 
                        sequence.setClearLoop(true);
                        break;
                    case 2:
                        if (!sequence.useLoop() && !sequence.isEmpty()) {
                            sequence.setReseed(1, false);
                        }
                        break;
                    default:
                        break;
                }
                break;
            }
        default:
            //sequenceEditStep(button.row, button.col);
            break;
        break;
    }   
}

void LaunchpadController::manageArpCircuitKeyboard(const Button &button) {
    const Scale &bypasssScale = Scale::get(0);

    switch ( _project.selectedArpSequenceLayer()) {
        case ArpSequence::Layer::Note:
            
         if (button.row >=3 && button.row <= 4) {

                auto &arpTrack = _project.selectedTrack().arpTrack();
                auto currentOctave = arpTrack.octave();

                if (button.row == 3 && button.col == 7) {
                    arpTrack.setOctave(currentOctave+1);
                    return;
                } 
                if (button.row == 4 && button.col == 7) {
                    arpTrack.setOctave(currentOctave-1);
                    return;
                }

                if ((button.row == 3 && button.col == 0) || (button.row == 3 && button.col == 3)) {
                    return;;
                }

                int ft = -1;
                if (button.row == 3) {
                    ft = getMapValue(semitones, button.col);
                } else if (button.row == 4) {
                    ft = getMapValue(tones, button.col);
                }
                int noteIndex = bypasssScale.getNoteIndex(ft);
                selectedNote = noteIndex; 
                if (arpTrack.midiKeyboard()) {
                    auto &trackEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();
                    trackEngine.addNote(selectedNote, noteIndex, ArpTrackEngine::Type::MIDI, 0);
                    auto &sequence = _project.selectedArpSequence();
                    sequence.step(noteIndex).setNoteOctave(selectedOctave);
                    trackEngine.setKeyPressed(noteIndex, true);
                }
                    
                break;
            } else if (button.row >= 0 && button.row < 2) {
                auto &sequence = _project.selectedArpSequence();
                int linearIndex = button.col  + (button.row*8);

                sequence.step(selectedNote).setGateProbability(linearIndex);

                break;
            } else if (button.row == 6) {
                switch (button.col) {
                    case 0:
                        selectedOctave = -4;
                        break;
                    case 1:
                        selectedOctave = -3;
                        break;
                    case 2:
                        selectedOctave = -2;
                        break;
                    case 3:
                        selectedOctave = -1;
                        break;
                    case 4:
                        selectedOctave = 0;
                        break;
                    case 5:
                        selectedOctave = 1;
                        break;
                    case 6:
                        selectedOctave = 2;
                        break;
                    case 7:
                        selectedOctave = 3;
                        break;
                    default:
                        break;
                }
                break;
            } else if (button.row == 7) {
                if (button.col == 0) {
                    auto &track = _project.selectedTrack().arpTrack();
                    track.toggleMidiKeybaord();
                    track.arpeggiator().setHold(false);
                }
                if (button.col == 1) {
                    auto &track = _project.selectedTrack().arpTrack();
                    if (!track.midiKeyboard()) {
                        break;
                    }
                    track.arpeggiator().setHold(!track.arpeggiator().hold());
                }
                break;
            }
        default:
            //sequenceEditStep(button.row, button.col);
            break;
        break;
    }   
}

bool LaunchpadController::isNoteKeyboardPressed(const Scale &scale) {
    for (int col = 0; col <= 7; ++col) {
        if (buttonState(3, col)) {
            if (semitones.find(col) != semitones.end()) {
                if (scale.isNotePresent(semitones.at(col))) {
                    return true;
                }
            }
        }
        if (buttonState(4, col)) {
            if (tones.find(col) != tones.end()) {
                if (scale.isNotePresent(tones.at(col))) {
                    return true;
                }
            }
        }
   
    }
    return false;
}

void LaunchpadController::sequenceUpdateNavigation() {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note: {
        auto layer = _project.selectedNoteSequenceLayer();
        _sequence.navigation.left = 0;
        _sequence.navigation.right = layer == NoteSequence::Layer::Gate || layer == NoteSequence::Layer::Slide ? 0 : 7;

        auto range = NoteSequence::layerRange(_project.selectedNoteSequenceLayer());
        _sequence.navigation.top = range.max / 8;
        _sequence.navigation.bottom = (range.min - 7) / 8;

        break;
    }
    case Track::TrackMode::Curve: {
        _sequence.navigation.left = 0;
        _sequence.navigation.right = 7;

        auto range = CurveSequence::layerRange(_project.selectedCurveSequenceLayer());
        auto rangeMap = curveSequenceLayerRangeMap[int(_project.selectedCurveSequenceLayer())];
        if (rangeMap) {
            range.min = rangeMap->min[1];
            range.max = rangeMap->max[1];
        }
        _sequence.navigation.top = range.max / 8;
        _sequence.navigation.bottom = (range.min - 7) / 8;
        break;
    }
    case Track::TrackMode::Stochastic: {
        auto layer = _project.selectedStochasticSequenceLayer();
        bool stochasticCircuitEditor = _noteStyle == 1 && layer == StochasticSequence::Layer::NoteVariationProbability;
        _sequence.navigation.left = 0;
        _sequence.navigation.right = layer == StochasticSequence::Layer::Gate || layer == StochasticSequence::Layer::Slide || stochasticCircuitEditor ? 0 : 1;

        auto range = StochasticSequence::layerRange(_project.selectedStochasticSequenceLayer());
        _sequence.navigation.top = stochasticCircuitEditor ? 0 : range.max / 8;
        _sequence.navigation.bottom = (range.min - 7) / 8;

        break;
    }
    case Track::TrackMode::Logic: {
        auto layer = _project.selectedLogicSequenceLayer();
        _sequence.navigation.left = 0;
        _sequence.navigation.right = layer == LogicSequence::Layer::Gate || layer == LogicSequence::Layer::Slide ? 0 : 7;

        auto range = LogicSequence::layerRange(_project.selectedLogicSequenceLayer());
        _sequence.navigation.top = range.max / 8;
        _sequence.navigation.bottom = (range.min - 7) / 8;

        break;
    }
    case Track::TrackMode::Arp: {
        auto layer = _project.selectedArpSequenceLayer();
        bool arpCircuitEditor = _noteStyle == 1 && layer == ArpSequence::Layer::Note;
        _sequence.navigation.left = 0;
        _sequence.navigation.right = layer == ArpSequence::Layer::Gate || layer == ArpSequence::Layer::Slide || arpCircuitEditor ? 0 : 1;

        auto range = ArpSequence::layerRange(_project.selectedArpSequenceLayer());
        _sequence.navigation.top = arpCircuitEditor ? 0 : range.max / 8;
        _sequence.navigation.bottom = (range.min - 7) / 8;

        break;
    }
    default:
        break;
    }
}

void LaunchpadController::sequenceSetLayer(int row, int col) {
    switch (_project.selectedTrack().trackMode()) { 
    case Track::TrackMode::Note: {
        for (int i = 0; i < noteSequenceLayerMapSize; ++i) {
            const auto &item = noteSequenceLayerMap[i];
            if (row == item.row && col == item.col) {
                _project.setSelectedNoteSequenceLayer(NoteSequence::Layer(i));
                break;
            }
        }
        break;
    }
    case Track::TrackMode::Curve: {
        for (int i = 0; i < curveSequenceLayerMapSize; ++i) {
            const auto &item = curveSequenceLayerMap[i];
            if (row == item.row && col == item.col) {
                _project.setSelectedCurveSequenceLayer(CurveSequence::Layer(i));
                break;
            }
        }
        break;
    }
    case Track::TrackMode::Stochastic:   
        for (int i = 0; i < stochasticSequenceLayerMapSize; ++i) {
            const auto &item = stochasticSequenceLayerMap[i];
            if (row == item.row && col == item.col) {
                _project.setSelectedStochasticSequenceLayer(StochasticSequence::Layer(i));
                break;
            }
        }
        break; 
    case Track::TrackMode::Logic:
        for (int i = 0; i < logicSequenceLayerMapSize; ++i) {
            const auto &item = logicSequenceLayerMap[i];
            if (row == item.row && col == item.col) {
                _project.setSelectedLogicSequenceLayer(LogicSequence::Layer(i));
                break;
            }
        }
        break;  
    case Track::TrackMode::Arp:   
        for (int i = 0; i < arpSequenceLayerMapSize; ++i) {
            const auto &item = arpSequenceLayerMap[i];
            if (row == item.row && col == item.col) {
                _project.setSelectedArpSequenceLayer(ArpSequence::Layer(i));
                break;
            }
        }
        break; 
    default:
        break;
    }
}

void LaunchpadController::sequenceSetFirstStep(int step) {
    _startingFirstStep[_project.selectedTrackIndex()] = step;
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        _project.selectedNoteSequence().setFirstStep(step);
        break;
    case Track::TrackMode::Curve:
        _project.selectedCurveSequence().setFirstStep(step);
        break;
    case Track::TrackMode::Stochastic:
        _project.selectedStochasticSequence().setSequenceFirstStep(step);
        break;
    case Track::TrackMode::Logic:
        _project.selectedLogicSequence().setFirstStep(step);
        break;
    case Track::TrackMode::Arp:
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceSetLastStep(int step) {
    _startingLastStep[_project.selectedTrackIndex()] = step;
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        _project.selectedNoteSequence().setLastStep(step);
        break;
    case Track::TrackMode::Curve:
        _project.selectedCurveSequence().setLastStep(step);
        break;
    case Track::TrackMode::Stochastic:
        _project.selectedStochasticSequence().setSequenceLastStep(step);
        break;
    case Track::TrackMode::Logic:
        _project.selectedLogicSequence().setLastStep(step);
        break;
    case Track::TrackMode::Arp:
        break; 
    default:
        break;
    }
}

void LaunchpadController::sequenceSetRunMode(int mode) {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        _project.selectedNoteSequence().setRunMode(Types::RunMode(mode));
        break;
    case Track::TrackMode::Curve:
        _project.selectedCurveSequence().setRunMode(Types::RunMode(mode));
        break;
    case Track::TrackMode::Stochastic:
         _project.selectedStochasticSequence().setRunMode(Types::RunMode(mode));
        break;
    case Track::TrackMode::Logic:
        _project.selectedLogicSequence().setRunMode(Types::RunMode(mode));
        break;
    case Track::TrackMode::Arp:
        _project.selectedTrack().arpTrack().arpeggiator().setMode(Arpeggiator::Mode(mode));
    default:
        break;
    }
}

void LaunchpadController::sequenceSetRests(Button button) {

    int val = 0;

    if (button.row % 2 == 0) {
        val = (button.col);
    } else {
        val = (button.col) + 8;
    }

    if (_project.selectedTrack().trackMode() == Track::TrackMode::Stochastic) {

        if (button.row == 2) {
            _project.selectedStochasticSequence().setRestProbability2(val);
        }
        if (button.row == 3) {
            _project.selectedStochasticSequence().setRestProbability2(val);
        }
        if (button.row == 4) {
            _project.selectedStochasticSequence().setRestProbability4(val);
        }
        if (button.row == 5) {
            _project.selectedStochasticSequence().setRestProbability4(val);
        }
        if (button.row == 6) {
            _project.selectedStochasticSequence().setRestProbability8(val);
        }
        if (button.row == 7) {
            _project.selectedStochasticSequence().setRestProbability8(val);
        }
    } else if (_project.selectedTrack().trackMode() == Track::TrackMode::Arp) {
        if (button.row == 2) {
            _project.selectedArpSequence().setRestProbability2(val);
        }
        if (button.row == 3) {
            _project.selectedArpSequence().setRestProbability2(val);
        }
        if (button.row == 4) {
            _project.selectedArpSequence().setRestProbability4(val);
        }
        if (button.row == 5) {
            _project.selectedArpSequence().setRestProbability4(val);
        }
        if (button.row == 6) {
            _project.selectedArpSequence().setRestProbability8(val);
        }
        if (button.row == 7) {
            _project.selectedArpSequence().setRestProbability8(val);
        }
    }
}

void LaunchpadController::sequenceSetFollowMode(int col) {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        _project.selectedTrack().noteTrack().setPatternFollow(Types::PatternFollow(col));
        break;
    case Track::TrackMode::Curve:
        _project.selectedTrack().curveTrack().setPatternFollow(Types::PatternFollow(col));
        break;
    case Track::TrackMode::Logic:
        _project.selectedTrack().logicTrack().setPatternFollow(Types::PatternFollow(col));
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceToggleStep(int row, int col) {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        sequenceToggleNoteStep(row, col);
        break;
    case Track::TrackMode::Logic:
        sequenceToggleLogicStep(row, col);
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceToggleNoteStep(int row, int col) {
    auto &sequence = _project.selectedNoteSequence();
    auto layer = _project.selectedNoteSequenceLayer();

    int linearIndex = col + _sequence.navigation.col * 8;

    switch (layer) {
    case NoteSequence::Layer::Gate:
    case NoteSequence::Layer::Slide:
        break;
    default:
        sequence.step(linearIndex).toggleGate();
        break;
    }
}

void LaunchpadController::sequenceToggleLogicStep(int row, int col) {
    auto &sequence = _project.selectedLogicSequence();
    auto layer = _project.selectedLogicSequenceLayer();

    int linearIndex = col + _sequence.navigation.col * 8;

    switch (layer) {
    case LogicSequence::Layer::Gate:
    case LogicSequence::Layer::Slide:
        break;
    default:
        sequence.step(linearIndex).toggleGate();
        break;
    }
}

void LaunchpadController::sequenceEditStep(int row, int col) {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        sequenceEditNoteStep(row, col);
        break;
    case Track::TrackMode::Curve:
        sequenceEditCurveStep(row, col);
        break;
    case Track::TrackMode::Stochastic:
        sequenceEditStochasticStep(row, col);
        break;
    case Track::TrackMode::Logic:
        sequenceEditLogicStep(row, col);
        break;
    case Track::TrackMode::Arp:
        sequenceEditArpStep(row, col);
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceEditNoteStep(int row, int col) {
    auto &sequence = _project.selectedNoteSequence();
    auto layer = _project.selectedNoteSequenceLayer();

    int gridIndex = row * 8 + col;
    int linearIndex = col + _sequence.navigation.col * 8;
    int value = (7 - row) + _sequence.navigation.row * 8;

    switch (layer) {
    case NoteSequence::Layer::Gate:
        sequence.step(gridIndex).toggleGate();
        break;
    case NoteSequence::Layer::Slide:
        sequence.step(gridIndex).toggleSlide();
        break;
    case NoteSequence::Layer::Note:
        if (_noteStyle == 0) {
            sequence.step(linearIndex).setLayerValue(layer, value);
        }
        break;
    default:
        sequence.step(linearIndex).setLayerValue(layer, value);
        break;
    }
}

void LaunchpadController::sequenceEditCurveStep(int row, int col) {
    auto &sequence = _project.selectedCurveSequence();
    auto layer = _project.selectedCurveSequenceLayer();
    auto rangeMap = curveSequenceLayerRangeMap[int(_project.selectedCurveSequenceLayer())];

    int linearIndex = col + _sequence.navigation.col * 8;
    int value = (7 - row) + _sequence.navigation.row * 8;
    if (rangeMap) {
        value = rangeMap->unmap(value);
    }

    sequence.step(linearIndex).setLayerValue(layer, value);
}

void LaunchpadController::sequenceEditStochasticStep(int row, int col) {
    auto &sequence = _project.selectedStochasticSequence();
    auto layer = _project.selectedStochasticSequenceLayer();

    int gridIndex = row * 8 + col;

    int linearIndex = col + _sequence.navigation.col * 8;
    int value = (7 - row) + _sequence.navigation.row * 8;

    switch (layer) {
    case StochasticSequence::Layer::Gate:
        if (gridIndex>11) {
            return;
        }
        sequence.step(gridIndex).toggleGate();
        break;
    case StochasticSequence::Layer::Slide:
        if (gridIndex>11) {
            return;
        }
        sequence.step(gridIndex).toggleSlide();
        break;
    default:
        sequence.step(linearIndex).setLayerValue(layer, value);
        break;
    }
}

void LaunchpadController::sequenceEditLogicStep(int row, int col) {
    auto &sequence = _project.selectedLogicSequence();
    auto layer = _project.selectedLogicSequenceLayer();

    int gridIndex = row * 8 + col;
    int linearIndex = col + _sequence.navigation.col * 8;
    int value = (7 - row) + _sequence.navigation.row * 8;

    switch (layer) {
    case LogicSequence::Layer::Gate:
        sequence.step(gridIndex).toggleGate();
        break;
    case LogicSequence::Layer::Slide:
        sequence.step(gridIndex).toggleSlide();
        break;
    default:
        sequence.step(linearIndex).setLayerValue(layer, value);
        break;
    }
}

void LaunchpadController::sequenceEditArpStep(int row, int col) {
    auto &sequence = _project.selectedArpSequence();
    auto layer = _project.selectedArpSequenceLayer();
    auto &trackEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();

    int gridIndex = row * 8 + col;
    int linearIndex = col + _sequence.navigation.col * 8;
    int value = (7 - row) + _sequence.navigation.row * 8;

    switch (layer) {
    case ArpSequence::Layer::Gate: {
            auto &arpTrack = _project.selectedTrack().arpTrack();
            if (arpTrack.midiKeyboard()) {
                    break;
            }
            if (sequence.step(gridIndex).gate()) {
                trackEngine.removeNote(sequence.step(gridIndex).note());
            } else {
                trackEngine.addNote(sequence.step(gridIndex).note(), gridIndex, ArpTrackEngine::Type::Sequencer, sequence.step(gridIndex).noteOctave());
            }
            sequence.step(gridIndex).toggleGate();
        }
        break;
    case ArpSequence::Layer::Slide:
        sequence.step(gridIndex).toggleSlide();
        break;
    default:
        sequence.step(linearIndex).setLayerValue(layer, value);
        break;
    }
}

void LaunchpadController::sequenceDrawLayer() {
    switch (_project.selectedTrack().trackMode()) {
    
    case Track::TrackMode::Note:
        for (int i = 0; i < noteSequenceLayerMapSize; ++i) {
            const auto &item = noteSequenceLayerMap[i];
            bool selected = i == int(_project.selectedNoteSequenceLayer());

            auto playMode = _engine.selectedTrackEngine().as<NoteTrackEngine>().playMode();
            if (playMode == Types::PlayMode::Aligned && (i == 5 || i == 6)) {
                continue;
            } 
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
        break;
    case Track::TrackMode::Curve:
        for (int i = 0; i < curveSequenceLayerMapSize; ++i) {
            const auto &item = curveSequenceLayerMap[i];
            bool selected = i == int(_project.selectedCurveSequenceLayer());
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
        break;
   case Track::TrackMode::Stochastic: 
        for (int i = 0; i < stochasticSequenceLayerMapSize; ++i) {
            const auto &item = stochasticSequenceLayerMap[i];
            bool selected = i == int(_project.selectedStochasticSequenceLayer());
            auto playMode = _engine.selectedTrackEngine().as<StochasticEngine>().playMode();
            if (playMode == Types::PlayMode::Aligned && (i == 5 || i == 6)) {
                continue;
            } 
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
        break;
    case Track::TrackMode::Logic:
        for (int i = 0; i < logicSequenceLayerMapSize; ++i) {
            const auto &item = logicSequenceLayerMap[i];
            bool selected = i == int(_project.selectedLogicSequenceLayer());

            auto playMode = _engine.selectedTrackEngine().as<LogicTrackEngine>().playMode();
            if (playMode == Types::PlayMode::Aligned && (i == 6 || i == 7)) {
                continue;
            } 
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
        break;
   case Track::TrackMode::Arp: 
        for (int i = 0; i < arpSequenceLayerMapSize; ++i) {
            const auto &item = arpSequenceLayerMap[i];
            bool selected = i == int(_project.selectedArpSequenceLayer());
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceDrawStepRange(int highlight) {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note: {
        const auto &sequence = _project.selectedNoteSequence();
        drawRange(sequence.firstStep(), sequence.lastStep(), highlight == 0 ? sequence.firstStep() : sequence.lastStep());
        break;
    }
    case Track::TrackMode::Curve: {
        const auto &sequence = _project.selectedCurveSequence();
        drawRange(sequence.firstStep(), sequence.lastStep(), highlight == 0 ? sequence.firstStep() : sequence.lastStep());
        break;
    }
    case Track::TrackMode::Stochastic: {
        const auto &sequence = _project.selectedStochasticSequence();
        drawRange(sequence.sequenceFirstStep(), sequence.sequenceLastStep(), highlight == 0 ? sequence.sequenceFirstStep() : sequence.sequenceLastStep());
        break;
    }
    case Track::TrackMode::Logic: {
        const auto &sequence = _project.selectedLogicSequence();
        drawRange(sequence.firstStep(), sequence.lastStep(), highlight == 0 ? sequence.firstStep() : sequence.lastStep());
        break;
    }
    case Track::TrackMode::Arp: {
        break;
    }
    default:
        break;
    }
}

void LaunchpadController::stochasticDrawRestProbability() {
    const auto &sequence = _project.selectedStochasticSequence();
    drawBar(2, sequence.restProbability2());
    drawBar(4, sequence.restProbability4());
    drawBar(6, sequence.restProbability8());
}

void LaunchpadController::arpDrawRestProbability() {
    const auto &sequence = _project.selectedArpSequence();
    drawBar(2, sequence.restProbability2());
    drawBar(4, sequence.restProbability4());
    drawBar(6, sequence.restProbability8());
}

void LaunchpadController::sequenceDrawRunMode() {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note: {
        drawEnum(_project.selectedNoteSequence().runMode());
        break;
    }
    case Track::TrackMode::Curve: {
        drawEnum(_project.selectedCurveSequence().runMode());
        break;
    }
    case Track::TrackMode::Stochastic: {
        drawEnum(_project.selectedStochasticSequence().runMode());
        break;
    }
    case Track::TrackMode::Logic: {
        drawEnum(_project.selectedLogicSequence().runMode());
        break;
    }
    case Track::TrackMode::Arp: {
        drawEnum(_project.selectedTrack().arpTrack().arpeggiator().mode());
        break;
    }
    default:
        break;
    }
}

void LaunchpadController::sequenceDrawFollowMode() {
    switch (_project.selectedTrack().trackMode()) {
        case Track::TrackMode::Note: {
            drawEnum(_project.selectedTrack().noteTrack().patternFollow());
            break;
        }
        case Track::TrackMode::Curve: {
            drawEnum(_project.selectedTrack().curveTrack().patternFollow());
            break;
        }
        case Track::TrackMode::Logic: {
            drawEnum(_project.selectedTrack().logicTrack().patternFollow());
            break;
        }
        default:
            break;
    }
}

void LaunchpadController::sequenceDrawSequence() {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        sequenceDrawNoteSequence();
        break;
    case Track::TrackMode::Curve:
        sequenceDrawCurveSequence();
        break;
    case Track::TrackMode::Stochastic:
        sequenceDrawStochasticSequence();
        break;
    case Track::TrackMode::Logic:
        sequenceDrawLogicSequence();
        break;
    case Track::TrackMode::Arp:
        sequenceDrawArpSequence();
        break;
    default:
        break;
    }
}

void LaunchpadController::sequenceDrawNoteSequence() {
    const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    auto sequence = std::ref(_project.selectedNoteSequence());
    if (_project.playState().songState().playing()) {
        auto trackIndex = _project.selectedTrackIndex() ;   
        sequence = std::ref(_project.selectedTrack().noteTrack().sequence(_project.playState().trackState(trackIndex).pattern()));
    }
    auto layer = _project.selectedNoteSequenceLayer();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    switch (layer) {
    case NoteSequence::Layer::Gate:
    case NoteSequence::Layer::Slide:
        drawNoteSequenceBits(sequence, layer, currentStep);
        break;
    case NoteSequence::Layer::Note:
        drawNoteSequenceNotes(sequence, layer, currentStep);
        break;
    case NoteSequence::Layer::Condition:
        drawNoteSequenceDots(sequence, layer, currentStep);
        break;
    default:
        drawNoteSequenceBars(sequence, layer, currentStep);
        break;
    }
}

void LaunchpadController::sequenceDrawLogicSequence() {
    const auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();

    auto sequence = std::ref(_project.selectedLogicSequence());
    if (_project.playState().songState().playing()) {
        auto trackIndex = _project.selectedTrackIndex() ;   
        sequence = std::ref(_project.selectedTrack().logicTrack().sequence(_project.playState().trackState(trackIndex).pattern()));
    }
    auto layer = _project.selectedLogicSequenceLayer();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    switch (layer) {
    case LogicSequence::Layer::Gate:
    case LogicSequence::Layer::Slide:
        drawLogicSequenceBits(sequence, layer, currentStep);
        break;
    case LogicSequence::Layer::Condition:
        drawLogicSequenceDots(sequence, layer, currentStep);
        break;
    default:
        drawLogicSequenceBars(sequence, layer, currentStep);
        break;
    }
}

void LaunchpadController::sequenceDrawStochasticSequence() {
    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();

    auto sequence = std::ref(_project.selectedStochasticSequence());
    if (_project.playState().songState().playing()) {
        auto trackIndex = _project.selectedTrackIndex() ;   
        sequence = std::ref(_project.selectedTrack().stochasticTrack().sequence(_project.playState().trackState(trackIndex).pattern()));
    }
    auto layer = _project.selectedStochasticSequenceLayer();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    switch (layer) {
    case StochasticSequence::Layer::Gate:
    case StochasticSequence::Layer::Slide:
        drawStochasticSequenceBits(sequence, layer, currentStep);
        break;
    case StochasticSequence::Layer::NoteVariationProbability:
        if (_noteStyle == 1) {
            drawStochasticSequenceNotes(sequence, layer, currentStep);
        } else {
            drawStochasticSequenceBars(sequence, layer, currentStep);
        }
        break;
    case StochasticSequence::Layer::Condition:
        drawStochasticSequenceDots(sequence, layer, currentStep);
        break;
    default:
        drawStochasticSequenceBars(sequence, layer, currentStep);
        break;
    }
}

void LaunchpadController::sequenceDrawArpSequence() {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();

    auto sequence = std::ref(_project.selectedArpSequence());
    if (_project.playState().songState().playing()) {
        auto trackIndex = _project.selectedTrackIndex() ;   
        sequence = std::ref(_project.selectedTrack().arpTrack().sequence(_project.playState().trackState(trackIndex).pattern()));
    }
    auto layer = _project.selectedArpSequenceLayer();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    switch (layer) {
    case ArpSequence::Layer::Gate:
    case ArpSequence::Layer::Slide:
        drawArpSequenceBits(sequence, layer, currentStep);
        break;
    case ArpSequence::Layer::Note:
        if (_noteStyle == 1) {
            drawArpSequenceNotes(sequence, layer, currentStep);
        } else {
            drawArpSequenceBars(sequence, layer, currentStep);
        }
        break;
    case ArpSequence::Layer::Condition:
        drawArpSequenceDots(sequence, layer, currentStep);
        break;
    default:
        drawArpSequenceBars(sequence, layer, currentStep);
        break;
    }
}

void LaunchpadController::sequenceDrawCurveSequence() {
    const auto &trackEngine = _engine.selectedTrackEngine().as<CurveTrackEngine>();

    auto sequence = std::ref(_project.selectedCurveSequence());
    if (_project.playState().songState().playing()) {
        auto trackIndex = _project.selectedTrackIndex() ;   
        sequence = std::ref(_project.selectedTrack().curveTrack().sequence(_project.playState().trackState(trackIndex).pattern()));
    }


    auto layer = _project.selectedCurveSequenceLayer();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    switch (layer) {
    case CurveSequence::Layer::Shape:
    case CurveSequence::Layer::ShapeVariation:
    case CurveSequence::Layer::Min:
    case CurveSequence::Layer::Max:
    case CurveSequence::Layer::Gate:
        drawCurveSequenceDots(sequence, layer, currentStep);
        break;
    case CurveSequence::Layer::ShapeVariationProbability:
    case CurveSequence::Layer::GateProbability:
    case CurveSequence::Layer::GateOffset:
    case CurveSequence::Layer::GateLength:
        drawCurveSequenceBars(sequence, layer, currentStep);
        break;
    default:
        break;
    }
}

//----------------------------------------
// Pattern mode
//----------------------------------------

void LaunchpadController::patternEnter() {
}

void LaunchpadController::patternExit() {
    _project.playState().commitLatchedRequests();
}

void LaunchpadController::patternDraw() {
    const auto &playState = _project.playState();

    mirrorButton<Navigate>(_style);
    mirrorButton<Latch>(_style);
    mirrorButton<Sync>(_style);


    if (buttonState<Shift>()) {
        drawTracksGateAndMute(_engine, _project.playState());
    } else {
        drawTracksGateAndSelected(_engine, _project.selectedTrackIndex());
    }

    if (buttonState<Navigate>()) {
        navigationDraw(_pattern.navigation);
    } else {
        for (int trackIndex = 0; trackIndex < 8; ++trackIndex) {
            // draw edited patterns for the visible 8-pattern page
            for (int row = 0; row < 8; ++row) {
                int patternIndex = row - _pattern.navigation.row * 8;
                const auto &track = _project.track(trackIndex);

                switch (track.trackMode()) {
                case Track::TrackMode::Note:
                    if (track.noteTrack().sequence(patternIndex).isEdited()) {
                        setGridLed(row, trackIndex, colorYellow(1));
                    }
                    break;
                case Track::TrackMode::Curve:
                    if (track.curveTrack().sequence(patternIndex).isEdited()) {
                        setGridLed(row, trackIndex, colorRed(1));
                    }
                    break;
                case Track::TrackMode::Stochastic:
                    if (track.stochasticTrack().sequence(patternIndex).isEdited()) {
                        setGridLed(row, trackIndex, colorRed(2));
                    }
                    break;
                case Track::TrackMode::Logic:
                    if (track.logicTrack().sequence(patternIndex).isEdited()) {
                        setGridLed(row, trackIndex, colorRed(2));
                    }
                    break;
                case Track::TrackMode::Arp:
                    if (track.arpTrack().sequence(patternIndex).isEdited()) {
                        setGridLed(row, trackIndex, colorRed(2));
                    }
                    break;
                default:
                    break;
                }
            }

            // draw selected (green) & requested (dim green) patterns
            int pattern = playState.trackState(trackIndex).pattern();
            int requestedPattern = playState.trackState(trackIndex).requestedPattern();
            setGridLed(pattern + _pattern.navigation.row * 8, trackIndex, colorGreen());
            if (pattern != requestedPattern) {
                setGridLed(requestedPattern + _pattern.navigation.row * 8, trackIndex, colorGreen(1));
            }
        }

        mirrorButton<Fill>(_style);
    }
}

void LaunchpadController::patternButton(const Button &button, ButtonAction action) {
    auto &playState = _project.playState();

    if (action == ButtonAction::Down) {
        if (buttonState<Shift>()) {
            if (button.isScene()) {
                _project.playState().toggleMuteTrack(button.scene());
            }
        } else if (buttonState<Navigate>()) {
            navigationButtonDown(_pattern.navigation, button);
        } else if (buttonState<Fill>()) {
            if (button.isScene()) {
                _project.playState().fillTrack(button.scene(), true);
            }
        } else {
            PlayState::ExecuteType executeType = PlayState::ExecuteType::Immediate;
            if (_patternChangeDefault==1) {
                executeType = PlayState::ExecuteType::Synced;
            }
            
            if (buttonState<Latch>()) {
                executeType = PlayState::ExecuteType::Latched;
            } else if (buttonState<Sync>()) {
                executeType = PlayState::ExecuteType::Synced;
                if (_patternChangeDefault==1) {
                    executeType = PlayState::ExecuteType::Immediate;
                }
            }

            if (button.isScene()) {
                int pattern = button.scene() - _pattern.navigation.row * 8;
                playState.selectPattern(pattern, executeType);
            }

            if (button.isGrid()) {
                int pattern = button.row - _pattern.navigation.row * 8;
                int trackIndex = button.col;
                playState.selectTrackPattern(trackIndex, pattern, executeType);
            }
        }
    } else if (action == ButtonAction::Up) {
        if (buttonState<Fill>()) {
            if (button.isScene()) {
                playState.fillTrack(button.scene(), false);
            }
        }
        if (button.is<Fill>()) {
            playState.fillAll(false);
        } else if (button.is<Latch>()) {
            playState.commitLatchedRequests();
        }
    }

}

//----------------------------------------
// Performer mode
//----------------------------------------

void LaunchpadController::performerEnter() {

    _performSelectedLayer = 0;

    for (int i = 0; i<8; ++i) {
        switch (_project.track(i).trackMode()) {
            case Track::TrackMode::Note: {
                    _startingFirstStep[i] = _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).firstStep();
                    _startingLastStep[i] = _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).lastStep();
                }
                break;
            case Track::TrackMode::Curve: {
                    _startingFirstStep[i] = _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).firstStep();
                    _startingLastStep[i] = _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).lastStep();
                }
                break;
            case Track::TrackMode::Stochastic: {
                    _startingFirstStep[i] = _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).sequenceFirstStep();
                    _startingLastStep[i] = _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).sequenceLastStep();
                }
                break;
            case Track::TrackMode::Logic: {
                    _startingFirstStep[i] = _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).firstStep();
                    _startingLastStep[i] = _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).lastStep();
                }
                break;
            default:
                break;
        }
    }

}

void LaunchpadController::performerExit() {
    _performSelectedLayer = -1;
}


void LaunchpadController::performerDraw() {

    sequenceUpdateNavigation();

    mirrorButton<Navigate>(_style);

    // selected track
    if (buttonState<Shift>()) {
        drawTracksGateAndMute(_engine, _project.playState());
        return;
    } else {
        drawTracksGateAndSelected(_engine, _project.selectedTrackIndex());
    }

    if (buttonState<Navigate>()) {
        if (_performSelectedLayer==1) {
            navigationDraw(_performNavigation.navigation);
        }
    } else if (buttonState<Layer>()) {
        mirrorButton<Layer>(_style);
        performDrawLayer();
    } else if (buttonState<FirstStep>()) {
        mirrorButton<FirstStep>(_style);
        sequenceDrawStepRange(0);
    } else if (buttonState<LastStep>()) {
        mirrorButton<LastStep>(_style);
        sequenceDrawStepRange(1);
    } else if (buttonState<RunMode>()) {
        mirrorButton<RunMode>(_style);
        sequenceDrawRunMode();
    } else if (buttonState<FollowMode>()) {
        mirrorButton<FollowMode>(_style);
        sequenceDrawFollowMode();
        setGridLed(2, 0, _performFollowMode == 1 ? colorGreen() : colorYellow());
    } else {
        mirrorButton<Fill>(_style);
        if (_performSelectedLayer == 0) {
            if (_performButton.firstStepButton.row != -1) {
                setGridLed(_performButton.firstStepButton.row, _performButton.firstStepButton.col, colorGreen());
            }
            if (_performButton.lastStepButton.row != -1) {
                setGridLed(_performButton.lastStepButton.row, _performButton.lastStepButton.col, colorGreen());
            }
        } else {
            const auto &scale = Scale::get(0);
            int currentStep = -1;
            for (int row = 0; row < 8; ++row) {
                auto track = _project.track(row);
                for (int col = 0; col < 8; ++col) {
                    int stepIndex = (_performNavigation.navigation.col*8)+col;
                    switch (track.trackMode()) {
                        case Track::TrackMode::Note: {
                                const auto &trackEngine = _engine.trackEngine(row).as<NoteTrackEngine>();
                                if (_performFollowMode || track.noteTrack().patternFollow() == Types::PatternFollow::DispAndLP || track.noteTrack().patternFollow() == Types::PatternFollow::LaunchPad) {
                                    int stepOffset = (std::max(0, trackEngine.currentStep()) / 8) * 8;
                                    stepIndex = stepOffset + col;
                                }
                                auto sequence = track.noteTrack().sequence(_project.selectedPatternIndex());
                                currentStep = trackEngine.currentStep();
                                Color color = colorOff();
                                if (sequence.step(stepIndex).gate()) {
                                    color = colorGreen(2);
                                }
                                if (sequence.step(stepIndex).gate() && sequence.step(stepIndex).note()== (scale.notesPerOctave()*5)) {
                                    color = colorGreen();
                                }                       
                                if (currentStep == stepIndex) {
                                    color = colorRed();
                                }
                                setGridLed(row, col, color);
                            }
                            break;
                        case Track::TrackMode::Stochastic: {
                                const auto &trackEngine = _engine.trackEngine(row).as<StochasticEngine>();
                                auto sequence = track.stochasticTrack().sequence(_project.selectedPatternIndex());
                                currentStep = trackEngine.currentStep();
                                Color color = colorOff();
                                if (sequence.step(stepIndex).gate()) {
                                    color = colorGreen();
                                }
                                if (currentStep == stepIndex) {
                                    color = colorRed();
                                }
                                setGridLed(row, col, color);
                            }
                            break;
                        case Track::TrackMode::Logic: {
                                const auto &trackEngine = _engine.trackEngine(row).as<LogicTrackEngine>();
                                if (_performFollowMode || track.logicTrack().patternFollow() == Types::PatternFollow::DispAndLP || track.logicTrack().patternFollow() == Types::PatternFollow::LaunchPad) {
                                    int stepOffset = (std::max(0, trackEngine.currentStep()) / 8) * 8;
                                    stepIndex = stepOffset + col;
                                }
                                auto sequence = track.logicTrack().sequence(_project.selectedPatternIndex());
                                currentStep = trackEngine.currentStep();
                                Color color = colorOff();
                                if (sequence.step(stepIndex).gate()) {
                                    color = colorGreen(2);
                                }                      
                                if (currentStep == stepIndex) {
                                    color = colorRed();
                                }
                                setGridLed(row, col, color);
                            }
                            break;
                        default:
                            break;

                    }                    
                }
            }

            auto selectedTrack = _project.selectedTrack();

            currentStep = 0;
            switch (selectedTrack.trackMode()) {
                case Track::TrackMode::Note: {
                        auto engine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
                        currentStep = engine.currentStep();
                    }
                    break;
                case Track::TrackMode::Curve: {
                        auto engine = _engine.selectedTrackEngine().as<CurveTrackEngine>();
                        currentStep = engine.currentStep();
                    }
                    break;
                case Track::TrackMode::Stochastic: {
                        auto engine = _engine.selectedTrackEngine().as<StochasticEngine>();
                        currentStep = engine.currentStep();
                    }
                    break;
                case Track::TrackMode::Logic: {
                        auto engine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
                        currentStep = engine.currentStep();
                    }
                    break;
                default:
                    break;
            
            }
            int section = int((currentStep) / 8);
            for (int i = 0; i < 8; ++i) {
                setFunctionLed(section, colorYellow());
            }        
        }
    } 



}

void LaunchpadController::performerButton(const Button &button, ButtonAction action) {

    if (action == ButtonAction::Down) {
    
        if (buttonState<Shift>()) {
            if (button.isScene()) {
                _project.playState().toggleMuteTrack(button.scene());
            }
        } else if (buttonState<Navigate>()) {
            performNavigationButtonDown(_performNavigation.navigation, button);
            
        } else if(buttonState<Layer>()) {
            if (button.isGrid()) {
                performSetLayer(button.row, button.col);
            }
        } else if (buttonState<FirstStep>()) {
            if (button.isGrid()) {
                sequenceSetFirstStep(button.gridIndex());
                
            }
        } else if (buttonState<LastStep>()) {
            if (button.isGrid()) {
                sequenceSetLastStep(button.gridIndex());
            }
        } else if (buttonState<RunMode>()) {
            if (button.isGrid()) {
                sequenceSetRunMode(button.gridIndex());
            }
        } else if (buttonState<FollowMode>()) {
            if (button.isGrid()) {
                sequenceSetFollowMode(button.col);
                if (button.col==0 && button.row == 2) {
                    _performFollowMode = !_performFollowMode;
                }
            } else if (button.isScene()) {
                _project.playState().toggleSoloTrack(button.scene());
            }
        } else if (buttonState<Fill>()) {
            if (button.isScene()) {
                _project.playState().fillTrack(button.scene(), true);
            }
        } else if (button.isGrid()) {

            if (_performSelectedLayer == 0) {
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < 8; ++col) {
                        if (buttonState(row, col)) {
                            if (_performButton.firstStepButton.row != -1) {
                                _performButton.lastStepButton  = button;
                                break;
                            } else {
                                _performButton.firstStepButton = button;
                                break;
                            }
                        }
                    }   
                } 
                int fs = (_performButton.firstStepButton.row * 8) + _performButton.firstStepButton.col;
                int ls = (_performButton.lastStepButton.row * 8) + _performButton.lastStepButton.col;
                if (ls <0) {
                    ls = fs;
                }

                for (int i = 0; i < 8; ++i)  {
                    if (_project.track(i).trackMode() == Track::TrackMode::Note) {
                        _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).setFirstStep(fs);    
                        _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).setLastStep(ls);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Curve) {
                        _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).setFirstStep(fs);    
                        _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).setLastStep(ls);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Stochastic) {
                        _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).setSequenceFirstStep(fs);
                        _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).setSequenceLastStep(ls);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Logic) {
                        _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).setFirstStep(fs);
                        _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).setLastStep(ls);
                    }
                }
            } else if (_performSelectedLayer == 1) {
                auto track = _project.track(button.row);
                int stepIndex = (_performNavigation.navigation.col*8)+button.col;
                
                switch (track.trackMode()) {
                    case Track::TrackMode::Note: {
                            const auto &trackEngine = _engine.trackEngine(button.row).as<NoteTrackEngine>();
                            if (_performFollowMode) {
                                int stepOffset = (std::max(0, trackEngine.currentStep()) / 8) * 8;
                                stepIndex = stepOffset + button.col;
                            }           
                            track.noteTrack().sequence(_project.selectedPatternIndex()).step(stepIndex).toggleGate();
                        }
                        break;
                    case Track::TrackMode::Stochastic: {
                        if (stepIndex<12) {  
                            track.stochasticTrack().sequence(_project.selectedPatternIndex()).step(stepIndex).toggleGate();
                        }
                        break;
                    }
                    case Track::TrackMode::Logic: {
                            const auto &trackEngine = _engine.trackEngine(button.row).as<LogicTrackEngine>();
                            if (_performFollowMode) {
                                int stepOffset = (std::max(0, trackEngine.currentStep()) / 8) * 8;
                                stepIndex = stepOffset + button.col;
                            }           
                            track.logicTrack().sequence(_project.selectedPatternIndex()).step(stepIndex).toggleGate();
                        }
                        break;
                    default:
                        break;
                }
            } 
        } else if (button.isScene()) {
            _project.setSelectedTrackIndex(button.scene());
        }
    } else if (action == ButtonAction::Up) {
        if (buttonState<Fill>()) {
            if (button.isScene()) {
                _project.playState().fillTrack(button.scene(), false);
            }
        }
        if (button.is<Fill>()) {
            _project.playState().fillAll(false);
        }
        if (button.isGrid() && (!buttonState<FirstStep>() && !buttonState<LastStep>())) {
            if (_performButton.firstStepButton.row != -1 && !buttonState(_performButton.firstStepButton.row, _performButton.firstStepButton.col )) {
                _performButton.firstStepButton = Button(-1,-1);
            }
            if (_performButton.lastStepButton.row != - 1 && !buttonState(_performButton.lastStepButton.row, _performButton.lastStepButton.col)) {
                _performButton.lastStepButton = Button(-1,-1);
            }
            
            for (int i = 0; i < 8; ++i)  {
                if (_performButton.firstStepButton.row == -1 && _performButton.lastStepButton.row == -1) {
                    if (_project.track(i).trackMode() == Track::TrackMode::Note) {
                        _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).setFirstStep(_startingFirstStep[i]);    
                        _project.track(i).noteTrack().sequence(_project.selectedPatternIndex()).setLastStep(_startingLastStep[i]);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Curve) {
                        _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).setFirstStep(_startingFirstStep[i]);    
                        _project.track(i).curveTrack().sequence(_project.selectedPatternIndex()).setLastStep(_startingLastStep[i]);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Stochastic) {
                        _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).setSequenceFirstStep(_startingFirstStep[i]);
                        _project.track(i).stochasticTrack().sequence(_project.selectedPatternIndex()).setSequenceLastStep(_startingLastStep[i]);
                    } else if (_project.track(i).trackMode() == Track::TrackMode::Logic) {
                        _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).setFirstStep(_startingFirstStep[i]);
                        _project.track(i).logicTrack().sequence(_project.selectedPatternIndex()).setLastStep(_startingLastStep[i]);
                    }
                }
            }
        }
    }

}

void LaunchpadController::performDrawLayer() {
 for (int i = 0; i < performLayerMapSize; ++i) {
            const auto &item = performLayerMap[i];
            bool selected = i == _performSelectedLayer;
            setGridLed(item.row, item.col, selected ? colorYellow() : colorGreen());
        }
}

void LaunchpadController::performSetLayer(int row, int col) {
        for (int i = 0; i < performLayerMapSize; ++i) {
            const auto &item = performLayerMap[i];
            if (row == item.row && col == item.col) {
                _performSelectedLayer = i;
                break;
            }
        }
}

//----------------------------------------
// Navigation
//----------------------------------------

void LaunchpadController::navigationDraw(const Navigation &navigation) {
    mirrorButton<Left>(_style);
    mirrorButton<Right>(_style);
    mirrorButton<Up>(_style);
    mirrorButton<Down>(_style);

    for (int row = navigation.bottom; row <= navigation.top; ++row) {
        for (int col = navigation.left; col <= navigation.right; ++col) {
            bool selected = row == navigation.row && col == navigation.col;
            setGridLed(3 - row, col, selected ? colorYellow() : colorGreen());
        }
    }
}

void LaunchpadController::navigationButtonDown(Navigation &navigation, const Button &button) {
    int col = button.is<Left>() ? -1 : button.is<Right>() ? 1 : 0;
    navigation.col = clamp(navigation.col + col, int(navigation.left), int(navigation.right));

    int row = button.is<Down>() ? -1 : button.is<Up>() ? 1 : 0;
    navigation.row = clamp(navigation.row + row, int(navigation.bottom), int(navigation.top));

    if (button.isGrid()) {
        int col = button.col;
        int row = 3 - button.row;
        if (col >= navigation.left && col <= navigation.right && row >= navigation.bottom && row <= navigation.top) {
            navigation.col = col;
            navigation.row = row;
        }
    }
}

void LaunchpadController::performNavigationButtonDown(Navigation &navigation, const Button &button) {
    int col = button.is<Left>() ? -1 : button.is<Right>() ? 1 : 0;
    navigation.col = clamp(navigation.col + col, int(navigation.left), int(navigation.right));

    int row = button.is<Down>() ? -1 : button.is<Up>() ? 1 : 0;
    navigation.row = clamp(navigation.row + row, int(navigation.bottom), int(navigation.top));

    if (button.isGrid()) {
        int col = button.col;
        int row = 3 - button.row;
        if (col >= navigation.left && col <= navigation.right && row >= navigation.bottom && row <= navigation.top) {
            navigation.col = col;
            navigation.row = row;
        }
    }
}

//----------------------------------------
// Drawing
//----------------------------------------

void LaunchpadController::drawTracksGateAndSelected(const Engine &engine, int selectedTrack) {
    for (int track = 0; track < 8; ++track) {
        const auto &trackEngine = engine.trackEngine(track);
        bool unmutedActivity = trackEngine.activity() && !trackEngine.mute();
        bool mutedActivity = trackEngine.activity() && trackEngine.mute();
        bool selected = track == selectedTrack;
        setSceneLed(
            track,
            color(
                (mutedActivity || (selected && !unmutedActivity)),
                (unmutedActivity || (selected && !mutedActivity))
            )
        );
    }
}

void LaunchpadController::drawTracksGateAndMute(const Engine &engine, const PlayState &playState) {
    for (int track = 0; track < 8; ++track) {
        const auto &trackEngine = engine.trackEngine(track);
        setSceneLed(
            track,
            color(
                trackEngine.mute(),
                trackEngine.activity()
            )
        );
    }
}

void LaunchpadController::drawRange(int first, int last, int selected) {
    for (int i = first; i <= last; ++i) {
        setGridLed(i, i == selected ? colorYellow() : colorGreen());
    }
}

LaunchpadController::Color LaunchpadController::stepColor(bool active, bool current) const {
    // active current color   red green
    // 0      0       yellow  1   1
    // 0      1       red     1   0
    // 1      0       green   0   1
    // 1      1       red     1   0
    bool red = (!active || current);
    bool green = !current;
    return color(red, green);
}

void LaunchpadController::drawNoteSequenceBits(const NoteSequence &sequence, NoteSequence::Layer layer, int currentStep) {
    const auto &scale = Scale::get(0);

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int stepIndex = row * 8 + col;
            const auto &step = sequence.step(stepIndex);

            Color color = colorOff();
            if (step.gate()) {
                color = colorYellow();
            }
            if (step.layerValue(layer) != 0) {
                color = colorGreen(2);
            }
            if (step.gate() && step.note()== (scale.notesPerOctave()*5)) {
                color = colorGreen();
            }
            if (stepIndex == currentStep) {
                color = colorRed();
            }
            
            setGridLed(row, col, color);
        }
    }
}

void LaunchpadController::drawNoteSequenceBars(const NoteSequence &sequence, NoteSequence::Layer layer, int currentStep) {
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        drawBar(col, step.layerValue(layer), step.gate(), stepIndex == currentStep);
    }
}

void LaunchpadController::drawNoteSequenceDots(const NoteSequence &sequence, NoteSequence::Layer layer, int currentStep) {
    int ofs = _sequence.navigation.row * 8;
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        int value = step.layerValue(layer);
        setGridLed((7 - value) + ofs, col, stepColor(true, stepIndex == currentStep));
    }
}

void LaunchpadController::drawStochasticSequenceDots(const StochasticSequence &sequence, StochasticSequence::Layer layer, int currentStep) {
    int ofs = _sequence.navigation.row * 8;
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        const auto &step = sequence.step(stepIndex);
        if (stepIndex>11) {
            break;
        }
        int value = step.layerValue(layer);
        setGridLed((7 - value) + ofs, col, stepColor(true, stepIndex == currentStep));
    }
}

void LaunchpadController::drawNoteSequenceNotes(const NoteSequence &sequence, NoteSequence::Layer layer, int currentStep) {
    int stepOfs = _sequence.navigation.col * 16;

    auto &scale = sequence.selectedScale(_project.scale());
    const Scale &bypassScale = Scale::get(0);

    if (_noteStyle == 1) {

        // draw steps
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 8; ++col) {
                int stepIndex = col + stepOfs + (row * 8);
                int lastStep = sequence.lastStep();
                followModeAction(currentStep, lastStep);
                const auto &step = sequence.step(stepIndex);   
                Color color = colorOff();
                if (step.gate()) {
                    color = colorGreen(2);
                }
                if (step.gate() && step.note()==(bypassScale.notesPerOctave()*5)) {
                    color = colorGreen(3);
                }
                if (stepIndex == currentStep) {
                    color = colorRed();
                }         
                setGridLed(row, col, color);
            }
        }       

        int rootNote = sequence.selectedRootNote(_model.project().rootNote());

        // draw keyboard
        for (int row = 3; row<=4; ++row) {
            for (int col = 0; col < 8; col++) {
                int index = (col+((row-3)*8));
            
                if (noteGridValues[index]==1) {
                    int n = getMapValue(semitones, index);
                    if (row == 4) {
                        n = getMapValue(tones, col);
                    }
                    if (scale.isNotePresent(n)) {
                        n = scale.getNoteIndex(n);
                        if (col == 7) {
                            n = n + scale.notesPerOctave();
                        }
                        Color color = (selectedNote - (scale.notesPerOctave()*selectedOctave))== n && !fullNoteSelected ? colorYellow() : colorGreen(2);
                        setGridLed(row, col, color);
                    } else {
                        n = bypassScale.getNoteIndex(n);
                        Color color = (fullSelectedNote - (bypassScale.notesPerOctave()*selectedOctave))== n && fullNoteSelected ? colorYellow() : colorGreen(1);
                        setGridLed(row, col, color);
                        
                    }
                }
                if (_engine.state().running()) {
                    const auto &step = sequence.step(currentStep);

                    if (step.bypassScale()) {
                        drawRunningKeyboardCircuit(row, col, step, bypassScale, rootNote);
                    } else {
                        drawRunningKeyboardCircuit(row, col, step, scale, rootNote);
                    }
                }
            }
        }

        // draw octave
        for (int col = 0; col < 8; ++col) {
            int o = getMapValue(octaveMap, col);
            setGridLed(6, col, o==selectedOctave ? colorYellow(): colorYellow(1));

            if (_engine.state().running()) {
                const auto &step = sequence.step(currentStep);
                int s = step.note();

                int octave = 0;
                if (step.bypassScale()) {
                    const Scale &bypassScale = Scale::get(0);
                    octave = s / bypassScale.notesPerOctave();
                } else {
                    octave = s / scale.notesPerOctave();
                }
                for (auto const& x : octaveMap)
                    {
                        if (step.gate() && octave == x.second) {
                            setGridLed(6, x.first, step.gate() && octave == x.second ? colorRed() : colorYellow(1));
                            break;
                        }
                    
                    }
            }
        }

        // draw pages
        for (int col = 0; col < 4; ++col) {
            setGridLed(7, col, sequence.section() == col ? colorYellow(): colorYellow(1));
        }


    } else {
        int noteOfs = _sequence.navigation.row * 8;

        // draw octave lines
        int octave = sequence.selectedScale(_project.scale()).notesPerOctave();
        for (int row = 0; row < 8; ++row) {
            if (modulo(row + noteOfs, octave) == 0) {
                for (int col = 0; col < 2; ++col) {
                    setGridLed(7 - row, col, colorYellow(1));
                }
            }
        }

        // draw notes
        for (int col = 0; col < 8; ++col) {
            int stepIndex = col + _sequence.navigation.col * 8;
            int lastStep = sequence.lastStep();
            followModeAction(currentStep, lastStep);        
            const auto &step = sequence.step(stepIndex);
            setGridLed((7 - step.layerValue(layer)) + noteOfs, col, stepColor(step.gate(), stepIndex == currentStep));
        }
    }
}

void LaunchpadController::drawRunningKeyboardCircuit(int row, int col, const NoteSequence::Step &step, const Scale &scale, int rootNote) {
    int noteOctave = step.note() / scale.notesPerOctave();
    int s = step.note() - (scale.notesPerOctave()*noteOctave);
    if (row == 4 && col == 7) {
        s = step.note();
    }

    for (auto const& x : semitones)
    {
        if (step.gate() && s == scale.getNoteIndex(x.second)) {
            setGridLed(3, x.first, step.gate() && s == scale.getNoteIndex(x.second) ? colorRed() : colorGreen());
            break;
        }
    }
    for (auto const& x : tones)
    {

        if (step.gate() && s == scale.getNoteIndex(x.second)+noteOctave*scale.notesPerOctave()) {
            if (step.note() == rootNote+(scale.notesPerOctave()*noteOctave) && noteOctave == selectedOctave +1) {
                setGridLed(4, 7, colorRed());
            } else {
                setGridLed(4, x.first, step.gate() && s == scale.getNoteIndex(x.second)+noteOctave*scale.notesPerOctave() ? colorRed() : colorGreen());
            }
            break;
        }
    }
}

void LaunchpadController::drawRunningStochasticKeyboardCircuit(int row, int col, const StochasticSequence::Step &step, const Scale &scale, int rootNote) {
    int s = step.note();

    for (auto const& x : semitones)
    {
        if (step.gate() && s == scale.getNoteIndex(x.second)) {
            setGridLed(3, x.first, step.gate() && s == scale.getNoteIndex(x.second) ? colorRed() : colorGreen());
            break;
        }
    }
    for (auto const& x : tones)
    {

        if (step.gate() && s == scale.getNoteIndex(x.second)) {
            setGridLed(4, x.first, step.gate() && s == scale.getNoteIndex(x.second) ? colorRed() : colorGreen());
            break;
        }
    }
}

void LaunchpadController::drawRunningArpKeyboardCircuit(int row, int col, const ArpSequence::Step &step, const Scale &scale, int rootNote) {
    int s = step.note();
    for (auto const& x : semitones)
    {
        if (step.gate() && s == scale.getNoteIndex(x.second)) {
            setGridLed(3, x.first, step.gate() && s == scale.getNoteIndex(x.second) ? colorRed() : colorGreen());
            break;
        }
    }
    for (auto const& x : tones)
    {

        if (step.gate() && s == scale.getNoteIndex(x.second)) {
            setGridLed(4, x.first, step.gate() && s == scale.getNoteIndex(x.second) ? colorRed() : colorGreen());
            break;
        }
    }
}

void LaunchpadController::drawCurveSequenceBars(const CurveSequence &sequence, CurveSequence::Layer layer, int currentStep) {
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        drawBar(col, step.layerValue(layer), true, stepIndex == currentStep);
    }
}

void LaunchpadController::drawCurveSequenceDots(const CurveSequence &sequence, CurveSequence::Layer layer, int currentStep) {
    auto rangeMap = curveSequenceLayerRangeMap[int(_project.selectedCurveSequenceLayer())];
    int ofs = _sequence.navigation.row * 8;
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        int value = step.layerValue(layer);
        if (rangeMap) {
            value = rangeMap->map(value);
        }
        setGridLed((7 - value) + ofs, col, stepColor(true, stepIndex == currentStep));
    }
}

void LaunchpadController::drawStochasticSequenceBits(const StochasticSequence &sequence, StochasticSequence::Layer layer, int currentStep) {
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 8; ++col) {
            int stepIndex = row * 8 + col;
            const auto &step = sequence.step(stepIndex);
            if (stepIndex>11) {
                break;
            }

            Color color = colorOff();
            if (step.gate()) {
                color = colorYellow();
            }
            if (step.layerValue(layer) != 0) {
                color = colorGreen();
            }
            if (stepIndex == currentStep) {
                color = colorRed();
            }
            
            setGridLed(row, col, color);
        }
    }
}

void LaunchpadController::drawStochasticSequenceBars(const StochasticSequence &sequence, StochasticSequence::Layer layer, int currentStep) {
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        if (stepIndex>11) {
            break;
        }
        drawBar(col, step.layerValue(layer), true, stepIndex == currentStep);
    }
}

void LaunchpadController::drawStochasticSequenceNotes(const StochasticSequence &sequence, StochasticSequence::Layer layer, int currentStep) {

    const auto &scale = sequence.selectedScale(_project.scale());
    const Scale &bypassScale = Scale::get(0);
    int selectedChromaticOctave = roundDownDivide(fullSelectedNote, 12);
    int selectedStepIndex = fullSelectedNote - (selectedChromaticOctave * 12);
    int selectedScaleNote = selectedNote - (scale.notesPerOctave() * selectedOctave);
    int selectedChromaticNote = fullSelectedNote - (bypassScale.notesPerOctave() * selectedOctave);
    const auto &step = sequence.step(selectedStepIndex);

    if (step.noteVariationProbability() > 7) {
        drawBarH(0, step.noteVariationProbability(), true, false);
        drawBarH(1, step.noteVariationProbability() - 8, true, false);
    } else if (step.noteVariationProbability() > 0) {
        drawBarH(0, step.noteVariationProbability(), true, false);
        drawBarH(1, -1, false, false);
    } else {
        drawBarH(0, -1, false, false);
        drawBarH(1, -1, false, false);
    }

    auto stochasticEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    for (int col = 0; col < 8; ++col) {
        if (col == stochasticEngine.currentIndex() % 8) {
            if (stochasticEngine.currentIndex() < 8) {
                setCustomGridLed(2, col, Color(1,0));
            } else if (stochasticEngine.currentIndex() >= 8 && stochasticEngine.currentIndex() < 16) {
                setCustomGridLed(2, col, Color(1,1));
            } else if (stochasticEngine.currentIndex() >= 16 && stochasticEngine.currentIndex() < 24) {
                setCustomGridLed(2, col, Color(1,2));
            } else if (stochasticEngine.currentIndex() >= 24 && stochasticEngine.currentIndex() < 32) {
                setCustomGridLed(2, col, Color(1,3));
            } else if (stochasticEngine.currentIndex() >= 40 && stochasticEngine.currentIndex() < 48) {
                setCustomGridLed(2, col, Color(2,0));
            } else if (stochasticEngine.currentIndex() >= 48 && stochasticEngine.currentIndex() < 56) {
                setCustomGridLed(2, col, Color(2,1));
            } else {
                setCustomGridLed(2, col, Color(2,2));
            }
        } else {
            setGridLed(2, col, colorOff());
        }
    }

    int rootNote = sequence.selectedRootNote(_model.project().rootNote());

    // draw keyboard
    for (int row = 3; row <= 4; ++row) {
        for (int col = 0; col < 8; col++) {
            int index = (col + ((row - 3) * 8));

            if (noteGridValues[index] == 1) {
                int n = getMapValue(semitones, index);
                if (row == 4) {
                    n = getMapValue(tones, col);
                }
                if (scale.isNotePresent(n)) {
                    n = scale.getNoteIndex(n);
                    int stepIndex = -1;
                    if (row == 3) {
                        stepIndex = getMapValue(semitones, col);
                    } else if (row == 4) {
                        stepIndex = getMapValue(tones, col);
                    }
                    const auto &step = sequence.step(stepIndex);
                    Color alternate = colorGreen(2);
                    if (step.gate()) {
                        alternate = colorYellow(1);
                    }

                    Color color = selectedScaleNote == n && !fullNoteSelected ? colorYellow() : alternate;
                    setGridLed(row, col, color);
                } else {
                    int stepIndex = -1;
                    if (row == 3) {
                        stepIndex = getMapValue(semitones, col);
                    } else if (row == 4) {
                        stepIndex = getMapValue(tones, col);
                    }
                    const auto &step = sequence.step(stepIndex);
                    Color alternate = colorGreen(1);
                    if (step.gate()) {
                        alternate = colorYellow(1);
                    }
                    n = bypassScale.getNoteIndex(n);
                    Color color = selectedChromaticNote == n && fullNoteSelected ? colorYellow() : alternate;
                    setGridLed(row, col, color);
                }
            }
            if (_engine.state().running()) {
                const auto &step = sequence.step(currentStep);

                if (step.bypassScale()) {
                    drawRunningStochasticKeyboardCircuit(row, col, step, bypassScale, rootNote);
                } else {
                    drawRunningStochasticKeyboardCircuit(row, col, step, scale, rootNote);
                }

                Color transposeUpColor = colorOff();
                switch (_project.selectedTrack().stochasticTrack().octave()) {
                case 0:
                    transposeUpColor = colorOff();
                    break;
                case 1:
                    transposeUpColor = Color(0,1);
                    break;
                case 2:
                    transposeUpColor = Color(0,2);
                    break;
                case 3:
                    transposeUpColor = Color(0,3);
                    break;
                case 4:
                    transposeUpColor = Color(1,0);
                    break;
                case 5:
                    transposeUpColor = Color(1,1);
                    break;
                case 6:
                    transposeUpColor = Color(1,2);
                    break;
                case 7:
                    transposeUpColor = Color(1, 3);
                    break;
                case 8:
                    transposeUpColor = Color(2, 0);
                    break;
                case 9:
                    transposeUpColor = Color(2,1);
                    break;
                case 10:
                    transposeUpColor = Color(2,2);
                    break;
                }

                setCustomGridLed(3, 7, transposeUpColor);

                Color transposeDownColor = colorOff();
                switch (_project.selectedTrack().stochasticTrack().octave()) {
                case 0:
                    transposeDownColor = colorOff();
                    break;
                case -1:
                    transposeDownColor = Color(0,1);
                    break;
                case -2:
                    transposeDownColor = Color(0,2);
                    break;
                case -3:
                    transposeDownColor = Color(0,3);
                    break;
                case -4:
                    transposeDownColor = Color(1,0);
                    break;
                case -5:
                    transposeDownColor = Color(1,1);
                    break;
                case -6:
                    transposeDownColor = Color(1,2);
                    break;
                case -7:
                    transposeDownColor = Color(1, 3);
                    break;
                case -8:
                    transposeDownColor = Color(2, 0);
                    break;
                case -9:
                    transposeDownColor = Color(2,1);
                    break;
                case -10:
                    transposeDownColor = Color(2,2);
                    break;
                }

                setCustomGridLed(4, 7, transposeDownColor);
            }
        }
    }

    // draw octave
    for (int col = 0; col < 8; ++col) {
        int o = getMapValue(octaveMap, col);
        setGridLed(6, col, o == selectedOctave ? colorYellow() : colorYellow(1));

        if (_engine.state().running()) {
            const auto &step = sequence.step(currentStep);
            int octave = step.noteOctave();
            for (auto const &x : octaveMap) {
                if (step.gate() && octave == x.second) {
                    setGridLed(6, x.first, step.gate() && octave == x.second ? colorRed() : colorYellow(1));
                    break;
                }
            }
        }
    }

    // draw options
    setGridLed(7, 0, sequence.useLoop() ? colorYellow() : colorYellow(1));
    setGridLed(7, 1, sequence.clearLoop() ? colorYellow() : colorYellow(1));
    setGridLed(7, 2, !sequence.useLoop() && !sequence.isEmpty() && sequence.reseed() == 1 ? colorYellow() : colorYellow(1));
}

void LaunchpadController::drawArpSequenceBits(const ArpSequence &sequence, ArpSequence::Layer layer, int currentStep) {
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 8; ++col) {
            int stepIndex = row * 8 + col;
            const auto &step = sequence.step(stepIndex);
            if (stepIndex>11) {
                break;
            }

            Color color = colorOff();
            if (step.gate()) {
                color = colorYellow();
            }
            if (step.layerValue(layer) != 0) {
                color = colorGreen();
            }
            if (stepIndex == currentStep) {
                color = colorRed();
            }
            
            setGridLed(row, col, color);
        }
    }
}

void LaunchpadController::drawArpSequenceBars(const ArpSequence &sequence, ArpSequence::Layer layer, int currentStep) {
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        if (stepIndex>11) {
            break;
        }
        drawBar(col, step.layerValue(layer), true, stepIndex == currentStep);
    }
}

void LaunchpadController::drawArpSequenceNotes(const ArpSequence &sequence, ArpSequence::Layer layer, int currentStep) {
    const auto &scale = sequence.selectedScale(_project.scale());
    const auto &track = _project.selectedTrack().arpTrack();
    const Scale &bypassScale = Scale::get(0);
    int stepIndex = selectedNote;
    const auto &step = sequence.step(stepIndex);  
    if (step.gateProbability() > 7) { 
        drawBarH(0, step.gateProbability(), true, false);
        drawBarH(1, step.gateProbability()-8, true, false);
    } else if (step.gateProbability() > 0) {
        drawBarH(0, step.gateProbability(), true, false);
        drawBarH(1, -1, false, false);
    } else {
        drawBarH(0, -1, false, false);
        drawBarH(1, -1, false, false);
    }

    auto &arpEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    for (int col = 0; col < 8; ++col) {
        if (col == arpEngine.currentIndex()%8) {
            if (arpEngine.currentIndex()<8) {
                setCustomGridLed(2, col, Color(1,0));
            } else if (arpEngine.currentIndex() >=8 && arpEngine.currentIndex() <16) {
                setCustomGridLed(2, col, Color(1,1));
            } else if (arpEngine.currentIndex() >=16 && arpEngine.currentIndex() <24) {
                setCustomGridLed(2, col, Color(1,2));
            } else if (arpEngine.currentIndex() >=24 && arpEngine.currentIndex() <32) { 
                setCustomGridLed(2, col, Color(1,3));
            } else if (arpEngine.currentIndex() >=40 && arpEngine.currentIndex() <48) {
                setCustomGridLed(2, col, Color(2,0));
            } else if (arpEngine.currentIndex() >=48 && arpEngine.currentIndex() <56) {
                setCustomGridLed(2, col, Color(2,1));
            } else {
                setCustomGridLed(2, col, Color(2,2));
            }
        } else {
            setGridLed(2, col, colorOff());
        }
    }

    int rootNote = sequence.selectedRootNote(_model.project().rootNote());

    // draw keyboard
    for (int row = 3; row<=4; ++row) {
        for (int col = 0; col < 8; col++) {
            int index = (col+((row-3)*8));
        
            if (noteGridValues[index]==1) {
                int n = getMapValue(semitones, index);
                if (row == 4) {
                    n = getMapValue(tones, col);
                }
                if (scale.isNotePresent(n)) {
                    n = scale.getNoteIndex(n);
                    
                    int stepIndex = -1;
                    if (row == 3) {
                        stepIndex = getMapValue(semitones, col);
                    } else if (row == 4) {
                        stepIndex = getMapValue(tones, col);
                    }
                    const auto &step = sequence.step(stepIndex);
                    Color alternate = colorGreen(2);
                    if (step.gate()) {
                        alternate = colorYellow(1);
                    }

                    Color color =  selectedNote== n && !fullNoteSelected ? colorYellow() : alternate;
                    setGridLed(row, col, color);
                } else {
                    int stepIndex = -1;
                    if (row == 3) {
                        stepIndex = getMapValue(semitones, col);
                    } else if (row == 4) {
                        stepIndex = getMapValue(tones, col);
                    }
                    const auto &step = sequence.step(stepIndex);
                    Color alternate = colorGreen(1);
                    if (step.gate()) {
                        alternate = colorYellow(1);
                    }
                    n = bypassScale.getNoteIndex(n);
                    Color color = selectedNote == n && fullNoteSelected ? colorYellow() : alternate;
                    setGridLed(row, col, color);
                    
                }
            }
            if (_engine.state().running()) {
                const auto &step = sequence.step(currentStep);

                if (step.bypassScale()) {
                    drawRunningArpKeyboardCircuit(row, col, step, bypassScale, rootNote);
                } else {
                    drawRunningArpKeyboardCircuit(row, col, step, scale, rootNote);
                }
            }

            Color transposeUpColor = colorOff();
            switch (_project.selectedTrack().arpTrack().octave()) {
                case 0:
                    transposeUpColor = colorOff();
                    break;
                case 1:
                    transposeUpColor = Color(0,1);
                    break;
                case 2:
                    transposeUpColor = Color(0,2);
                    break;
                case 3:
                    transposeUpColor = Color(0,3);
                    break;
                case 4:
                    transposeUpColor = Color(1,0);
                    break;
                case 5:
                    transposeUpColor = Color(1,1);
                    break;
                case 6:
                    transposeUpColor = Color(1,2);
                    break;
                case 7:
                    transposeUpColor = Color(1, 3);
                    break;
                case 8:
                    transposeUpColor = Color(2, 0);
                    break;
                case 9:
                    transposeUpColor = Color(2,1);
                    break;
                case 10:
                    transposeUpColor = Color(2,2);
                    break;
            }

            setCustomGridLed(3, 7,  transposeUpColor);

            Color transposeDownColor = colorOff();
            switch (_project.selectedTrack().arpTrack().octave()) {
                case 0:
                    transposeDownColor = colorOff();
                    break;
                case -1:
                    transposeDownColor = Color(0,1);
                    break;
                case -2:
                    transposeDownColor = Color(0,2);
                    break;
                case -3:
                    transposeDownColor = Color(0,3);
                    break;
                case -4:
                    transposeDownColor = Color(1,0);
                    break;
                case -5:
                    transposeDownColor = Color(1,1);
                    break;
                case -6:
                    transposeDownColor = Color(1,2);
                    break;
                case -7:
                    transposeDownColor = Color(1, 3);
                    break;
                case -8:
                    transposeDownColor = Color(2, 0);
                    break;
                case -9:
                    transposeDownColor = Color(2,1);
                    break;
                case -10:
                    transposeDownColor = Color(2,2);
                    break;
            }

            setCustomGridLed(4, 7,  transposeDownColor);
        }
    }

    // draw octave
    for (int col = 0; col < 8; ++col) {
        int o = getMapValue(octaveMap, col);
        setGridLed(6, col, o==selectedOctave ? colorYellow(): colorYellow(1));

        if (_engine.state().running()) {
            const auto &step = sequence.step(currentStep);
            int octave = step.noteOctave();
            for (auto const& x : octaveMap)
                {
                    if (step.gate() && octave == x.second) {
                        setGridLed(6, x.first, step.gate() && octave == x.second ? colorRed() : colorYellow(1));
                        break;
                    }
                
                }
        }
    }
    // draw options
    setGridLed(7, 0, track.midiKeyboard() ? colorYellow(): colorYellow(1));
    setGridLed(7, 1, track.arpeggiator().hold() ? colorYellow(): colorYellow(1));
}

void LaunchpadController::drawArpSequenceDots(const ArpSequence &sequence, ArpSequence::Layer layer, int currentStep) {
    int ofs = _sequence.navigation.row * 8;
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        const auto &step = sequence.step(stepIndex);
        if (stepIndex>11) {
            break;
        }
        int value = step.layerValue(layer);
        setGridLed((7 - value) + ofs, col, stepColor(true, stepIndex == currentStep));
    }
}

void LaunchpadController::drawLogicSequenceBits(const LogicSequence &sequence, LogicSequence::Layer layer, int currentStep) {
     const auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int stepIndex = row * 8 + col;
            const auto &step = sequence.step(stepIndex);

            Color color = colorOff();
            if (step.gate()) {
                color = colorYellow();
            }
            if (step.layerValue(layer) != 0) {
                color = colorGreen(2);
            }
            if (stepIndex == currentStep) {
                if (trackEngine.gateOutput(stepIndex)) {
                    color = colorYellow();
                } else {
                    color = colorRed();
                }
            }
            
            setGridLed(row, col, color);
        }
    }
}

void LaunchpadController::drawLogicSequenceBars(const LogicSequence &sequence, LogicSequence::Layer layer, int currentStep) {
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        drawBar(col, step.layerValue(layer), step.gate(), stepIndex == currentStep);
    }
}

void LaunchpadController::drawLogicSequenceDots(const LogicSequence &sequence, LogicSequence::Layer layer, int currentStep) {
    int ofs = _sequence.navigation.row * 8;
    for (int col = 0; col < 8; ++col) {
        int stepIndex = col + _sequence.navigation.col * 8;
        int lastStep = sequence.lastStep();
        followModeAction(currentStep, lastStep);
        const auto &step = sequence.step(stepIndex);
        int value = step.layerValue(layer);
        setGridLed((7 - value) + ofs, col, stepColor(true, stepIndex == currentStep));
    }
}

void LaunchpadController::followModeAction(int currentStep, int lastStep) {
    if (_engine.state().running()) {
        bool followMode = false;
        int g = currentStep / 8;
        switch (_project.selectedTrack().trackMode()) {
            case Track::TrackMode::Note: {
                auto mode = _project.selectedTrack().noteTrack().patternFollow();
                if (mode == Types::PatternFollow::LaunchPad || mode == Types::PatternFollow::DispAndLP) {
                    followMode = true;
                    if (_project.selectedNoteSequenceLayer()==NoteSequence::Layer::Note) {
                        g = currentStep / 16;
                    }
                }
                break;
            }
            case Track::TrackMode::Curve: {
                auto mode = _project.selectedTrack().curveTrack().patternFollow();
                if (mode == Types::PatternFollow::LaunchPad || mode == Types::PatternFollow::DispAndLP) {
                    followMode = true;
                }
                break;
            }
            case Track::TrackMode::Logic: {
                auto mode = _project.selectedTrack().logicTrack().patternFollow();
                if (mode == Types::PatternFollow::LaunchPad || mode == Types::PatternFollow::DispAndLP) {
                    followMode = true;
                }
                break;
            }
            default:
                break;
        }
            
        if (followMode) {
            int row = nativationRowMap.at(_sequence.navigation.row);
            Button button = Button(row,g);
            if (currentStep == lastStep) {
                button = Button(row,0);
            }
            navigationButtonDown(_sequence.navigation, button);    
        }
    }
}

void LaunchpadController::drawBar(int col, int value, bool active, bool current) {
    int ofs = _sequence.navigation.row * 8;
    if (value >= 0) {
        for (int i = 0; i <= value; ++i) {
            setGridLed((7 - i) + ofs, col, colorYellow());
        }
    } else {
        for (int i = 0; i >= value; --i) {
            setGridLed((7 - i) + ofs, col, colorYellow());
        }
    }
    setGridLed((7 - value) + ofs, col, stepColor(active, current));
}

void LaunchpadController::drawBarH(int col, int value, bool active, bool current) {
    for (int i = 0; i < 8; ++i) {
        setGridLed(col, i, colorOff());
    }
    if (!active || value < 0) {
        return;
    }
    for (int i = 0; i <= value && i < 8; ++i) {
        setGridLed(col, i, colorYellow());
    }
    if (value < 8) {
        setGridLed(col, value, stepColor(active, current));
    }
}

//----------------------------------------
// Led handling
//----------------------------------------

void LaunchpadController::setGridLed(int row, int col, Color color) {
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        _device->setLed(row, col, color, _style);
    }
}

void LaunchpadController::setCustomGridLed(int row, int col, Color color) {
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        _device->setCustomLed(row, col, color, _style);
    }
}

void LaunchpadController::setGridLed(int index, Color color) {    if (index >= 0 && index < 64) {
        _device->setLed(index / 8, index % 8, color, _style);
    }
}

void LaunchpadController::setFunctionLed(int col, Color color) {    if (col >= 0 && col < 8) {
        _device->setLed(LaunchpadDevice::FunctionRow, col, color, _style);
    }
}

void LaunchpadController::setSceneLed(int col, Color color) {    if (col >= 0 && col < 8) {
        _device->setLed(LaunchpadDevice::SceneRow, col, color, _style);
    }
}

//----------------------------------------
// Button handling
//----------------------------------------

void LaunchpadController::dispatchButtonEvent(const Button& button, ButtonAction action) {
    if (globalButton(button, action)) {
        return;
    }

    CALL_MODE_FUNCTION(_mode, Button, button, action);
}

void LaunchpadController::buttonDown(int row, int col) {
    Button button(row, col);

    dispatchButtonEvent(button, ButtonAction::Down);

    uint32_t currentTicks = os::ticks();
    uint32_t deltaTicks = currentTicks - _buttonTracker.lastTicks;

    if (button != _buttonTracker.lastButton || deltaTicks > os::time::ms(300)) {
        _buttonTracker.count = 1;
    } else {
        ++_buttonTracker.count;
    }

    _buttonTracker.lastButton = button;
    _buttonTracker.lastTicks = currentTicks;

    if (_buttonTracker.count == 1) dispatchButtonEvent(button, ButtonAction::Press);
    if (_buttonTracker.count == 2) dispatchButtonEvent(button, ButtonAction::DoublePress);
}

void LaunchpadController::buttonUp(int row, int col) {
    dispatchButtonEvent(Button(row, col), ButtonAction::Up);
}

bool LaunchpadController::buttonState(int row, int col) const {
    return _device->buttonState(row, col);
}
