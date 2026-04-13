#include "ArpTrackEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/ArpSequence.h"
#include "model/Scale.h"
#include "ui/MatrixMap.h"
#include <climits>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <random>


static Random rng;

static int activePitchSlotCount(const Scale &scale) {
    return clamp(scale.notesPerOctave(), 1, 12);
}

static int wrappedPitchSlotIndex(int note, int slotCount) {
    int index = note % slotCount;
    if (index < 0) {
        index += slotCount;
    }
    return index;
}

bool sortTaskByProbRev(const ArpStep& lhs, const ArpStep& rhs) {
    return lhs.probability() > rhs.probability();
}

// evaluate if step gate is active
static bool evalStepGate(const ArpSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.gateProbability() + probabilityBias, -1, ArpSequence::GateProbability::Max);
    if (probability==0) {
        return false;
    }
    return step.gate() && int(rng.nextRange(ArpSequence::GateProbability::Range)) <= probability;
}

static bool evalMIDIStepGate(const ArpSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.gateProbability() + probabilityBias, -1, ArpSequence::GateProbability::Max);
    if (probability==0) {
        return false;
    }
    return int(rng.nextRange(ArpSequence::GateProbability::Range)) <= probability;
}

// evaluate if step gate is active
 int ArpTrackEngine::evalRestProbability(const ArpSequence &sequence) {
    int sum = 0;
    std::vector<ArpStep> probability;
    for (int i = 0; i < 4; i++) {

        switch (i) {
            case 0: {
                probability.insert(probability.end(), ArpStep(i, sequence.restProbability()));
            }
                break;
            case 1: {
                probability.insert(probability.end(), ArpStep(i, sequence.restProbability2()));
                break;
            }
            case 2: {
                probability.insert(probability.end(), ArpStep(i, sequence.restProbability4()));
                break;
            }
            case 3: {
                probability.insert(probability.end(), ArpStep(i, sequence.restProbability8()));
                break;
            }
            default:
                break;
        }

        sum = sum + probability.at(i).probability();
    }
    if (sum==0) { return -1;}
    std::sort (std::begin(probability), std::end(probability), sortTaskByProbRev);
    int stepIndex = getNextWeightedPitch(probability, probability.size());
    switch (stepIndex) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 3;
        case 3:
            return 7;
    }
    return -1;
}

// evaluate step condition
static bool evalStepCondition(const ArpSequence::Step &step, int iteration, bool fill, bool &prevCondition) {
    auto condition = step.condition();
    switch (condition) {
    case Types::Condition::Off:                                         return true;
    case Types::Condition::Fill:        prevCondition = fill;           return prevCondition;
    case Types::Condition::NotFill:     prevCondition = !fill;          return prevCondition;
    case Types::Condition::Pre:                                         return prevCondition;
    case Types::Condition::NotPre:                                      return !prevCondition;
    case Types::Condition::First:       prevCondition = iteration == 0; return prevCondition;
    case Types::Condition::NotFirst:    prevCondition = iteration != 0; return prevCondition;
    default:
        int index = int(condition);
        if (index >= int(Types::Condition::Loop) && index < int(Types::Condition::Last)) {
            auto loop = Types::conditionLoop(condition);
            prevCondition = iteration % loop.base == loop.offset;
            if (loop.invert) prevCondition = !prevCondition;
            return prevCondition;
        }
    }
    return true;
}

// evaluate step retrigger count
static int evalStepRetrigger(const ArpSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.retriggerProbability() + probabilityBias, -1, ArpSequence::RetriggerProbability::Max);
    return int(rng.nextRange(ArpSequence::RetriggerProbability::Range)) <= probability ? step.retrigger() + 1 : 1;
}

// evaluate step length
static int evalStepLength(const ArpSequence::Step &step, int lengthBias) {
    int length = ArpSequence::Length::clamp(step.length() + lengthBias) + 1;
    int probability = step.lengthVariationProbability();
    if (int(rng.nextRange(ArpSequence::LengthVariationProbability::Range)) <= probability) {
        int offset = step.lengthVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.lengthVariationRange()) + 1);
        if (step.lengthVariationRange() < 0) {
            offset = -offset;
        }
        length = clamp(length + offset, 0, ArpSequence::Length::Range);
    }
    return length;
}

// evaluate transposition
static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

// evaluate note voltage
static float evalStepNote(const ArpSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, ArpSequence sequence, bool useVariation = true) {

    // Bypass-scale semantics are meaningful only on chromatic scales.
    // On non-chromatic scales (for example User Scale in Voltage mode),
    // keep evaluation on the selected scale instead of forcing semitone scale.
    if (step.bypassScale() && scale.isChromatic()) {
        const Scale &bypassScale = Scale::get(0);
        int note = step.note() + evalTransposition(bypassScale, octave, transpose);
        int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, ArpSequence::NoteOctaveProbability::Max);
        if (step.noteOctaveProbability()==0) {
            probability = 0;
        }
        if (useVariation && int(rng.nextRange(ArpSequence::NoteOctaveProbability::Range)) <= probability && probability!= 0) {
            int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
            note = ArpSequence::Note::clamp(note + (bypassScale.notesPerOctave()*oct));
        }
        if (step.noteVariationProbability() == 0) {
             probability = 0;
        }
        if (useVariation && int(rng.nextRange(ArpSequence::NoteVariationProbability::Range)) <= probability) {
            int offset = step.noteVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.noteVariationRange()) + 1);
            int offsetOctave = roundDownDivide(offset, scale.notesPerOctave());
            int offSetCleared = offset - (offsetOctave*scale.notesPerOctave());
            while (!scale.isNotePresent(offSetCleared)) {
                offset++;
                offsetOctave = roundDownDivide(offset, scale.notesPerOctave());
                offSetCleared = offset - (offsetOctave*scale.notesPerOctave());
            }
            if (step.noteVariationRange() < 0) {
                offset = -offset;
            }
            note = NoteSequence::Note::clamp(note + offset);
        }
        return bypassScale.noteToVolts(note) + (bypassScale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
    }
    int note = step.note() + evalTransposition(scale, octave, transpose);
    int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, ArpSequence::NoteOctaveProbability::Max);
    if (useVariation && int(rng.nextRange(ArpSequence::NoteOctaveProbability::Range)) <= probability && probability != 0) {
        int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
        note = ArpSequence::Note::clamp(note + (scale.notesPerOctave()*oct));
    }
    if (useVariation && int(rng.nextRange(ArpSequence::NoteVariationProbability::Range)) <= probability) {
        int offset = step.noteVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.noteVariationRange()) + 1);
        if (step.noteVariationRange() < 0) {
            offset = -offset;
        }
        note = NoteSequence::Note::clamp(note + offset);
    }
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

int ArpTrackEngine::getNextWeightedPitch(std::vector<ArpStep> distr, int notesPerOctave) {
        int total_weights = 0;

        for(int i = 0; i < notesPerOctave; i++) {
            total_weights += distr.at(i % notesPerOctave).probability();
        }

        int rnd = rng.nextRange(total_weights);

        for(int i = 0; i < notesPerOctave; i++) {
            int weight = distr.at(i % notesPerOctave).probability();
            if (rnd <= weight && weight > 0) {
                return distr.at(i % notesPerOctave).index();
            }
            rnd -= weight;
        }
        return -1;
    }

void ArpTrackEngine::reset() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    _prevCondition = false;
    _activity = false;
    _gateOutput = false;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
        _cvOutputTarget = 0.f;
    }
    _slideActive = false;
    _gateQueue.clear();
    _cvQueue.clear();
    _recordHistory.clear();

    changePattern();

    _stepIndex = -1;
    _noteIndex = 0;
    _noteOrder = 0;

    _octave = 0;
    _octaveDirection = 0;

    _noteCount = 0;

    for (int i = 0; i < int(_notes.size()); ++i) {
        ++_noteCount;
        uint32_t order = _notes.at(i).order;
        if (order > _noteOrder) {
            _noteOrder = order + 1;
        }
    }
}

void ArpTrackEngine::restart() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
}

TrackEngine::TickResult ArpTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = activeSequence();
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            int abstoluteStep = int(linkData->relativeTick / linkData->divisor);
            recordStep(tick+1, linkData->divisor);
            if (abstoluteStep == 0 ||abstoluteStep >= _model.project().recordDelay()+1) {
                    recordStep(tick+1, linkData->divisor);
                }
            triggerStep(tick, linkData->divisor);
        }
    } else {

        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        if (_arpTrack.midiKeyboard() ) {
            divisor = _arpeggiator.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        }
        uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
        uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;

        if (int(_model.project().stepsToStop()) != 0 && int(relativeTick / divisor) == int(_model.project().stepsToStop())) {
            _engine.clockStop();
        }

        // handle reset measure
        if (relativeTick == 0) {
            reset();
            _currentStageRepeat = 1;
        }

        // advance sequence
        switch (_arpTrack.playMode()) {
        case Types::PlayMode::Aligned:
            if (relativeTick % divisor == 0) {
                int abstoluteStep = int(relativeTick / divisor);
                //_sequenceState.advanceAligned(abstoluteStep, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);

                if (abstoluteStep == 0 ||abstoluteStep >= _model.project().recordDelay()+1) {
                    recordStep(tick+1, divisor);
                }
                triggerStep(tick, divisor);

                /*const auto &step = sequence.step(_sequenceState.step());
                if (step.gateOffset()<0) { //TODO verify
                    _sequenceState.calculateNextStepAligned(
                            (relativeTick + divisor) / divisor,
                            sequence.runMode(),
                            sequence.firstStep(),
                            sequence.lastStep(),
                            rng
                        );
                        
                    triggerStep(tick + divisor, divisor, true);
                }*/
            }
            break;
        case Types::PlayMode::Free:
            break;
        case Types::PlayMode::Last:
            break;
        }

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
    }

    auto &midiOutputEngine = _engine.midiOutputEngine();

    TickResult result = TickResult::NoUpdate;

    while (!_gateQueue.empty() && tick >= _gateQueue.front().tick) {
        if (!_monitorOverrideActive) {
            result |= TickResult::GateUpdate;
            _activity = _gateQueue.front().gate;
            _gateOutput = (!mute() || fill()) && _activity;
            midiOutputEngine.sendGate(_track.trackIndex(), _gateOutput);
        }
        _gateQueue.pop();

    }

    while (!_cvQueue.empty() && tick >= _cvQueue.front().tick) {
        if (!mute() || _arpTrack.cvUpdateMode() == ArpTrack::CvUpdateMode::Always) {
            if (!_monitorOverrideActive) {
                result |= TickResult::CvUpdate;
                _cvOutputTarget = _cvQueue.front().cv;
                _slideActive = _cvQueue.front().slide;
                midiOutputEngine.sendCv(_track.trackIndex(), _cvOutputTarget);
                midiOutputEngine.sendSlide(_track.trackIndex(), _slideActive);
            }
        }
        _cvQueue.pop();
    }

    return result;
}

void ArpTrackEngine::update(float dt) {
    _noteCount = _notes.size();
    bool running = _engine.state().running();

    const auto &sequence = activeSequence();
    const auto &scale = sequence.selectedScale(_model.project().scale());
    int rootNote = sequence.selectedRootNote(_model.project().rootNote());
    int octave = _arpTrack.octave();
    int transpose = _arpTrack.transpose();

    // helper to send gate/cv from monitoring to midi output engine
    auto sendToMidiOutputEngine = [this] (bool gate, float cv = 0.f) {
        auto &midiOutputEngine = _engine.midiOutputEngine();
        midiOutputEngine.sendGate(_track.trackIndex(), gate);
        if (gate) {
            midiOutputEngine.sendCv(_track.trackIndex(), cv);
            midiOutputEngine.sendSlide(_track.trackIndex(), false);
        }
    };

    // set monitor override
    auto setOverride = [&] (float cv) {
        _cvOutputTarget = cv;
        _activity = _gateOutput = true;
        _monitorOverrideActive = true;
        // pass through to midi engine
        sendToMidiOutputEngine(true, cv);
    };

    // clear monitor override
    auto clearOverride = [&] () {
        if (_monitorOverrideActive) {
            _activity = _gateOutput = false;
            _monitorOverrideActive = false;
            sendToMidiOutputEngine(false);
        }
    };

    // check for step monitoring
    bool stepMonitoring = (!running && _monitorStepIndex >= 0);

    // check for live monitoring
    auto monitorMode = _model.project().monitorMode();
    bool liveMonitoring =
        (monitorMode == Types::MonitorMode::Always) ||
        (monitorMode == Types::MonitorMode::Stopped && !running);

    if (stepMonitoring) {
        const auto &step = sequence.step(_monitorStepIndex);
        setOverride(evalStepNote(step, 0, scale, rootNote, octave, transpose,  sequence, true));
    } else if (liveMonitoring && _recordHistory.isNoteActive() && !running) {
        int note = noteFromMidiNote(_recordHistory.activeNote()) + evalTransposition(scale, octave, transpose);
        setOverride(scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f));
    } else {
        clearOverride();
    }

    if (_slideActive && _arpTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, _arpTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void ArpTrackEngine::changePattern() {

    if (_prevPattern != pattern()) {
        _notes.clear();
        _prevPattern = pattern();
    }
 
    _sequence = &_arpTrack.sequence(pattern());
    _fillSequence = &_arpTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
    _previewSequence = nullptr;
}

void ArpTrackEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _noteCount = _notes.size();
    _recordHistory.write(tick, message);

    const auto &sequence = activeSequence();
    const auto &scale = sequence.selectedScale(_model.project().scale());
    const int pitchSlots = activePitchSlotCount(scale);
    int octave = _arpTrack.octave();
    int transpose = _arpTrack.transpose();
    
    bool running = _engine.state().running();
    if (!running) {
        return;
    }

    if (message.isNoteOff()) {
        int note = noteFromMidiNote(message.note())  + evalTransposition(scale, octave, transpose);
        int stepNoteCleared = wrappedPitchSlotIndex(note, pitchSlots);
        if (sequence.step(stepNoteCleared).gate()) {
            return;
        }

        removeNote(note);
        setKeyPressed(note, false);
    }

    if (message.isNoteOn()) {
        int note = noteFromMidiNote(message.note()) + evalTransposition(scale, octave, transpose);
        
        int stepNoteCleared = wrappedPitchSlotIndex(note, pitchSlots);
        if (sequence.step(stepNoteCleared).gate()) {
            return;
        }
        //sequence.step(stepNoteCleared).setNoteOctave(octave);
        addNote(note, stepNoteCleared, ArpTrackEngine::Type::MIDI, octave);

    
        setKeyPressed(note, true);
    }
    /*if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord) {
        _stepRecorder.process(message, *_sequence, [this] (int midiNote) { return noteFromMidiNote(midiNote); });
        if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
            _sequence->setCurrentRecordStep(_stepRecorder.stepIndex(), true);
        } else {
            _sequence->setCurrentRecordStep(_stepRecorder.stepIndex(), false);
        }
    }*/
}

void ArpTrackEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void ArpTrackEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;

    // in step record mode, select step to start recording recording from
    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord &&
        index >= _sequence->firstStep() && index <= _sequence->lastStep()) {
        _stepRecorder.setStepIndex(index);
    }
}
void ArpTrackEngine::triggerStep(uint32_t tick, uint32_t divisor, bool forNextStep) {
    _noteCount = _notes.size();
    int octave = _arpTrack.octave();
    int transpose = _arpTrack.transpose();
    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _arpTrack.fillMode() == ArpTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _arpTrack.fillMode() == ArpTrack::FillMode::NextPattern;
    bool useFillCondition = fillStep && _arpTrack.fillMode() == ArpTrack::FillMode::Condition;

    const auto &sequence = activeSequence();
    const auto &evalSequence = useFillSequence ? *_fillSequence : activeSequence();
    const auto &sequenceScale = sequence.selectedScale(_model.project().scale());
    const int pitchSlots = activePitchSlotCount(sequenceScale);

    if (!_arpeggiator.hold() && !isKeyPressed()) {
        for (int i = 0; i < _noteCount; ++i) {
            if (_notes.at(i).type == Type::MIDI) {
                removeNote(_notes.at(i).note);
            }
        }
    }

    if (_noteCount == 0 && sequence.hasSteps()) {
        for (int i = 0; i < pitchSlots; ++i) {
            if (sequence.step(i).gate()) {
                addNote(sequence.step(i).note(), i, Type::Sequencer, sequence.step(i).noteOctave());
            }
        }
    }
    if (!sequence.hasSteps() && !_arpTrack.midiKeyboard()) {
        _noteCount = 0;
        _notes.clear();
    }

    if (_noteCount == 0) {
        _currentStep = -1;
        _noteOrder = 0;
        return;
    }

    if (_notes.empty()) {
        return;
    }
    
    advanceStep();
    if (_stepIndex == 0) {
        advanceOctave();
    }

    if (_noteIndex >= int(_notes.size())) {
        _noteIndex = int(_notes.size())-1;
    }

    if (_skips != 0 && _stepIndex > 0 && !useFillGates) {
        --_skips;
        return;
    }

    if (_stepIndex == 0 || _stepIndex % 2 == 0) {
        int rest = evalRestProbability(sequence);
        if (rest != -1) {
            _skips = rest;
        }
    }

    int sequenceStepIndex = _notes.at(_noteIndex).index;
    _currentStep = sequenceStepIndex;

    const auto &step = evalSequence.step(sequenceStepIndex);

    int gateOffset = ((int) divisor * step.gateOffset()) / (ArpSequence::GateOffset::Max + 1);
    uint32_t stepTick = (int) tick + gateOffset;

    bool stepGate= false;
    if (_notes.at(_noteIndex).type == Type::MIDI) {
        stepGate = evalMIDIStepGate(step, _arpTrack.gateProbabilityBias()) || useFillGates;
    } else {
        stepGate = evalStepGate(step, _arpTrack.gateProbabilityBias()) || useFillGates;
    }

    //bool stepGate = evalStepGate(step, _arpTrack.gateProbabilityBias()) || useFillGates;
    if (stepGate) {
        stepGate = evalStepCondition(step, _iteration, useFillCondition, _prevCondition); //TODO check iteration
    }

    if (stepGate) {
        uint32_t stepLength = (divisor * evalStepLength(step, _arpTrack.lengthBias())) / ArpSequence::Length::Range;
        int rnd = 0;
        if (sequence.lengthModifier()!= 0) {
            int m = rng.nextRange(ArpSequence::NoteVariationProbability::Range-1);
            int mean = sequence.lengthModifier();
            std::mt19937 e2(m);
            std::normal_distribution<float> normal_dist(mean, 2);
            rnd = std::round(normal_dist(e2));
        }
        stepLength = stepLength + (rnd*2);
        int stepRetrigger = evalStepRetrigger(step, _arpTrack.retriggerProbabilityBias());
        if (stepRetrigger > 1) {
            uint32_t retriggerLength = divisor / stepRetrigger;
            uint32_t retriggerOffset = 0;
            while (stepRetrigger-- > 0 && retriggerOffset <= stepLength) {
                _gateQueue.pushReplace({ Groove::applySwing(stepTick + retriggerOffset, swing()), true });
                _gateQueue.pushReplace({ Groove::applySwing(stepTick + retriggerOffset + retriggerLength / 2, swing()), false });
                retriggerOffset += retriggerLength;
            }
        } else {
            _gateQueue.pushReplace({ Groove::applySwing(stepTick, swing()), true });
            _gateQueue.pushReplace({ Groove::applySwing(stepTick + stepLength, swing()), false });
        }
    }

    if (stepGate || _arpTrack.cvUpdateMode() == ArpTrack::CvUpdateMode::Always) {
        const auto &scale = evalSequence.selectedScale(_model.project().scale());
        int rootNote = evalSequence.selectedRootNote(_model.project().rootNote());
        _cvQueue.push({ Groove::applySwing(stepTick, swing()), evalStepNote(step, _arpTrack.noteProbabilityBias(), scale, rootNote, _octave+octave+_notes.at(_noteIndex).octave, transpose, sequence), step.slide() });
    }
}

void ArpTrackEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    triggerStep(tick, divisor, false);
}

void ArpTrackEngine::recordStep(uint32_t tick, uint32_t divisor) {

    if (!_engine.state().recording() || _model.project().recordMode() == Types::RecordMode::StepRecord) {
        return;
    }

    bool stepWritten = false;
    const auto &scale = activeSequence().selectedScale(_model.project().scale());
    const int notesPerOctave = clamp(scale.notesPerOctave(), 1, 128);
    const int pitchSlots = activePitchSlotCount(scale);

    auto writeStep = [this, divisor, &stepWritten, notesPerOctave, pitchSlots] (int note, int lengthTicks) {

        int stepNote = noteFromMidiNote(note);
        int octave = roundDownDivide(stepNote, notesPerOctave);
        
        int stepNoteCleared = wrappedPitchSlotIndex(stepNote, pitchSlots);
        auto &step = _sequence->step(stepNoteCleared);
        int length = (lengthTicks * ArpSequence::Length::Range) / divisor;
        step.setGate(true);
        step.setGateProbability(ArpSequence::GateProbability::Max);
        step.setRetrigger(0);
        step.setRetriggerProbability(ArpSequence::RetriggerProbability::Max);
        step.setLength(length);
        step.setLengthVariationRange(0);
        step.setLengthVariationProbability(ArpSequence::LengthVariationProbability::Max);
        step.setNote(stepNoteCleared);
        step.setNoteOctave(octave);
        step.setNoteOctaveProbability(ArpSequence::NoteOctaveProbability::Max);
        //step.setNoteVariationRange(0);
        step.setNoteVariationProbability(ArpSequence::NoteVariationProbability::Max);
        step.setCondition(Types::Condition::Off);

        addNote(stepNoteCleared, stepNoteCleared, Type::Sequencer, octave);
        stepWritten = true;
    };

    auto clearStep = [this] (int stepIndex) {
        auto &step = _sequence->step(stepIndex);

        step.clear();
    };

    for (size_t i = 0; i < _recordHistory.size(); ++i) {
        if (_recordHistory[i].type != RecordHistory::Type::NoteOn) {
            continue;
        }

        int note = _recordHistory[i].note;
        uint32_t noteStart = _recordHistory[i].tick;
        uint32_t noteEnd = i + 1 < _recordHistory.size() ? _recordHistory[i + 1].tick : tick;
        int length = noteEnd - noteStart;
        writeStep(note, length);
    }
    _recordHistory.clear();

    if (isSelected() && !stepWritten && _model.project().recordMode() == Types::RecordMode::Overwrite) {
        clearStep(_sequenceState.prevStep());
    }
}

int ArpTrackEngine::noteFromMidiNote(uint8_t midiNote) const {
    const auto &sequence = activeSequence();
    const auto &scale = sequence.selectedScale(_model.project().scale());
    int rootNote = sequence.selectedRootNote(_model.project().rootNote());
    float octaveVolts = scale.noteToVolts(scale.notesPerOctave()) - scale.noteToVolts(0);
    float semitoneVolts = octaveVolts * (1.f / 12.f);
    float volts = (int(midiNote) - 60) * semitoneVolts;

    if (scale.isChromatic()) {
        volts -= rootNote * semitoneVolts;
    } else {
        // Non-chromatic scales ignore root-note transposition here, matching the existing behavior.
    }

    return scale.noteFromVolts(volts);
}

void ArpTrackEngine::addNote(int note, int index, Type type, int octave) {
    _noteCount = _notes.size();
    // exit if note set is full
    if (_noteCount >= MaxNotes) {
        return;
    }

    for (int i= 0; i < int(_notes.size()); ++i) {
        if (_notes.at(i).note == note) {
            return;
        }
    }

    Note n;
    n.note = note;
    n.order = ++_noteOrder;
    n.index = index;
    n.octave = octave;
    n.type = type;

    _notes.insert(_notes.end(), n);

    std::sort(_notes.begin(), _notes.end(), _note);
    ++_noteCount;
}


void ArpTrackEngine::removeNote(int note) {
    int index = -1;
    _noteCount = _notes.size();
    for (int i = 0; i < _noteCount; ++i) {
        if (_arpeggiator.hold() && _arpTrack.midiKeyboard()) {
            return;
        }

        if (_notes.at(i).note == note) {
            index = i;
            --_noteCount;
            _notes.erase(_notes.begin() + index);
            break;
        }
    }

}

int ArpTrackEngine::noteIndexFromOrder(int order) {
    _noteCount = _notes.size();
    // search note index of note with given relative order
    for (int noteIndex = 0; noteIndex < _noteCount; ++noteIndex) {
        int currentOrder = 0;
        for (int i = 0; i < _noteCount; ++i) {
            if (_notes.at(i).order < _notes.at(noteIndex).order) {
                ++currentOrder;
            }
        }
        if (currentOrder == order) {
            return noteIndex;
        }
    }
    return 0;
}

void ArpTrackEngine::advanceStep() {
    _noteIndex = 0;
    _noteCount = _notes.size();
    const auto &sequence = activeSequence();
    uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
    uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
    uint32_t relativeTick = resetDivisor == 0 ? _engine.tick() : _engine.tick() % resetDivisor;
    int absoluteStep = int(relativeTick / divisor);

    auto mode = _arpeggiator.mode();

    switch (mode) {
    case Arpeggiator::Mode::PlayOrder:
        _stepIndex = (_stepIndex + 1) % _noteCount;
        _noteIndex = noteIndexFromOrder(_stepIndex);
        _iteration = relativeTick / _noteCount;
        break;
    case Arpeggiator::Mode::Up:
    case Arpeggiator::Mode::Down:
        _stepIndex = (_stepIndex + 1) % _noteCount;
        _noteIndex = _stepIndex;
        _iteration = absoluteStep / _noteCount;
        break;
    case Arpeggiator::Mode::UpDown:
    case Arpeggiator::Mode::DownUp:
        if (_noteCount >= 2) {
            _stepIndex = (_stepIndex + 1) % ((_noteCount - 1) * 2);
            _noteIndex = _stepIndex % (_noteCount - 1);
            _noteIndex = _stepIndex < _noteCount - 1 ? _noteIndex : _noteCount - _noteIndex - 1;
            _iteration = absoluteStep / (2 * _noteCount);
        } else {
            _stepIndex = 0;
        }
        break;
    case Arpeggiator::Mode::UpAndDown:
    case Arpeggiator::Mode::DownAndUp:
        _stepIndex = (_stepIndex + 1) % (_noteCount * 2);
        _noteIndex = _stepIndex % _noteCount;
        _noteIndex = _stepIndex < _noteCount ? _noteIndex : _noteCount - _noteIndex - 1;
        _iteration = absoluteStep / (2 * _noteCount - 2);
        break;
    case Arpeggiator::Mode::Converge:
        _stepIndex = (_stepIndex + 1) % _noteCount;
        _noteIndex = _stepIndex / 2;
        if (_stepIndex % 2 == 1) {
            _noteIndex = _noteCount - _noteIndex - 1;
        }
        break;
    case Arpeggiator::Mode::Diverge:
        _stepIndex = (_stepIndex + 1) % _noteCount;
        _noteIndex = _stepIndex / 2;
        _noteIndex = _noteCount / 2 + ((_stepIndex % 2 == 0) ? _noteIndex : - _noteIndex - 1);
        break;
    case Arpeggiator::Mode::Random:
        _stepIndex = (_stepIndex + 1) % _noteCount;
        _noteIndex = rng.nextRange(_noteCount);
        break;
    case Arpeggiator::Mode::Last:
        break;
    }

    switch (mode) {
    case Arpeggiator::Mode::Down:
    case Arpeggiator::Mode::DownUp:
    case Arpeggiator::Mode::DownAndUp:
        _noteIndex = _noteCount - _noteIndex - 1;
        break;
    default:
        break;
    }
}

void ArpTrackEngine::advanceOctave() {
    int octaves = _arpeggiator.octaves();

    bool bothDirections = false;
    if (octaves > 5) {
        octaves -= 5;
        bothDirections = true;
    }
    if (octaves < -5) {
        octaves += 5;
        bothDirections = true;
    }

    if (octaves == 0) {
        _octave = 0;
        _octaveDirection = 0;
    } else if (octaves > 0) {
        if (_octaveDirection == 0) {
            _octaveDirection = 1;
        } else {
            _octave += _octaveDirection;
            if (_octave > octaves) {
                _octave = bothDirections ? octaves : 0;
                _octaveDirection = bothDirections ? -1 : 1;
            } else if (_octave < 0) {
                _octave = 0;
                _octaveDirection = 1;
            }
        }
    } else if (octaves < 0) {
        if (_octaveDirection == 0) {
            _octaveDirection = -1;
        } else {
            _octave += _octaveDirection;
            if (_octave < octaves) {
                _octave = bothDirections ? octaves : 0;
                _octaveDirection = bothDirections ? 1 : -1;
            } else if (_octave > 0) {
                _octave = 0;
                _octaveDirection = -1;
            }
        }
    }
}
