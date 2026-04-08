#include "LogicSequenceEditPage.h"

#include "LayoutPage.h"
#include "Pages.h"

#include "model/LogicSequence.h"
#include "ui/LedPainter.h"
#include "ui/StepSelectionUtils.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"
#include "engine/SequenceUtils.h"
#include "engine/generators/ChaosEntropyGenerator.h"

#include "model/Scale.h"
#include "model/UserSettings.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"
#include <array>
#include <bitset>
#include <cstdint>


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


static const LogicSequenceListModel::Item quickEditItems[8] = {
    LogicSequenceListModel::Item::FirstStep,
    LogicSequenceListModel::Item::LastStep,
    LogicSequenceListModel::Item::RunMode,
    LogicSequenceListModel::Item::Divisor,
    LogicSequenceListModel::Item::ResetMeasure,
    LogicSequenceListModel::Item::Scale,
    LogicSequenceListModel::Item::RootNote,
    LogicSequenceListModel::Item::Last
};

LogicSequenceEditPage::LogicSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedLogicSequenceLayer();
        const auto &sequence = _project.selectedLogicSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void LogicSequenceEditPage::enter() {
    updateMonitorStep();

    _inMemorySequence = _project.selectedLogicSequence();
    auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
    auto *ne1 = &trackEngine.input1TrackEngine();
    auto &track = _project.selectedTrack().logicTrack();

    if (ne1==nullptr && track.inputTrack1()!=-1) {
        auto *ne = &_engine.trackEngine(track.inputTrack1()).as<NoteTrackEngine>();
        trackEngine.setInput1TrackEngine(ne);
    }
    auto *ne2 = &trackEngine.input2TrackEngine();
    if (ne2==nullptr && track.inputTrack2()!=-1) {
        auto &track = _project.selectedTrack().logicTrack();
        auto *ne = &_engine.trackEngine(track.inputTrack2()).as<NoteTrackEngine>();
        trackEngine.setInput2TrackEngine(ne);
    }

    _showDetail = false;
}

void LogicSequenceEditPage::exit() {
    _engine.selectedTrackEngine().as<LogicTrackEngine>().setMonitorStep(-1);
}

void LogicSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    auto &track = _project.selectedTrack().logicTrack();

    /* Prepare flags shown before mode name (top right header) */
    const auto pattern_follow = track.patternFollow();
    const char* pf_repr = Types::patternFollowShortRepresentation(pattern_follow);

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", pf_repr);

    WindowPainter::drawActiveFunction(canvas, LogicSequence::layerName(layer()));
    if (_launchpadGeneratorModeActive) {
        WindowPainter::drawFooter(canvas);
        drawLaunchpadGeneratorOverlay(canvas);
        return;
    }
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    const auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();

    auto &sequence = _project.selectedLogicSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

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

    std::array<EvalStep, CONFIG_STEP_COUNT> evalSteps;
    for (int i = 0; i< StepCount; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);
        EvalStep es = EvalStep();
        es.setLogicStep(step.gate());

        if (track.inputTrack1() != -1) {
            const int currentStep1 = trackEngine.input1TrackEngine().currentStep();
            int stepIndex1 = currentStep1 != -1 ? (currentStep1 - currentStep) + stepIndex : stepIndex;
            const auto &noteTrack = _project.track(track.inputTrack1()).noteTrack();
            const auto &inputSeq1 = noteTrack.sequence(_project.selectedPatternIndex());
            int idx = SequenceUtils::rotateStep(stepIndex1, inputSeq1.firstStep(), inputSeq1.lastStep(), noteTrack.rotate());
            es.setInput1Step(inputSeq1.step(idx).gate());            
        }

        if (track.inputTrack2() != -1) {
            const int currentStep2 = trackEngine.input2TrackEngine().currentStep();
            int stepIndex2 = currentStep2 != -1 ? (currentStep2 - currentStep) + stepIndex : stepIndex;
            const auto &noteTrack = _project.track(track.inputTrack2()).noteTrack();
            const auto &inputSeq2 = noteTrack.sequence(_project.selectedPatternIndex());
            int idx = SequenceUtils::rotateStep(stepIndex2, inputSeq2.firstStep(), inputSeq2.lastStep(), noteTrack.rotate());
            es.setInput2Step(inputSeq2.step(idx).gate());            
        }

        evalSteps[stepIndex] = es;
    }
    
    for (int i = 0; i < StepCount; ++i) {
        int stepIndex = stepOffset + i;
        EvalStep evalStep = evalSteps[stepIndex];
        int x = i * stepWidth;
        int y = 20;

        // step index
        SequencePainter::drawStepIndex(canvas, x, y, stepWidth, stepIndex + 1, _stepSelection[stepIndex], sequence.step(stepIndex).condition() != Types::Condition::Off);

        // step gate
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (evalStep.logicStep() && !globalKeyState()[Key::Shift]) {
            canvas.setColor(SequencePainter::dimSequenceColor(
                _context.model.settings().userSettings().get<DimSequenceSetting>(SettingDimSequence)->getValue()
            ));
            if (stepIndex == currentStep) {
                if (trackEngine.gateOutput(currentStep)) {
                    canvas.fillRect(x + 6, y + 6, stepWidth - 12, stepWidth - 12);
                    canvas.setColor(Color::Medium);
                    canvas.hline(x + 7, y + 5, 2);
                    canvas.hline(x + 7, y + 10, 2);
                    canvas.vline(x + 5, y + 7, 2);
                    canvas.vline(x + 10, y + 7, 2);
                } else {
                    SequencePainter::drawGateBody(canvas, x, y, stepWidth, sequence.step(stepIndex).gateOffset(), LogicSequence::GateOffset::Max, sequence.step(stepIndex).length(), LogicSequence::Length::Range, sequence.step(stepIndex).retrigger(), LogicSequence::Retrigger::Range, sequence.step(stepIndex).slide());
                }
            } else {
                SequencePainter::drawGateBody(canvas, x, y, stepWidth, sequence.step(stepIndex).gateOffset(), LogicSequence::GateOffset::Max, sequence.step(stepIndex).length(), LogicSequence::Length::Range, sequence.step(stepIndex).retrigger(), LogicSequence::Retrigger::Range, sequence.step(stepIndex).slide());
            }
        } else {
            if (track.detailedView()) {

                if (evalStep.input1Step()) {
                    canvas.fillRect(x + 6, y + 6, 4, 4);
                }
                if (evalStep.input2Step()) {
                    canvas.hline(x + 4, y + 4, 8);
                    canvas.hline(x + 4, y + 11, 8);
                    canvas.vline(x + 4, y + 4, 8);
                    canvas.vline(x + 11, y + 4, 7);
                } 
            }
        }
        const auto &step = sequence.step(stepIndex);

        switch (layer()) {
        case Layer::Gate:
            break;
        case Layer::GateLogic:
            SequencePainter::drawGateLogicMode(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.gateLogic()
            );
            break;
        case Layer::GateProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateProbability() + 1, LogicSequence::GateProbability::Range
            );
            break;
        case Layer::GateOffset:
            SequencePainter::drawOffset(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateOffset(), LogicSequence::GateOffset::Min - 1, LogicSequence::GateOffset::Max + 1
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
                step.length() + 1, LogicSequence::Length::Range
            );
            break;
        case Layer::LengthVariationRange:
            SequencePainter::drawLengthRange(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, step.lengthVariationRange(), LogicSequence::Length::Range
            );
            break;
        case Layer::LengthVariationProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.lengthVariationProbability() + 1, LogicSequence::LengthVariationProbability::Range
            );
            break;
        case Layer::NoteLogic: {
            SequencePainter::drawNoteLogicMode(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.noteLogic()
            );
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
                step.noteVariationProbability() + 1, LogicSequence::NoteVariationProbability::Range
            );
            break;
        
        case Layer::Slide:
            SequencePainter::drawSlide(
                canvas,
                x + 4, y + 18, stepWidth - 8, 4,
                step.slide()
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
        if (layer() == Layer::Gate || layer() == Layer::Slide || _stepSelection.none()) {
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

void LogicSequenceEditPage::drawLaunchpadGeneratorOverlay(Canvas &canvas) {
    static const char *overlayCells[2][6] = {
        { "RAND", nullptr, "ENTPY", "EUCL", nullptr, nullptr },
        { nullptr, nullptr, nullptr, nullptr, nullptr, "INITS" },
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

void LogicSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
    auto &sequence = _project.selectedLogicSequence();
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
            leds.set(index, false, quickEditItems[i] != LogicSequenceListModel::Item::Last);
            leds.mask(index);
        }
        int index = MatrixMap::fromStep(15);
        leds.unmask(index);
        leds.set(index, false, true);
        leds.mask(index);
        index = MatrixMap::fromStep(4);
        leds.unmask(index);
        leds.set(index, false, true);
        leds.mask(index);
    }
}

void LogicSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
    updateMonitorStep();
}

void LogicSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
    updateMonitorStep();
}

void LogicSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedLogicSequence();
    auto &track = _project.selectedTrack().logicTrack();

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
        // undo function
        launchpadUndo();
        event.consume();
        return;
    }

    if (key.isQuickEdit()) {
         if (key.is(Key::Step15)) {
            bool lpConnected = _engine.isLaunchpadConnected();

             track.togglePatternFollowDisplay(lpConnected);
        } else {
            _inMemorySequence = _project.selectedLogicSequence();
            quickEdit(key.quickEdit());
        }
        event.consume();
        return;
    }

    if (key.pageModifier() && key.is(Key::Step4)) {
            track.toggleDetailedView();
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
            _inMemorySequence = _project.selectedLogicSequence();
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
            _inMemorySequence = _project.selectedLogicSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
        }
    }

    if (key.isFunction()) {
        if(key.shiftModifier() && key.function() == 2 && _stepSelection.any()) {
            _inMemorySequence = _project.selectedLogicSequence();
            tieNotes();
            event.consume();
            return;
        }
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
    }

    if (key.isEncoder()) {
        track.setPatternFollowDisplay(false);
        _inMemorySequence = _project.selectedLogicSequence();
        if (!_showDetail && _stepSelection.any() && allSelectedStepsActive()) {
            setSelectedStepsGate(false);
        } else {
            setSelectedStepsGate(true);
        }
        event.consume();
    }


    if (key.isLeft()) {
        if (key.shiftModifier()) {
            _inMemorySequence = _project.selectedLogicSequence();
            sequence.shiftSteps(_stepSelection.selected(), -1);
            _stepSelection.shiftLeft(sequence.firstStep(), sequence.lastStep() + 1);
        } else {
            track.setPatternFollowDisplay(false);
             int sectionCount = sequence.lastStep() / StepCount + 1;
             sequence.setSecion((sequence.section() + sectionCount - 1) % sectionCount);
        }
        event.consume();
    }
    if (key.isRight()) {
        if (key.shiftModifier()) {
            _inMemorySequence = _project.selectedLogicSequence();
            sequence.shiftSteps(_stepSelection.selected(), 1);
            _stepSelection.shiftRight(sequence.firstStep(), sequence.lastStep() + 1);
        } else {
            track.setPatternFollowDisplay(false);
            int sectionCount = sequence.lastStep() / StepCount + 1;
            sequence.setSecion((sequence.section() + 1) % sectionCount);
        }
        event.consume();
    }
}

void LogicSequenceEditPage::launchpadUndo() {
    LogicSequence currentSequence = _project.selectedLogicSequence();
    _project.selectedLogicSequence() = _inMemorySequence;
    _inMemorySequence = currentSequence;
    showMessage("UNDO/REDO");
}

void LogicSequenceEditPage::openLaunchpadGenerator(Generator::Mode mode) {
    _inMemorySequence = _project.selectedLogicSequence();
    auto builder = _builderContainer.create<LogicSequenceBuilder>(_project.selectedLogicSequence(), layer());

    if (mode == Generator::Mode::InitSteps || mode == Generator::Mode::InitLayer) {
        auto selected = selectedOrAllSteps(_stepSelection);
        Generator::execute(Generator::Mode::InitSteps, *builder, selected);
        builder->showPreview();
        builder->apply();
        showMessage("STEPS INITIALIZED");
        return;
    }

    if (_stepSelection.none()) {
        _stepSelection.selectAll();
    }

    auto *generator = Generator::execute(mode, *builder, _stepSelection.selected());
    if (generator) {
        if (mode == Generator::Mode::ChaosEntropy) {
            auto *entropy = static_cast<ChaosEntropyGenerator *>(generator);
            entropy->setTargetMask(_model.settings().userSettings().get<EntropyLayersSetting>(SettingEntropyLayers)->getValue());
            entropy->update();
        }
        _manager.pages().generator.show(generator, &_stepSelection);
    }
}

void LogicSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedLogicSequence();
    const auto &scale = sequence.selectedScale(_project.scale());

    if (!_stepSelection.any())
    {
        switch (layer())
        {
        case Layer::Gate:
            setLayer(event.value() > 0 ? Layer::GateLogic : Layer::GateProbability);
            break;
        case Layer::GateLogic:
            setLayer(event.value() > 0 ? Layer::GateOffset : Layer::Gate);
            break;
        case Layer::GateOffset:
            setLayer(event.value() > 0 ? Layer::GateProbability : Layer::GateLogic);
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
        case Layer::NoteLogic:
            setLayer(event.value() > 0 ? Layer::NoteVariationRange : Layer::Slide);
            break;
        case Layer::NoteVariationRange:
            setLayer(event.value() > 0 ? Layer::NoteVariationProbability : Layer::NoteLogic);
            break;
        case Layer::NoteVariationProbability:
            setLayer(event.value() > 0 ? Layer::Slide : Layer::NoteVariationRange);
            break;
        case Layer::Slide:
            setLayer(event.value() > 0 ? Layer::NoteLogic : Layer::NoteVariationProbability);
            break;
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
            case Layer::GateLogic:
                step.setGateLogic(static_cast<LogicSequence::GateLogicMode>(step.gateLogic() + event.value()));
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
            case Layer::NoteLogic:
                step.setNoteLogic(static_cast<LogicSequence::NoteLogicMode>(step.noteLogic() + event.value()));
                updateMonitorStep();
                break;
             case Layer::NoteVariationRange:
                step.setNoteVariationRange(step.noteVariationRange() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                break;
            case Layer::NoteVariationProbability:
                step.setNoteVariationProbability(step.noteVariationProbability() + event.value());
                break;
            case Layer::Slide:
                step.setSlide(event.value() > 0);
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

void LogicSequenceEditPage::midi(MidiEvent &event) {
    if (!_engine.recording() && layer() == Layer::NoteLogic && _stepSelection.any()) {
        auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
        auto &sequence = _project.selectedLogicSequence();
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

void LogicSequenceEditPage::switchLayer(int functionKey, bool shift) {

    auto engine = _engine.selectedTrackEngine().as<LogicTrackEngine>();

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
            setLayer(Layer::NoteLogic);
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
            setLayer(Layer::GateLogic);
            break;
        case Layer::GateLogic:
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

        default:
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
        case Layer::NoteLogic:
            setLayer(Layer::NoteVariationRange);
            break;
        case Layer::NoteVariationRange:
            setLayer(Layer::NoteVariationProbability);
            break;
        case Layer::NoteVariationProbability:
            setLayer(Layer::Slide);
            break;
        default:
            setLayer(Layer::NoteLogic);
            break;
        }
        break;
    case Function::Condition:
        setLayer(Layer::Condition);
        break;
    }
}

int LogicSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Gate:
    case Layer::GateProbability:
    case Layer::GateOffset:
    case Layer::GateLogic:
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
    case Layer::NoteLogic:
    case Layer::NoteVariationRange:
    case Layer::NoteVariationProbability:
    case Layer::Slide:
        return 3;
    case Layer::Condition:
        return 4;
    case Layer::Last:
        break;
    }

    return -1;
}

void LogicSequenceEditPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();

    // TODO should we monitor an all layers not just note?
    if (layer() == Layer::NoteLogic && !_stepSelection.isPersisted() && _stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    } else {
        trackEngine.setMonitorStep(-1);
    }
}

void LogicSequenceEditPage::drawDetail(Canvas &canvas, const LogicSequence::Step &step) {

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
        break;

    case Layer::GateLogic:
        str.reset();
        switch (step.gateLogic()) {
            case LogicSequence::GateLogicMode::One:
                str("INPUT 1");
                break;
            case LogicSequence::GateLogicMode::Two:
                str("INPUT 2");
                break;
            case LogicSequence::GateLogicMode::And:
                str("AND");
                break;
            case LogicSequence::GateLogicMode::Or:
                str("OR");
                break;
            case LogicSequence::GateLogicMode::Xor:
                str("XOR");
                break;
            case LogicSequence::GateLogicMode::Nand:
                str("NAND");
                break;
            
            case LogicSequence::GateLogicMode::RandomInput:
                str("RND INPUT");
                break;
            case LogicSequence::GateLogicMode::RandomLogic:
                str("RND LOGIC");
                break;
            default:
                break;
        }
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::GateProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateProbability(), LogicSequence::GateProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.gateProbability()) / (LogicSequence::GateProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::GateOffset:
        SequencePainter::drawOffset(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateOffset(), LogicSequence::GateOffset::Min - 1, LogicSequence::GateOffset::Max + 1
        );
        str.reset();
        str("%.1f%%", 100.f * step.gateOffset() / float(LogicSequence::GateOffset::Max + 1));
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
            step.length() + 1, LogicSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.length() + 1.f) / LogicSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationRange:
        SequencePainter::drawLengthRange(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, step.lengthVariationRange(), LogicSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationRange()) / LogicSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.lengthVariationProbability(), LogicSequence::LengthVariationProbability::Range-1
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationProbability()) / (LogicSequence::LengthVariationProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::NoteLogic:
        str.reset();
        switch (step.noteLogic()) {
            case LogicSequence::NoteLogicMode::NOne:
                str("INPUT 1");
                break;
            case LogicSequence::NoteLogicMode::NTwo:
                str("INPUT 2");
                break;
            case LogicSequence::NoteLogicMode::Min:
                str("MIN");
                break;
            case LogicSequence::NoteLogicMode::Max:
                str("MAX");
                break;
            case LogicSequence::NoteLogicMode::Sum:
                str("SUM");
                break;
            case LogicSequence::NoteLogicMode::Avg:
                str("AVG");
                break;
            case LogicSequence::NoteLogicMode::NRandomInput:
                str("RND INPUT");
                break;
            case LogicSequence::NoteLogicMode::NRandomLogic:
                str("RND LOGIC");
                break;
            default:
                break;
        }
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 64, 32 - 4, 32, 8, str);
        break;



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

void LogicSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }, doubleClick
    ));
}

void LogicSequenceEditPage::contextAction(int index) {
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

bool LogicSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteLogicSequenceSteps();
    default:
        return true;
    }
}

void LogicSequenceEditPage::initSequence() {
    auto selected = selectedOrAllSteps(_stepSelection);
    auto builder = _builderContainer.create<LogicSequenceBuilder>(_project.selectedLogicSequence(), layer());
    builder->clearLayer(selected);
    builder->showPreview();
    builder->apply();
    showMessage("LAYER INITIALIZED");
}

void LogicSequenceEditPage::copySequence() {
    _model.clipBoard().copyLogicSequenceSteps(_project.selectedLogicSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void LogicSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteLogicSequenceSteps(_project.selectedLogicSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void LogicSequenceEditPage::duplicateSequence() {
    _project.selectedLogicSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}


void LogicSequenceEditPage::tieNotes() {

    auto &sequence = _project.selectedLogicSequence();

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
                sequence.step(i).setLength(LogicSequence::Length::Max);
                showMessage("NOTES TIED");
            }
            sequence.step(i).setNote(sequence.step(first).note());
        }
    }
}

void LogicSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show([this] (bool success, Generator::Mode mode) {
        if (success) {
            auto builder = _builderContainer.create<LogicSequenceBuilder>(_project.selectedLogicSequence(), layer());

            if (mode == Generator::Mode::InitLayer || mode == Generator::Mode::InitSteps) {
                auto selected = selectedOrAllSteps(_stepSelection);
                Generator::execute(mode, *builder, selected);
                builder->showPreview();
                builder->apply();
                showMessage(mode == Generator::Mode::InitLayer ? "LAYER INITIALIZED" : "STEPS INITIALIZED");
                return;
            }

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

void LogicSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedLogicSequence());
    if (quickEditItems[index] != LogicSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}

bool LogicSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedLogicSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex] && !sequence.step(stepIndex).gate()) {
            return false;
        }
    }
    return true;
}

void LogicSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedLogicSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(gate);
        }
    }
}
