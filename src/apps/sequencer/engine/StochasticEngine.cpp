#include "StochasticEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "SequenceState.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/StochasticSequence.h"
#include "model/Scale.h"
#include "ui/MatrixMap.h"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <random>
#include <ratio>
#include <vector>

static Random rng;

bool sortTaskByProbRev(const StochasticStep& lhs, const StochasticStep& rhs) {
    return lhs.probability() > rhs.probability();
}

// evaluate if step gate is active
static bool evalStepGate(const StochasticSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.gateProbability() + probabilityBias, -1, StochasticSequence::GateProbability::Max);
    if (probability==0) {
        return false;
    }
    return step.gate() && int(rng.nextRange(StochasticSequence::GateProbability::Range)) <= probability;
}

// evaluate if step gate is active
 int StochasticEngine::evalRestProbability(StochasticSequence &sequence) {
    int sum = 0;
    std::vector<StochasticStep> probability;
    for (int i = 0; i < 4; i++) {

        switch (i) {
            case 0: {
                probability.insert(probability.end(), StochasticStep(i, sequence.restProbability()));
            }
                break;
            case 1: {
                probability.insert(probability.end(), StochasticStep(i, sequence.restProbability2()));
                break;
            }
            case 2: {
                probability.insert(probability.end(), StochasticStep(i, sequence.restProbability4()));
                break;
            }
            case 3: {
                probability.insert(probability.end(), StochasticStep(i, sequence.restProbability8()));
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
static bool evalStepCondition(const StochasticSequence::Step &step, int iteration, bool fill, bool &prevCondition) {
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
static int evalStepRetrigger(const StochasticSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.retriggerProbability() + probabilityBias, -1, StochasticSequence::RetriggerProbability::Max);
    return int(rng.nextRange(StochasticSequence::RetriggerProbability::Range)) <= probability ? step.retrigger() + 1 : 1;
}

// evaluate step length
static int evalStepLength(const StochasticSequence::Step &step, int lengthBias) {
    int length = StochasticSequence::Length::clamp(step.length() + lengthBias) + 1;
    int probability = step.lengthVariationProbability();
    if (int(rng.nextRange(StochasticSequence::LengthVariationProbability::Range)) <= probability) {
        int offset = step.lengthVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.lengthVariationRange()) + 1);
        if (step.lengthVariationRange() < 0) {
            offset = -offset;
        }
        length = clamp(length + offset, 0, StochasticSequence::Length::Range);
    }
    return length;
}

// evaluate transposition
static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

// evaluate note voltage
static float evalStepNote(const StochasticSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, StochasticSequence sequence, bool useVariation = true) {
    if (step.bypassScale()) {
        const Scale &bypassScale = Scale::get(0);
        int note = step.note() + evalTransposition(bypassScale, octave, transpose);
        int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, StochasticSequence::NoteOctaveProbability::Max);
        if (step.noteOctaveProbability()==0) {
            probability = 0;
        }
        if (useVariation && int(rng.nextRange(StochasticSequence::NoteOctaveProbability::Range)) <= probability && probability!= 0) {
            int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
            note = StochasticSequence::Note::clamp(note + (bypassScale.notesPerOctave()*oct));
        }
        return bypassScale.noteToVolts(note) + (bypassScale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
    }
    int note = step.note() + evalTransposition(scale, octave, transpose);
    int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, StochasticSequence::NoteOctaveProbability::Max);
    if (useVariation && int(rng.nextRange(StochasticSequence::NoteOctaveProbability::Range)) <= probability && probability != 0) {
        int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
        note = StochasticSequence::Note::clamp(note + (scale.notesPerOctave()*oct));
    }
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}



void StochasticEngine::reset() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    _prevCondition = false;
    _activity = false;
    _gateOutput = false;
    _slideActive = false;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
        _cvOutputTarget = 0.f;
    }
    _gateQueue.clear();
    _cvQueue.clear();
    _recordHistory.clear();
    rng = Random(time(NULL)); 
    changePattern();
}

void StochasticEngine::restart() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    rng = Random(time(NULL));    
}

TrackEngine::TickResult StochasticEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            recordStep(tick, linkData->divisor);
            triggerStep(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
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
        const auto &sequence = *_sequence;

        // advance sequence
        
        switch (_stochasticTrack.playMode()) {
        case Types::PlayMode::Aligned: {
            if (relativeTick % divisor == 0) {
               
                if (sequence.useLoop()) {
                    _sequenceState.calculateNextStepAligned(
                            relativeTick / divisor,
                            sequence.runMode(),
                            sequence.sequenceFirstStep(),
                            sequence.sequenceLastStep(),
                            rng
                    );
                    triggerStep(tick, divisor, true);
                } else {
                    _sequenceState.advanceAligned(relativeTick / divisor, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                     triggerStep(tick, divisor);
                }
            }
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
        if (!mute() || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {
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

void StochasticEngine::update(float dt) {
    bool running = _engine.state().running();
    const auto &sequence = *_sequence;
    const auto &scale = sequence.selectedScale(_model.project().scale());
    int rootNote = sequence.selectedRootNote(_model.project().rootNote());
    int octave = _stochasticTrack.octave();
    int transpose = _stochasticTrack.transpose();

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
        setOverride(evalStepNote(step, 0, scale, rootNote, octave, transpose, sequence, false));
    } else if (liveMonitoring && _recordHistory.isNoteActive()) {
        int note = noteFromMidiNote(_recordHistory.activeNote()) + evalTransposition(scale, octave, transpose);
        setOverride(scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f));
    } else {
        clearOverride();
    }

    if (_slideActive && _stochasticTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, _stochasticTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void StochasticEngine::changePattern() {
    _sequence = &_stochasticTrack.sequence(pattern());
    _fillSequence = &_stochasticTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void StochasticEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);
}

void StochasticEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void StochasticEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;

    // in step record mode, select step to start recording recording from
    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord &&
        index >= _sequence->firstStep() && index <= _sequence->lastStep()) {
        _stepRecorder.setStepIndex(index);
    }
}

bool canLoop = false;

void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor, bool forNextStep) {
    int octave = _stochasticTrack.octave();
    int transpose = _stochasticTrack.transpose();

    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::NextPattern;
    bool useFillCondition = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::Condition;

    auto &sequence = *_sequence;
    const auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;
    
    int stepIndex;

    uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
    uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;
    auto abstoluteStep = relativeTick / divisor;
    
    _index = abstoluteStep % sequence.sequenceLength();

    StochasticSequence::Step step;
    uint32_t stepTick = 0;
    bool stepGate = false;
    float noteValue = 0;
    uint32_t stepLength = 0;
    int stepRetrigger = 0;

    if (!sequence.useLoop() && sequence.reseed() && !sequence.isEmpty()) {
        int rnd = -StochasticSequence::NoteVariationProbability::Range/2 + ( std::rand() % ( (StochasticSequence::NoteVariationProbability::Range/2) - (-StochasticSequence::NoteVariationProbability::Range/2)) + 1 );
        _stochasticTrack.setNoteProbabilityBias(rnd);
        rng = Random(time(NULL));
        sequence.setReseed(0, false);
    }

        // clear the locked memory sequence and reset it to the in memory sequence
    if (sequence.clearLoop()) {
        
        if (_index == 0) {
            _lockedSteps.clear();
            sequence.setClearLoop(false);
            sequence.setUseLoop(false);
            _sequenceState.reset();
            _index = -1;
            int rest = evalRestProbability(sequence);
            if (rest != -1) {
                _skips = rest;
            }
        }
    }

    // fill in memory step when sequence is running or when the in memory loop is not full filled
    if (!sequence.useLoop() || int(_lockedSteps.size()) < sequence.bufferLoopLength()) { 
        if (_skips != 0 && _index > 0 && !useFillGates) {
            --_skips;
            if (int(_lockedSteps.size()) < sequence.bufferLoopLength()) {
                _lockedSteps.insert(_lockedSteps.end(), StochasticLoopStep(-1, false, step, 0, 0, 0));
            }
            return;
        }
        if (_index == 0 || _index % 2 == 0) {
            int rest = evalRestProbability(sequence);
            if (rest != -1) {
                _skips = rest;
            }
        }

        std::vector<StochasticStep> probability;
        int sum =0;
        for (int i = 0; i < 12; i++) {
            if (sequence.step(i).gate()) {
                int prob = sequence.step(i).noteVariationProbability() + _stochasticTrack.noteProbabilityBias();
                if (sequence.step(i).noteVariationProbability()==0) {
                    prob = 0;
                }
                probability.insert(probability.end(), StochasticStep(i, clamp(prob, -1, StochasticSequence::NoteVariationProbability::Max)));
            } else {
                probability.insert(probability.end(), StochasticStep(i, 0));
            }
            sum = sum + probability.at(i).probability();
        }
        if (sum==0) { return;}
        std::sort (std::begin(probability), std::end(probability), sortTaskByProbRev);
        stepIndex = getNextWeightedPitch(probability, probability.size());

        step = evalSequence.step(stepIndex); 
        _currentStep = stepIndex;   
        
        int gateOffset = ((int) divisor * step.gateOffset()) / (StochasticSequence::GateOffset::Max + 1);
        stepTick = (int) tick + gateOffset;

        stepGate = evalStepGate(step, _stochasticTrack.gateProbabilityBias()) || useFillGates;
        if (stepGate) {
            stepGate = evalStepCondition(step, _sequenceState.iteration(), useFillCondition, _prevCondition);
        }
        const auto &scale = sequence.selectedScale(_model.project().scale());
        int rootNote = sequence.selectedRootNote(_model.project().rootNote());
        noteValue = evalStepNote(step, _stochasticTrack.noteProbabilityBias(), scale, rootNote, octave, transpose, sequence);
        stepLength = (divisor * evalStepLength(step, _stochasticTrack.lengthBias())) / StochasticSequence::Length::Range;

        int rnd = 0;
        if (sequence.lengthModifier()!= 0) {
            int m = rng.nextRange(StochasticSequence::NoteVariationProbability::Range-1);
            int mean = sequence.lengthModifier();
            std::mt19937 e2(m);
            std::normal_distribution<float> normal_dist(mean, 2);
            rnd = std::round(normal_dist(e2));
        }
        stepLength = stepLength + (rnd*2);
        stepRetrigger = evalStepRetrigger(step, _stochasticTrack.retriggerProbabilityBias());
        
        if (int(_lockedSteps.size()) < sequence.bufferLoopLength()) {
            _lockedSteps.insert(_lockedSteps.end(), StochasticLoopStep(stepIndex, stepGate, step, noteValue, stepLength, stepRetrigger));
        }

        if (stepGate) {
            sequence.setStepBounds(stepIndex);
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

        if (stepGate || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {

            _cvQueue.push({ Groove::applySwing(stepTick, swing()), noteValue, step.slide() });
        }
        return;

    }

    // use the locked loop to retrieve steps data
    if (sequence.useLoop() && int(_lockedSteps.size()) >= sequence.bufferLoopLength()) {
        if (forNextStep) {
            if (sequence.runMode() == Types::RunMode::RandomWalk) {
                if (rng.nextRange(2) == 0) {
                    _sequenceState.setStep(-1);
                } else {
                    _sequenceState.setStep(_index);
                }
            }
            _index = _sequenceState.nextStep();
        }

        _lockedSteps = slicing(_lockedSteps, 0, sequence.bufferLoopLength()-1);
       
        auto subArray = slicing(_lockedSteps, sequence.sequenceFirstStep(), sequence.sequenceLastStep());
        _index = _index%sequence.sequenceLength();
        stepIndex = subArray.at(_index).index();
        if (stepIndex == -1) {
            return;
        }
        _currentStep = stepIndex;

        stepGate = subArray.at(_index).gate();
        step = subArray.at(_index).step();

        int gateOffset = ((int) divisor * step.gateOffset()) / (StochasticSequence::GateOffset::Max + 1);
        stepTick = (int) tick + gateOffset;
        noteValue = subArray.at(_index).noteValue();
        stepLength = subArray.at(_index).stepLength();
        stepRetrigger = _lockedSteps.at(_index).stepRetrigger();
    
    

    if (stepGate) {
        sequence.setStepBounds(stepIndex);
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

    if (stepGate || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {

        _cvQueue.push({ Groove::applySwing(stepTick, swing()), noteValue, step.slide() });
    }
    }
}


int StochasticEngine::getNextWeightedPitch(std::vector<StochasticStep> distr, int notesPerOctave) {
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



void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    triggerStep(tick, divisor, false);
}

void StochasticEngine::recordStep(uint32_t tick, uint32_t divisor) {
    if (!_engine.state().recording() || _model.project().recordMode() == Types::RecordMode::StepRecord || _sequenceState.prevStep() < 0) {
        return;
    }

    bool stepWritten = false;

    auto writeStep = [this, divisor, &stepWritten] (int stepIndex, int note, int lengthTicks) {
        auto &step = _sequence->step(stepIndex);
        int length = (lengthTicks * StochasticSequence::Length::Range) / divisor;

        step.setGate(true);
        step.setGateProbability(StochasticSequence::GateProbability::Max);
        step.setRetrigger(0);
        step.setRetriggerProbability(StochasticSequence::RetriggerProbability::Max);
        step.setLength(length);
        step.setLengthVariationRange(0);
        step.setLengthVariationProbability(StochasticSequence::LengthVariationProbability::Max);
        step.setNote(noteFromMidiNote(note));
        step.setNoteOctave(0);
        step.setNoteVariationProbability(StochasticSequence::NoteVariationProbability::Max);
        step.setCondition(Types::Condition::Off);
        step.setStageRepeats(1);

        stepWritten = true;
    };

    auto clearStep = [this] (int stepIndex) {
        auto &step = _sequence->step(stepIndex);

        step.clear();
    };

    uint32_t stepStart = tick - divisor;
    uint32_t stepEnd = tick;
    uint32_t margin = divisor / 2;

    for (size_t i = 0; i < _recordHistory.size(); ++i) {
        if (_recordHistory[i].type != RecordHistory::Type::NoteOn) {
            continue;
        }

        int note = _recordHistory[i].note;
        uint32_t noteStart = _recordHistory[i].tick;
        uint32_t noteEnd = i + 1 < _recordHistory.size() ? _recordHistory[i + 1].tick : tick;

        if (noteStart >= stepStart - margin && noteStart < stepStart + margin) {
            // note on during step start phase
            if (noteEnd >= stepEnd) {
                // note hold during step
                int length = std::min(noteEnd, stepEnd) - stepStart;
                writeStep(_sequenceState.prevStep(), note, length);
            } else {
                // note released during step
                int length = noteEnd - noteStart;
                writeStep(_sequenceState.prevStep(), note, length);
            }
        } else if (noteStart < stepStart && noteEnd > stepStart) {
            // note on during previous step
            int length = std::min(noteEnd, stepEnd) - stepStart;
            writeStep(_sequenceState.prevStep(), note, length);
        }
    }

    if (isSelected() && !stepWritten && _model.project().recordMode() == Types::RecordMode::Overwrite) {
        clearStep(_sequenceState.prevStep());
    }
}

int StochasticEngine::noteFromMidiNote(uint8_t midiNote) const {
    const auto &scale = _sequence->selectedScale(_model.project().scale());
    int rootNote = _sequence->selectedRootNote(_model.project().rootNote());
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
