#include "NoteSequenceEditPage.h"

#include "LayoutPage.h"
#include "Pages.h"

#include "model/NoteSequence.h"
#include "model/FileManager.h"
#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"
#include "ui/StepSelectionUtils.h"

#include "engine/generators/AcidGenerator.h"
#include "engine/generators/ChaosGenerator.h"
#include "engine/generators/Generator.h"

#include "model/Scale.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"
#include <bitset>

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,    Generate,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT LAYER" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "GEN" },
};

enum class Function {
    Gate        = 0,
    Retrigger   = 1,
    Length      = 2,
    Note        = 3,
    Condition     = 4,
};

static const char *functionNames[] = { "GATE", "RETRIG", "LENGTH", "NOTE", "COND" };


static const NoteSequenceListModel::Item quickEditItems[8] = {
    NoteSequenceListModel::Item::FirstStep,
    NoteSequenceListModel::Item::LastStep,
    NoteSequenceListModel::Item::RunMode,
    NoteSequenceListModel::Item::Divisor,
    NoteSequenceListModel::Item::ResetMeasure,
    NoteSequenceListModel::Item::Scale,
    NoteSequenceListModel::Item::RootNote,
    NoteSequenceListModel::Item::Last
};

NoteSequenceEditPage::NoteSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedNoteSequenceLayer();
        const auto &sequence = _project.selectedNoteSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void NoteSequenceEditPage::enter() {
    updateMonitorStep();

    _inMemorySequence = _project.selectedNoteSequence();

    _showDetail = false;

    if (_project.selectedTrack().noteTrack().playMode() == Types::PlayMode::Aligned) {
            if (_project.selectedNoteSequenceLayer() == NoteSequence::Layer::StageRepeats || _project.selectedNoteSequenceLayer() == NoteSequence::Layer::StageRepeatsMode ) {
                _project.setSelectedNoteSequenceLayer(NoteSequence::Layer::Retrigger); 
            }
        } else {
            if (_project.selectedNoteSequenceLayer() == NoteSequence::Layer::Retrigger) {
                _project.setSelectedNoteSequenceLayer(NoteSequence::Layer::StageRepeats);
            }
        }
    }

void NoteSequenceEditPage::exit() {
    destroyActiveBuilder();
    _engine.selectedTrackEngine().as<NoteTrackEngine>().setMonitorStep(-1);
}

void NoteSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    auto &track = _project.selectedTrack().noteTrack();

    /* Prepare flags shown before mode name (top right header) */
    const auto pattern_follow = track.patternFollow();
    const char* pf_repr = Types::patternFollowShortRepresentation(pattern_follow);

    const char *headerMode = _model.knobPad16Armed() ? "16STEP EDIT" : "STEPS";
    WindowPainter::drawHeader(canvas, _model, _engine, headerMode, pf_repr);

    WindowPainter::drawActiveFunction(canvas, NoteSequence::layerName(layer()));
    if (_launchpadGeneratorModeActive) {
        WindowPainter::drawFooter(canvas);
        drawLaunchpadGeneratorOverlay(canvas);
        return;
    }

    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    auto &sequence = _project.selectedNoteSequence();
    const auto &scale = sequence.selectedScale(_project.scale());
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    if (trackEngine.currentRecordStep()!=-1) {
        trackEngine.setCurrentRecordStep(sequence.currentRecordStep());
    }
    int currentRecordStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentRecordStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();

    const int loopY = 16;

    // Track Pattern Section on the UI
    if (track.isPatternFollowDisplayOn() && _engine.state().running()) {
        bool section_change = bool((currentStep) % StepCount == 0); // StepCount is relative to screen
        int section_no = int((currentStep) / StepCount);
        if (section_change && section_no != sequence.section()) {
            sequence.setSecion(section_no);
        }
    }

    // draw loop points
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    SequencePainter::drawLoopStart(canvas, (sequence.firstStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);
    SequencePainter::drawLoopEnd(canvas, (sequence.lastStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);

    for (int i = 0; i < StepCount; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = i * stepWidth;
        int y = 20;

        // step index
        SequencePainter::drawStepIndex(canvas, x, y, stepWidth, stepIndex + 1, _stepSelection[stepIndex], step.condition() != Types::Condition::Off);

        // step gate
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(SequencePainter::dimSequenceColor(
                _context.model.settings().userSettings().get<DimSequenceSetting>(SettingDimSequence)->getValue()
            ));
            SequencePainter::drawGateBody(canvas, x, y, stepWidth, step.gateOffset(), NoteSequence::GateOffset::Max, step.length(), NoteSequence::Length::Range, step.retrigger(), NoteSequence::Retrigger::Range, step.slide());
        }

        // record step
        if (stepIndex == currentRecordStep) {
            // draw circle
            canvas.setColor(step.gate() ? Color::None : Color::Bright);
            canvas.fillRect(x + 6, y + 6, stepWidth - 12, stepWidth - 12);
            canvas.setColor(Color::Medium);
            canvas.hline(x + 7, y + 5, 2);
            canvas.hline(x + 7, y + 10, 2);
            canvas.vline(x + 5, y + 7, 2);
            canvas.vline(x + 10, y + 7, 2);
        }

        switch (layer()) {
        case Layer::Gate:
            break;
        case Layer::GateProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateProbability() + 1, NoteSequence::GateProbability::Range
            );
            break;
        case Layer::GateOffset:
            SequencePainter::drawOffset(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateOffset(), NoteSequence::GateOffset::Min - 1, NoteSequence::GateOffset::Max + 1
            );
            break;
        case Layer::Retrigger:
            SequencePainter::drawRetrigger(
                canvas,
                x, y + 18, stepWidth, 2,
                step.retrigger() + 1, NoteSequence::Retrigger::Range
            );
            break;
        case Layer::RetriggerProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.retriggerProbability() + 1, NoteSequence::RetriggerProbability::Range
            );
            break;
        case Layer::Length:
            SequencePainter::drawLength(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, NoteSequence::Length::Range
            );
            break;
        case Layer::LengthVariationRange:
            SequencePainter::drawLengthRange(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, step.lengthVariationRange(), NoteSequence::Length::Range
            );
            break;
        case Layer::LengthVariationProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.lengthVariationProbability() + 1, NoteSequence::LengthVariationProbability::Range
            );
            break;
        case Layer::Note: {
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;

            if (step.bypassScale()) {
                const Scale &bypassScale = std::ref(Scale::get(0));
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
            
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
                str.reset();
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short2);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                break;
            } 
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            scale.noteName(str, step.note(), rootNote, Scale::Short2);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::NoteVariationRange: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str("%d", step.noteVariationRange());
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::NoteVariationProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.noteVariationProbability() + 1, NoteSequence::NoteVariationProbability::Range
            );
            break;
        case Layer::Slide:
            SequencePainter::drawSlide(
                canvas,
                x + 4, y + 18, stepWidth - 8, 4,
                step.slide()
            );
            break;
        case Layer::BypassScale:
            SequencePainter::drawBypassScale(
                canvas,
                x + 4, y + 18, stepWidth - 8, 4,
                step.bypassScale()
            );
            break;
        case Layer::Condition: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            Types::printCondition(str, step.condition(), Types::ConditionFormat::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            Types::printCondition(str, step.condition(), Types::ConditionFormat::Short2);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::StageRepeats: {
            canvas.setColor(Bright);
            FixedStringBuilder<8> str("x%d", step.stageRepeats()+1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::StageRepeatsMode: {
            SequencePainter::drawStageRepeatMode(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.stageRepeatMode()
            );
            break;
        }
        case Layer::Last:
            break;
        }
    }

    // handle detail display

    if (_showDetail) {
        if (layer() == Layer::Gate || layer() == Layer::Slide || _stepSelection.none() || layer() == Layer::BypassScale) {
            _showDetail = false;
        }
        if (_stepSelection.isPersisted() && os::ticks() > _showDetailTicks + os::time::ms(500)) {
            _showDetail = false;
        }
    }

    if (_showDetail) {
        drawDetail(canvas, sequence.step(_stepSelection.first()));
    }



}

void NoteSequenceEditPage::drawLaunchpadGeneratorOverlay(Canvas &canvas) {
    static const char *overlayCells[2][6] = {
        { "RAND", "ACIDL", "VNDLZ", "EUCL", nullptr, "INITL" },
        { nullptr, "ACIDP", "WRECK", nullptr, nullptr, "INITS" },
    };

    constexpr int columns = 6;
    constexpr int rows = 2;
    constexpr int cellWidth = 41;
    constexpr int cellHeight = 15;
    constexpr int gridX = 0;
    constexpr int gridY = 18;

    canvas.setBlendMode(BlendMode::Set);
    canvas.setFont(Font::Tiny);

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            int x = gridX + col * (cellWidth + 2);
            int y = gridY + row * (cellHeight + 2);
            const char *label = overlayCells[row][col];

            canvas.setColor(label ? Color::Medium : Color::Low);
            canvas.drawRect(x, y, cellWidth, cellHeight);

            if (label) {
                canvas.setColor(Color::Bright);
                canvas.drawTextCentered(x, y + 4, cellWidth, 8, label);
            }
        }
    }
}

void NoteSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
    auto &sequence = _project.selectedNoteSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset() + i;
        bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
        bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, sequence.section());

    // show quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != NoteSequenceListModel::Item::Last);
            leds.mask(index);
        }
        int index = MatrixMap::fromStep(15);
        leds.unmask(index);
        leds.set(index, false, true);
        leds.mask(index);
    }
}

void NoteSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
    updateMonitorStep();
}

void NoteSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
    updateMonitorStep();
}

void NoteSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedNoteSequence();
    auto &track = _project.selectedTrack().noteTrack();

    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }
    if (key.pageModifier() && event.count() == 2) {
        contextShow(true);
        event.consume();
        return;
    }

    if (key.pageModifier() && key.is(Key::Step6)) {
        launchpadUndo();
        event.consume();
        return;
    }

    if (key.isQuickEdit()) {
         if (key.is(Key::Step15)) {
            bool lpConnected = _engine.isLaunchpadConnected();

             track.togglePatternFollowDisplay(lpConnected);
        } else {
            _inMemorySequence = _project.selectedNoteSequence();
            quickEdit(key.quickEdit());
        }
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }


    if (key.isFunction()) {
        int v = 0;
        switch (key.code()) {
            case Key::F0:
                v=1;
                break;
            case Key::F1:
                v=2;
                break;
            case Key::F2:
                v=3;
                break;
            case Key::F3:
                v=4;
                break;
            case Key::F4:
                v=5;
                break;
        }
        for (int i=0; i<16; ++i) {
           if (key.state(i)) {
                const auto &scale = sequence.selectedScale(_project.scale());
                int stepIndex = 0;
                if (i>=8) {
                    stepIndex = i -8;
                } else {
                    stepIndex = i+8;
                }
                sequence.step(stepIndex).setNote(scale.notesPerOctave()*v);
                event.consume();
                return;
                
           }
        }
        
    }
    _stepSelection.keyPress(event, stepOffset());
    updateMonitorStep();

    if (!key.shiftModifier() && key.isStep()) {
        int stepIndex = stepOffset() + key.step();
        switch (layer()) {
        case Layer::Gate:
            _inMemorySequence = _project.selectedNoteSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
            break;
        default:
            break;
        }
    }

    KeyPressEvent keyPressEvent =_keyPressEventTracker.process(key);

    if (!key.shiftModifier() && key.isStep() && keyPressEvent.count() == 2) {
        int stepIndex = stepOffset() + key.step();
        if (layer() != Layer::Gate) {
            _inMemorySequence = _project.selectedNoteSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
        }
    }

    if (key.isFunction()) {
        if(key.shiftModifier() && key.function() == 2 && _stepSelection.any()) {
            _inMemorySequence = _project.selectedNoteSequence();
            tieNotes();
            event.consume();
            return;
        }
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
    }

    if (key.isEncoder()) {
        track.setPatternFollowDisplay(false);
        _inMemorySequence = _project.selectedNoteSequence();
        if (!_showDetail && _stepSelection.any() && allSelectedStepsActive()) {
            setSelectedStepsGate(false);
        } else {
            setSelectedStepsGate(true);
        }
        event.consume();
    }


    if (key.isLeft()) {
        if (key.shiftModifier()) {
            if (trackEngine.currentRecordStep()!=-1) {
                if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()-1, true);
                } else {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()-1, false);
                }
            } else {
                _inMemorySequence = _project.selectedNoteSequence();
                sequence.shiftSteps(_stepSelection.selected(), -1);
                _stepSelection.shiftLeft(sequence.firstStep(), sequence.lastStep()+1);
            }
        } else {
             track.setPatternFollowDisplay(false);
             int sectionCount = sequence.lastStep() / StepCount + 1;
             sequence.setSecion((sequence.section() + sectionCount - 1) % sectionCount);
        }
        event.consume();
    }
    if (key.isRight()) {
        if (key.shiftModifier()) {
            if (trackEngine.currentRecordStep()!=-1) {
                if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()+1, true);
                } else {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()+1, false);
                }
                
            } else {
                _inMemorySequence = _project.selectedNoteSequence();
                sequence.shiftSteps(_stepSelection.selected(), 1);
                _stepSelection.shiftRight(sequence.firstStep(), sequence.lastStep()+1);
            }
        } else {
            track.setPatternFollowDisplay(false);
            int sectionCount = sequence.lastStep() / StepCount + 1;
            sequence.setSecion((sequence.section() + 1) % sectionCount);
        }
        event.consume();
    }
}

void NoteSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedNoteSequence();
    const auto &scale = sequence.selectedScale(_project.scale());

    if (!_stepSelection.any())
    {
        switch (layer())
        {
        case Layer::Gate:
            setLayer(event.value() > 0 ? Layer::GateOffset : Layer::GateProbability);
            break;
        case Layer::GateOffset:
            setLayer(event.value() > 0 ? Layer::GateProbability : Layer::Gate);
            break;
        case Layer::GateProbability:
            setLayer(event.value() > 0 ? Layer::Gate : Layer::GateOffset);
            break;
        case Layer::Retrigger:
            setLayer(event.value() > 0 ? Layer::RetriggerProbability : Layer::StageRepeatsMode);
            break;
        case Layer::RetriggerProbability:
            setLayer(event.value() > 0 ? Layer::StageRepeats : Layer::Retrigger);
            break;
        case Layer::StageRepeats:
            setLayer(event.value() > 0 ? Layer::StageRepeatsMode : Layer::RetriggerProbability);
            break;
        case Layer::StageRepeatsMode:
            setLayer(event.value() > 0 ? Layer::Retrigger : Layer::StageRepeats);
            break;
        case Layer::Length:
            setLayer(event.value() > 0 ? Layer::LengthVariationRange : Layer::LengthVariationProbability);
            break;
        case Layer::LengthVariationRange:
            setLayer(event.value() > 0 ? Layer::LengthVariationProbability : Layer::Length);
            break;
        case Layer::LengthVariationProbability:
            setLayer(event.value() > 0 ? Layer::Length : Layer::LengthVariationRange);
            break;
        case Layer::Note:
            setLayer(event.value() > 0 ? Layer::Slide : Layer::BypassScale);
            break;
        case Layer::Slide:
            setLayer(event.value() > 0 ? Layer::NoteVariationRange : Layer::Note);
            break;
        case Layer::NoteVariationRange:
            setLayer(event.value() > 0 ? Layer::NoteVariationProbability : Layer::Slide);
            break;
        case Layer::NoteVariationProbability:
            setLayer(event.value() > 0 ? Layer::BypassScale : Layer::NoteVariationRange);
            break;
        case Layer::BypassScale:
            setLayer(event.value() > 0 ? Layer::Note : Layer::NoteVariationProbability);
        default:
            break;
        }
        return;
    }
    else
    {
        _showDetail = true;
        _showDetailTicks = os::ticks();
    }

    for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            auto &step = sequence.step(stepIndex);
            bool shift = globalKeyState()[Key::Shift];
            switch (layer()) {
            case Layer::Gate:
                step.setGate(event.value() > 0);
                break;
            case Layer::GateProbability:
                step.setGateProbability(step.gateProbability() + event.value());
                break;
            case Layer::GateOffset:
                step.setGateOffset(step.gateOffset() + event.value());
                break;
            case Layer::Retrigger:
                step.setRetrigger(step.retrigger() + event.value());
                break;
            case Layer::RetriggerProbability:
                step.setRetriggerProbability(step.retriggerProbability() + event.value());
                break;
            case Layer::Length:
                step.setLength(step.length() + event.value());
                break;
            case Layer::LengthVariationRange:
                step.setLengthVariationRange(step.lengthVariationRange() + event.value());
                break;
            case Layer::LengthVariationProbability:
                step.setLengthVariationProbability(step.lengthVariationProbability() + event.value());
                break;
            case Layer::Note:
                step.setNote(step.note() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                updateMonitorStep();
                break;
            case Layer::NoteVariationRange:
                step.setNoteVariationRange(step.noteVariationRange() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                updateMonitorStep();
                break;
            case Layer::NoteVariationProbability:
                step.setNoteVariationProbability(step.noteVariationProbability() + event.value());
                break;
            case Layer::Slide:
                step.setSlide(event.value() > 0);
                break;
            case Layer::BypassScale:
                step.setBypassScale(event.value() > 0);
                break;
            case Layer::Condition:
                step.setCondition(ModelUtils::adjustedEnum(step.condition(), event.value()));
                break;
            case Layer::StageRepeats:
                step.setStageRepeats(step.stageRepeats() + event.value());
                break;
            case Layer::StageRepeatsMode:
                step.setStageRepeatsMode(
                    static_cast<Types::StageRepeatMode>(
                        step.stageRepeatMode() + event.value()
                    )
                );
                break;
            case Layer::Last:
                break;
            }
        }
    }

    event.consume();
}

void NoteSequenceEditPage::midi(MidiEvent &event) {
    if (!_engine.recording() && layer() == Layer::Note && _stepSelection.any()) {
        if (_project.clockSetup().filterNote()) {
            return;
        }
        auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
        auto &sequence = _project.selectedNoteSequence();
        const auto &scale = sequence.selectedScale(_project.scale());
        const auto &message = event.message();

        if (message.isNoteOn()) {
            float volts = (message.note() - 60) * (1.f / 12.f);
            int note = scale.noteFromVolts(volts);

            for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
                if (_stepSelection[stepIndex]) {
                    auto &step = sequence.step(stepIndex);
                    step.setNote(note);
                    step.setGate(true);
                }
            }

            trackEngine.setMonitorStep(_stepSelection.first());
            updateMonitorStep();
        }
    }
}

void NoteSequenceEditPage::switchLayer(int functionKey, bool shift) {

    auto engine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    if (shift) {
        switch (Function(functionKey)) {
        case Function::Gate:
            setLayer(Layer::Gate);
            break;
        case Function::Retrigger:
            if (engine.playMode() == Types::PlayMode::Free) {
                setLayer(Layer::StageRepeats);
            }
            break;
        case Function::Length:
            if (engine.playMode() == Types::PlayMode::Free) {
                setLayer(Layer::StageRepeatsMode);
            }
            break;
        case Function::Note:
            setLayer(Layer::Slide);
            break;
        case Function::Condition:
            setLayer(Layer::Condition);
            break;
        }
        return;
    }

    switch (Function(functionKey)) {
    case Function::Gate:
        switch (layer()) {
        case Layer::Gate:
            setLayer(Layer::GateOffset);
            break;
        case Layer::GateOffset:
            setLayer(Layer::GateProbability);
            break;
        default:
            setLayer(Layer::Gate);
            break;
        }
        break;
    case Function::Retrigger:
        switch (layer()) {
        case Layer::Retrigger:
            setLayer(Layer::RetriggerProbability);
            break;
        case Layer::RetriggerProbability:
            if (engine.playMode() == Types::PlayMode::Free) {
                setLayer(Layer::StageRepeats);
                break;
            }

        case Layer::StageRepeats:
            if (engine.playMode() == Types::PlayMode::Free) {
                setLayer(Layer::StageRepeatsMode);
                break;
            }
        case Layer::StageRepeatsMode:         
            setLayer(Layer::Retrigger);
            break;
        default:
            if (engine.playMode() == Types::PlayMode::Free) {
                setLayer(Layer::StageRepeats);
                break;
            }
            setLayer(Layer::Retrigger);
            break;
        }
        break;
    case Function::Length:
        switch (layer()) {
        case Layer::Length:
            setLayer(Layer::LengthVariationRange);
            break;
        case Layer::LengthVariationRange:
            setLayer(Layer::LengthVariationProbability);
            break;
        default:
            setLayer(Layer::Length);
            break;
        }
        break;
    case Function::Note:
        switch (layer()) {
        case Layer::Note:
            setLayer(Layer::Slide);
            break;
        case Layer::Slide:
            setLayer(Layer::NoteVariationRange);
            break;
        case Layer::NoteVariationRange:
            setLayer(Layer::NoteVariationProbability);
            break;
        case Layer::NoteVariationProbability:
            setLayer(Layer::BypassScale);
            break;
        default:
            setLayer(Layer::Note);
            break;
        }
        break;
    case Function::Condition:
        setLayer(Layer::Condition);
        break;
    }
}

int NoteSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Gate:
    case Layer::GateProbability:
    case Layer::GateOffset:
        return 0;
    case Layer::Retrigger:
    case Layer::RetriggerProbability:
    case Layer::StageRepeats:
    case Layer::StageRepeatsMode:
        return 1;
    case Layer::Length:
    case Layer::LengthVariationRange:
    case Layer::LengthVariationProbability:
        return 2;
    case Layer::Note:
    case Layer::NoteVariationRange:
    case Layer::NoteVariationProbability:
    case Layer::Slide:
    case Layer::BypassScale:
        return 3;
    case Layer::Condition:
        return 4;
    case Layer::Last:
        break;
    }

    return -1;
}

void NoteSequenceEditPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    // TODO should we monitor an all layers not just note?
    if (layer() == Layer::Note && !_stepSelection.isPersisted() && _stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    } else {
        trackEngine.setMonitorStep(-1);
    }
}

void NoteSequenceEditPage::drawDetail(Canvas &canvas, const NoteSequence::Step &step) {

    const auto &sequence = _project.selectedNoteSequence();
    const auto &scale = sequence.selectedScale(_project.scale());

    FixedStringBuilder<16> str;

    WindowPainter::drawFrame(canvas, 64, 16, 128, 32);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    canvas.vline(64 + 32, 16, 32);

    canvas.setFont(Font::Small);
    str("%d", _stepSelection.first() + 1);
    if (_stepSelection.count() > 1) {
        str("*");
    }
    canvas.drawTextCentered(64, 16, 32, 32, str);

    canvas.setFont(Font::Tiny);

    switch (layer()) {
    case Layer::Gate:
    case Layer::Slide:
    case Layer::BypassScale:
        break;
    case Layer::GateProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateProbability(), NoteSequence::GateProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.gateProbability()) / (NoteSequence::GateProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::GateOffset:
        SequencePainter::drawOffset(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateOffset(), NoteSequence::GateOffset::Min - 1, NoteSequence::GateOffset::Max + 1
        );
        str.reset();
        str("%.1f%%", 100.f * step.gateOffset() / float(NoteSequence::GateOffset::Max + 1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Retrigger:
        SequencePainter::drawRetrigger(
            canvas,
            64+ 32 + 8, 32 - 4, 64 - 16, 8,
            step.retrigger() + 1, NoteSequence::Retrigger::Range
        );
        str.reset();
        str("%d", step.retrigger() + 1);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::RetriggerProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.retriggerProbability(), NoteSequence::RetriggerProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.retriggerProbability()) / (NoteSequence::RetriggerProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Length:
        SequencePainter::drawLength(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, NoteSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.length() + 1.f) / NoteSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationRange:
        SequencePainter::drawLengthRange(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, step.lengthVariationRange(), NoteSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationRange()) / NoteSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.lengthVariationProbability(), NoteSequence::LengthVariationProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationProbability()) / (NoteSequence::LengthVariationProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Note:
        str.reset();
        scale.noteName(str, step.note(), sequence.selectedRootNote(_model.project().rootNote()), Scale::Long);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::NoteVariationRange:
        str.reset();
        str("%d", step.noteVariationRange());
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::NoteVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.noteVariationProbability(), NoteSequence::NoteVariationProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.noteVariationProbability()) / (NoteSequence::NoteVariationProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Condition:
        str.reset();
        Types::printCondition(str, step.condition(), Types::ConditionFormat::Long);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 96, 32, str);
        break;
    case Layer::StageRepeats:
        str.reset();
        str("x%d", step.stageRepeats()+1);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
     case Layer::StageRepeatsMode:
        str.reset();
        switch (step.stageRepeatMode()) {
            case Types::Each:
                str("EACH");
                break;
            case Types::First:
                str("FIRST");
                break;
            case Types::Middle:
                str("MIDDLE");
                break;
            case Types::Last:
                str("LAST");
                break;
            case Types::Odd:
                str("ODD");
                break;
            case Types::Even:
                str("EVEN");
                break;
            case Types::Triplets:
                str("TRIPLET");
                break;
            case Types::Random:
                str("RANDOM");
                break;

            default:
                break;
        }
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::Last:
        break;
    }
}

void NoteSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }, doubleClick
    ));
}

void NoteSequenceEditPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        initSequence();
        break;
    case ContextAction::Copy:
        copySequence();
        break;
    case ContextAction::Paste:
        pasteSequence();
        break;
    case ContextAction::Duplicate:
        duplicateSequence();
        break;
    case ContextAction::Generate:
        generateSequence();
        break;
    case ContextAction::Last:
        break;
    }
}

bool NoteSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteNoteSequenceSteps();
    default:
        return true;
    }
}

void NoteSequenceEditPage::initSequence() {
    auto selected = selectedOrAllSteps(_stepSelection);
    auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());
    builder->clearLayer(selected);
    builder->showPreview();
    builder->apply();
    showMessage("LAYER INITIALIZED");
}

void NoteSequenceEditPage::copySequence() {
    _model.clipBoard().copyNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void NoteSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void NoteSequenceEditPage::duplicateSequence() {
    _project.selectedNoteSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}


void NoteSequenceEditPage::tieNotes() {

    auto &sequence = _project.selectedNoteSequence();

    if (_stepSelection.any()) {
        int first=-1;
        int last=-1;

        for (size_t i = 0; i < sequence.steps().size(); ++i) {
            if (_stepSelection[i]) {
                if (first == -1 ) {
                    first = i;
                }
                last = i;
            }
        }

        for (int i = first; i <= last; i++) {
            sequence.step(i).setGate(true);
            if (i != last) {
                sequence.step(i).setLength(NoteSequence::Length::Max);
                showMessage("NOTES TIED");
            }
            sequence.step(i).setNote(sequence.step(first).note());
        }
    }
}

void NoteSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show(true, [this] (bool success, Generator::Mode mode) {
        if (success) {
            if (mode == Generator::Mode::InitLayer || mode == Generator::Mode::InitSteps) {
                _inMemorySequence = _project.selectedNoteSequence();
                auto selected = selectedOrAllSteps(_stepSelection);
                auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());
                Generator::execute(mode, *builder, selected);
                builder->showPreview();
                builder->apply();
                showMessage(mode == Generator::Mode::InitLayer ? "LAYER INITIALIZED" : "STEPS INITIALIZED");
                return;
            }

            if (mode == Generator::Mode::Acid) {
                showAcidGenerator();
                return;
            }

            if (mode == Generator::Mode::Chaos) {
                showChaosGenerator();
                return;
            }

            destroyActiveBuilder();
            auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());
            _activeBuilder = ActiveBuilder::Note;

            if (_stepSelection.none()) {
                _stepSelection.selectAll();
            }

            auto generator = Generator::execute(mode, *builder, _stepSelection.selected());
            if (generator) {
                _manager.pages().generator.show(generator, &_stepSelection);
            }
        }
    });
}

void NoteSequenceEditPage::openLaunchpadGenerator(LaunchpadGenerator generator) {
    if (!inNoteTrackContext()) {
        return;
    }

    // LP generator pages operate on all steps when no persistent selection exists.
    if (generator != LaunchpadGenerator::InitLayer &&
        generator != LaunchpadGenerator::InitSteps &&
        _stepSelection.none()) {
        _stepSelection.selectAll();
    }

    switch (generator) {
    case LaunchpadGenerator::Random:
    case LaunchpadGenerator::Euclidean: {
        destroyActiveBuilder();
        auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());
        _activeBuilder = ActiveBuilder::Note;

        auto generatorMode = generator == LaunchpadGenerator::Random ? Generator::Mode::Random : Generator::Mode::Euclidean;
        auto *generated = Generator::execute(generatorMode, *builder, _stepSelection.selected());
        if (generated) {
            _manager.pages().generator.show(generated, &_stepSelection);
        }
        break;
    }
    case LaunchpadGenerator::AcidPhrase:
        showAcidGenerator(AcidSequenceBuilder::ApplyMode::Phrase);
        break;
    case LaunchpadGenerator::AcidLayer:
        // Keep LP behavior aligned with machine Acid selector semantics:
        // Layer mode is valid only on Gate/Note/Slide, otherwise fall back to Phrase.
        showAcidGenerator(supportsAcidLayerMode() ? AcidSequenceBuilder::ApplyMode::Layer : AcidSequenceBuilder::ApplyMode::Phrase);
        break;
    case LaunchpadGenerator::Vandalize:
        showChaosGenerator(ChaosGenerator::Scope::Sequence);
        break;
    case LaunchpadGenerator::Wreck:
        showChaosGenerator(ChaosGenerator::Scope::Pattern);
        break;
    case LaunchpadGenerator::InitLayer:
    case LaunchpadGenerator::InitSteps: {
        destroyActiveBuilder();
        _inMemorySequence = _project.selectedNoteSequence();
        auto selected = selectedOrAllSteps(_stepSelection);
        auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());
        const bool initLayer = generator == LaunchpadGenerator::InitLayer;
        Generator::execute(initLayer ? Generator::Mode::InitLayer : Generator::Mode::InitSteps, *builder, selected);
        builder->showPreview();
        builder->apply();
        showMessage(initLayer ? "LAYER INITIALIZED" : "STEPS INITIALIZED");
        _builderContainer.destroy(builder);
        break;
    }
    }
}

void NoteSequenceEditPage::launchpadUndo() {
    if (!inNoteTrackContext()) {
        return;
    }

    NoteSequence currentSequence = _project.selectedNoteSequence();
    _project.selectedNoteSequence() = _inMemorySequence;
    _inMemorySequence = currentSequence;
    showMessage("UNDO/REDO");
}

void NoteSequenceEditPage::showAcidGenerator() {
    const bool allowLayer = supportsAcidLayerMode();

    _manager.pages().acidModeSelect.show(allowLayer, [this] (bool success, AcidSequenceBuilder::ApplyMode applyMode) {
        if (!success || !inNoteTrackContext()) {
            return;
        }

        showAcidGenerator(applyMode);
    });
}

bool NoteSequenceEditPage::supportsAcidLayerMode() const {
    return layer() == Layer::Gate || layer() == Layer::Note || layer() == Layer::Slide;
}

void NoteSequenceEditPage::showAcidGenerator(AcidSequenceBuilder::ApplyMode applyMode) {
    if (!inNoteTrackContext()) {
        return;
    }

    destroyActiveBuilder();
    auto builder = _builderContainer.create<AcidSequenceBuilder>(
        _project.selectedNoteSequence(),
        layer(),
        applyMode,
        _stepSelection.selected()
    );
    _activeBuilder = ActiveBuilder::Acid;

    auto generator = Generator::execute(Generator::Mode::Acid, *builder, _stepSelection.selected());
    if (generator) {
        _manager.pages().generator.show(generator, &_stepSelection);
    }
}

void NoteSequenceEditPage::showChaosGenerator() {
    _manager.pages().chaosScopeSelect.show([this] (bool success, ChaosGenerator::Scope scope) {
        if (!success || !inNoteTrackContext()) {
            return;
        }

        if (scope == ChaosGenerator::Scope::Pattern) {
            _manager.pages().wreckPatternWarning.show([this] (WreckPatternWarningPage::Action action) {
                switch (action) {
                case WreckPatternWarningPage::Action::Save:
                    showProjectSavePage();
                    break;
                case WreckPatternWarningPage::Action::Wreck:
                    showChaosGenerator(ChaosGenerator::Scope::Pattern);
                    break;
                case WreckPatternWarningPage::Action::Cancel:
                    break;
                }
            });
            return;
        }

        showChaosGenerator(ChaosGenerator::Scope::Sequence);
    });
}

void NoteSequenceEditPage::showChaosGenerator(ChaosGenerator::Scope scope) {
    if (!inNoteTrackContext()) {
        return;
    }

    destroyActiveBuilder();
    auto builder = _builderContainer.create<ChaosSequenceBuilder>(
        _project,
        _stepSelection.selected(),
        scope == ChaosGenerator::Scope::Pattern ? ChaosSequenceBuilder::Scope::Pattern : ChaosSequenceBuilder::Scope::Sequence
    );
    _activeBuilder = ActiveBuilder::Chaos;
    auto generator = Generator::execute(Generator::Mode::Chaos, *builder, _stepSelection.selected());
    if (generator) {
        auto *chaos = static_cast<ChaosGenerator *>(generator);
        chaos->setScope(scope == ChaosGenerator::Scope::Pattern ? ChaosGenerator::Scope::Pattern : ChaosGenerator::Scope::Sequence);
        chaos->setTargetMask(scope == ChaosGenerator::Scope::Pattern
            ? _model.settings().userSettings().get<ChaosPatLayersSetting>(SettingChaosPatLayers)->getValue()
            : _model.settings().userSettings().get<ChaosSeqLayersSetting>(SettingChaosSeqLayers)->getValue());
        generator->update();
        _manager.pages().generator.show(generator, &_stepSelection);
    }
}

bool NoteSequenceEditPage::inNoteTrackContext() const {
    return _project.selectedTrack().trackMode() == Track::TrackMode::Note;
}

void NoteSequenceEditPage::destroyActiveBuilder() {
    Generator::destroyActive();

    switch (_activeBuilder) {
    case ActiveBuilder::Note:
        _builderContainer.destroy(&(_builderContainer.as<NoteSequenceBuilder>()));
        break;
    case ActiveBuilder::Acid:
        _builderContainer.destroy(&(_builderContainer.as<AcidSequenceBuilder>()));
        break;
    case ActiveBuilder::Chaos:
        _builderContainer.destroy(&(_builderContainer.as<ChaosSequenceBuilder>()));
        break;
    case ActiveBuilder::None:
        break;
    }

    _activeBuilder = ActiveBuilder::None;
}

void NoteSequenceEditPage::showProjectSavePage() {
    _manager.pages().fileSelect.show("SAVE PROJECT", FileType::Project, _project.slotAssigned() ? _project.slot() : 0, true, [this] (bool result, int slot) {
        if (!result) {
            return;
        }

        if (FileManager::slotUsed(FileType::Project, slot)) {
            _manager.pages().confirmation.show("ARE YOU SURE?", [this, slot] (bool confirmed) {
                if (confirmed) {
                    saveProjectToSlot(slot);
                }
            });
        } else {
            saveProjectToSlot(slot);
        }
    });
}

void NoteSequenceEditPage::saveProjectToSlot(int slot) {
    _engine.suspend();
    _manager.pages().busy.show("SAVING PROJECT ...");

    FileManager::task([this, slot] () {
        return FileManager::writeProject(_project, slot);
    }, [this, slot] (fs::Error result) {
        if (result == fs::OK) {
            _project.setSlot(slot);
            _project.setAutoLoaded(false);
            showMessage("PROJECT SAVED");
        } else {
            showMessage(FixedStringBuilder<32>("FAILED (%s)", fs::errorToString(result)));
        }
        _manager.pages().busy.close();
        _engine.resume();
    });
}

void NoteSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedNoteSequence());
    if (quickEditItems[index] != NoteSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}

bool NoteSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedNoteSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex] && !sequence.step(stepIndex).gate()) {
            return false;
        }
    }
    return true;
}

void NoteSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedNoteSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(gate);
        }
    }
}
