#include "ArpSequenceEditPage.h"

#include "Pages.h"

#include "model/ArpSequence.h"
#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "model/Scale.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"
#include <cstddef>
#include <iostream>

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate, 
    Generate,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
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

static const ArpSequenceListModel::Item quickEditItems[8] = {
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Divisor,
    ArpSequenceListModel::Item::ResetMeasure,
    ArpSequenceListModel::Item::Scale,
    ArpSequenceListModel::Item::RootNote,
    ArpSequenceListModel::Item::Last
};

static const ArpTrackListModel::Item quickEditTrackItems[4] {
    ArpTrackListModel::ArpeggiatorDivisor,
    ArpTrackListModel::ArpeggiatorMode,
    ArpTrackListModel::ArpeggiatorOctaves,
    ArpTrackListModel::Item::Last
};

ArpSequenceEditPage::ArpSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedArpSequenceLayer();
        const auto &sequence = _project.selectedArpSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void ArpSequenceEditPage::enter() {
    updateMonitorStep();
    _showDetail = false;
    _section = 0;
}

void ArpSequenceEditPage::exit() {
    _engine.selectedTrackEngine().as<ArpTrackEngine>().setMonitorStep(-1);
}

void ArpSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    /* Prepare flags shown before mode name (top right header) */
    auto &sequence = _project.selectedArpSequence();
    auto &track = _project.selectedTrack().arpTrack();

    const char *mode_flags = NULL;
    if (track.midiKeyboard()) {
        const char *st_flag = "K";
        mode_flags = st_flag;
    }

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", mode_flags);

    WindowPainter::drawActiveFunction(canvas, ArpSequence::layerName(layer()));
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    const auto &scale = sequence.selectedScale(_project.scale());
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    int currentRecordStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentRecordStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();

    int stepsToDraw = 12;

    // Track Pattern Section on the UI
    if (_sectionTracking && _engine.state().running()) {
        bool section_change = bool((currentStep) % StepCount == 0); // StepCount is relative to screen
        int section_no = int((currentStep) / StepCount);
        if (section_change && section_no != _section) {
            _section = section_no;
        }
    }

    for (int i = 0; i < stepsToDraw; ++i) {
        int stepIndex = stepOffset + i;
        auto &step = sequence.step(stepIndex);

        int x = (i * stepWidth) + ((16 - stepsToDraw)*stepWidth)/2 ;
        int y = 20;

        // step index
        {
            canvas.setColor(_stepSelection[stepIndex] ? Color::Bright : Color::Medium);
            FixedStringBuilder<8> str("%d", stepIndex + 1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y - 2, str);
        }

        // step gate
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(_context.model.settings().userSettings().get<DimSequenceSetting>(SettingDimSequence)->getValue() ? Color::Low : Color::Bright);
            constexpr int gateInset = 3;
            constexpr int gateShiftRange = 3;
            int gateArea = stepWidth - 2 * gateInset;
            int gateOffsetShift = (step.gateOffset() * gateShiftRange) / (ArpSequence::GateOffset::Max + 1);
            int gateWidth = 3 + ((gateArea - 3) * (step.length() + 1)) / ArpSequence::Length::Range;
            int gateX = x + gateInset + gateOffsetShift + (gateArea - gateWidth) / 2;
            if (step.retrigger() > 0) {
                int stripeStart = gateX;
                if (gateWidth >= 8) {
                    stripeStart += 1;
                }
                for (int stripeX = stripeStart; stripeX < gateX + gateWidth; stripeX += 2) {
                    canvas.fillRect(stripeX, y + gateInset, 1, gateArea);
                }
            } else {
                canvas.fillRect(gateX, y + gateInset, gateWidth, gateArea);
            }
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
        case Layer::Gate: {
                int rootNote = sequence.selectedRootNote(_model.project().rootNote());

                if (scale.isNotePresent(step.note())) {
                    canvas.setColor(Color::Bright);
                } else {
                    canvas.setColor(Color::Low);
                }
                
                FixedStringBuilder<8> str;
                if (step.bypassScale()) {
                    const Scale &bypassScale = std::ref(Scale::get(0));
                    bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
                    canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                    break;
                } 
                scale.noteName(str, step.note(), rootNote, Scale::Short1);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            }
            break;
        case Layer::GateProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateProbability() + 1, ArpSequence::GateProbability::Range
            );
            break;
        case Layer::GateOffset:
            SequencePainter::drawOffset(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateOffset(), ArpSequence::GateOffset::Min - 1, ArpSequence::GateOffset::Max + 1
            );
            break;
        case Layer::Retrigger:
            SequencePainter::drawRetrigger(
                canvas,
                x, y + 18, stepWidth, 2,
                step.retrigger() + 1, ArpSequence::Retrigger::Range
            );
            break;
        case Layer::RetriggerProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.retriggerProbability() + 1, ArpSequence::RetriggerProbability::Range
            );
            break;
        case Layer::Length:
            SequencePainter::drawLength(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, ArpSequence::Length::Range
            );
            break;
        case Layer::LengthVariationRange:
            SequencePainter::drawLengthRange(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, step.lengthVariationRange(), ArpSequence::Length::Range
            );
            break;
        case Layer::LengthVariationProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.lengthVariationProbability() + 1, ArpSequence::LengthVariationProbability::Range
            );
            break;
        case Layer::NoteOctave: {
            if (step.noteOctave() != 0) {
                canvas.setColor(Color::Bright);
            }
            FixedStringBuilder<8> str;

            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            if (scale.isNotePresent(step.note())) {
                canvas.setColor(Color::Bright);
            } else {
                canvas.setColor(Color::Low);
            }
            if (step.bypassScale()) {
                const Scale &bypassScale = std::ref(Scale::get(0));
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
            
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
                str.reset();
                str("%d", step.noteOctave());
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                break;
            } 
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::NoteOctaveProbability: {
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.noteOctaveProbability() + 1, ArpSequence::NoteOctaveProbability::Range
            );
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            if (scale.isNotePresent(step.note())) {
                canvas.setColor(Color::Bright);
            } else {
                canvas.setColor(Color::Low);
            }
            FixedStringBuilder<8> str;
            if (step.bypassScale()) {
                const Scale &bypassScale = std::ref(Scale::get(0));
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                break;
            } 
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::Note: {
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;

            if (step.bypassScale()) {
                if (scale.isNotePresent(step.note())) {
                    canvas.setColor(Color::Bright);
                } else {
                    canvas.setColor(Color::Low);
                }
                const Scale &bypassScale = std::ref(Scale::get(0));
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
            
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
                str.reset();
                str("%d", step.noteOctave());
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                break;
            } 
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::NoteVariationRange: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str("%d", step.noteVariationRange());
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::NoteVariationProbability: {
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.noteVariationProbability() + 1, ArpSequence::NoteVariationProbability::Range
            );
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());

            if (scale.isNotePresent(step.note())) {
                canvas.setColor(Color::Bright);
            } else {
                canvas.setColor(Color::Low);
            }
            
            FixedStringBuilder<8> str;
            if (step.bypassScale()) {
                const Scale &bypassScale = std::ref(Scale::get(0));
                bypassScale.noteName(str, step.note(), rootNote, Scale::Short1);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
                break;
            } 
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
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
        case Layer::Last:
            break;
        }
    }

    // display arp detail

    int x = (12 * stepWidth) + ((16 - stepsToDraw)*stepWidth)/2 ;
    int y = 20;
    FixedStringBuilder<8> str;
    canvas.setColor(Color::Bright);
    track.arpeggiator().printDivisorAbbr(str);
    canvas.drawText(x + 2,  y -2, str);
    str.reset();
    track.arpeggiator().printModeAbbr(str);
    canvas.drawText(x + 2,  y -2 + 8, str);
    str.reset();
    track.arpeggiator().printOctavesAbbr(str);
    canvas.drawText(x + 2,  y -2 + 16, str);
    str.reset();
    sequence.printScaleAbbr(str);
    canvas.drawText(x + 2,  y -2 + 24, str);
    str.reset();
    if (sequence.rootNote()==-1) {
        str("DFLT");
    } else {
        sequence.printRootNote(str);
    }
    canvas.drawText(x + 2,  y -2 + 32, str);


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

void ArpSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    const auto &sequence = _project.selectedArpSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset() + i;
        bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
        bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, _section);

    // show quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            if (i >=0 && i<=2) {
                leds.set(index, false, quickEditTrackItems[i] != ArpTrackListModel::Item::Last);
            } else {
                leds.set(index, false, quickEditItems[i] != ArpSequenceListModel::Item::Last);
            }
            leds.mask(index);
        }

        for (int i : {15}) {
            int index = MatrixMap::fromStep(i);
            leds.unmask(index);
            leds.set(index, false, true);
            leds.mask(index);
        }
    }
}

void ArpSequenceEditPage::keyDown(KeyEvent &event) {
    const auto &key = event.key();
    if (key.is(Key::Step15) || key.is(Key::Step14)|| key.is(Key::Step13) || key.is(Key::Step12)) {
        return;
    }
    _stepSelection.keyDown(event, stepOffset());
    updateMonitorStep();
    auto &track = _project.selectedTrack().arpTrack();
    bool shift = globalKeyState()[Key::Shift];
    bool page =  globalKeyState()[Key::Page];
    if (track.midiKeyboard() && _engine.state().running() && (key.isStep() && !page && !shift)) {
        auto i = MatrixMap::toStep(key.code());
        auto &sequence = _project.selectedArpSequence();
        auto &arpEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();
        arpEngine.addNote(i, i, ArpTrackEngine::Type::MIDI, sequence.step(i).noteOctave());
        arpEngine.setKeyPressed(i, true);

    }
}

void ArpSequenceEditPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();
    if (key.is(Key::Step15) || key.is(Key::Step14)|| key.is(Key::Step13) || key.is(Key::Step12)) {
        return;
    }
    _stepSelection.keyUp(event, stepOffset());
    updateMonitorStep();
    auto &track = _project.selectedTrack().arpTrack();
    bool shift = globalKeyState()[Key::Shift];
    bool page =  globalKeyState()[Key::Page];
    if (track.midiKeyboard() && _engine.state().running() && key.isStep() && (key.isStep() && !page &&! shift)) {
        auto i = MatrixMap::toStep(key.code());
        auto &arpEngine = _engine.trackEngine(_project.selectedTrackIndex()).as<ArpTrackEngine>();
        auto &sequence = _project.selectedArpSequence();
        if (!sequence.step(i).gate()) {
            arpEngine.removeNote(i);
        }
        arpEngine.setKeyPressed(i, false);
    }

}

void ArpSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedArpSequence();
    auto &track = _project.selectedTrack().arpTrack();


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

    if (key.isQuickEdit()) {
        if (key.is(Key::Step15)) {
            track.toggleMidiKeybaord();
            track.arpeggiator().setHold(false);
        } else {
            quickEdit(key.quickEdit());
        }
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    if (key.is(Key::Step15) || key.is(Key::Step14)|| key.is(Key::Step13) || key.is(Key::Step12)) {
        return;
    }

    _stepSelection.keyPress(event, stepOffset());
    updateMonitorStep();

    if (!key.shiftModifier() && key.isStep()) {
        int stepIndex = stepOffset() + key.step();
        switch (layer()) {
        case Layer::Gate:{            
            auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
            
            if (!track.midiKeyboard()) {
                if (sequence.step(stepIndex).gate()) {
                    trackEngine.removeNote(sequence.step(stepIndex).note());
                } else {
                    trackEngine.addNote(sequence.step(stepIndex).note(), stepIndex, ArpTrackEngine::Type::Sequencer, sequence.step(stepIndex).noteOctave());
                }
                sequence.step(stepIndex).toggleGate();
            } 
            event.consume();
        }
            break;
        default:
            break;
        }
    }

    KeyPressEvent keyPressEvent =_keyPressEventTracker.process(key);

    if (!key.shiftModifier() && key.isStep() && keyPressEvent.count() == 2) {
        int stepIndex = stepOffset() + key.step();
        if (layer() != Layer::Gate) {
            auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
            if (!track.midiKeyboard()) {
                 if (sequence.step(stepIndex).gate()) {
                    trackEngine.removeNote(sequence.step(stepIndex).note());
                } else {
                    trackEngine.addNote(sequence.step(stepIndex).note(), stepIndex, ArpTrackEngine::Type::Sequencer, sequence.step(stepIndex).noteOctave());
                }
                sequence.step(stepIndex).toggleGate();
            } 
            event.consume();
        }
    }

    if (key.isFunction()) {
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
    }

    if (key.isEncoder()) {
        if (!_showDetail && _stepSelection.any() && allSelectedStepsActive()) {
            setSelectedStepsGate(false);
        } else {
            setSelectedStepsGate(true);
        }
    }
}

void ArpSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedArpSequence();
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
            setLayer(event.value() > 0 ? Layer::RetriggerProbability : Layer::Retrigger);
            break;
        case Layer::RetriggerProbability:
            setLayer(event.value() > 0 ? Layer::RetriggerProbability : Layer::Retrigger);
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
            setLayer(event.value() > 0 ? Layer::NoteVariationRange : Layer::Slide);
            break;
        case Layer::NoteVariationRange:
            setLayer(event.value() > 0 ? Layer::NoteVariationProbability : Layer::Note);
            break;
        case Layer::NoteVariationProbability:
            setLayer(event.value() > 0 ? Layer::NoteOctave : Layer::NoteVariationRange);
            break;
        case Layer::NoteOctave:
            setLayer(event.value() > 0 ? Layer::NoteOctaveProbability : Layer::NoteVariationProbability);
            break;
        case Layer::NoteOctaveProbability:
            setLayer(event.value() > 0 ? Layer::Slide : Layer::NoteOctave);
            break;
        case Layer::Slide:
            setLayer(event.value() > 0 ? Layer::Note : Layer::NoteOctaveProbability);
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
            case Layer::NoteOctave:
                step.setNoteOctave(step.noteOctave() + event.value());
                updateMonitorStep();
                break;
            case Layer::NoteOctaveProbability:
                step.setNoteOctaveProbability(step.noteOctaveProbability() + event.value());
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
                updateMonitorStep();
                break;
            case Layer::Slide:
                step.setSlide(event.value() > 0);
                break;
            case Layer::Condition:
                step.setCondition(ModelUtils::adjustedEnum(step.condition(), event.value()));
                break;
            case Layer::Last:
                break;
            }
        }
    }

    event.consume();
}

void ArpSequenceEditPage::midi(MidiEvent &event) {
    if (!_engine.recording() && layer() == Layer::NoteVariationProbability && _stepSelection.any()) {
        auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
        auto &sequence = _project.selectedArpSequence();
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

void ArpSequenceEditPage::switchLayer(int functionKey, bool shift) {

    if (shift) {
        switch (Function(functionKey)) {
        case Function::Gate:
            setLayer(Layer::GateProbability);
            break;
        case Function::Retrigger:
            break;
        case Function::Length:
            break;
        case Function::Note:
            setLayer(Layer::Note);
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
        case Layer::Note:
            setLayer(Layer::NoteVariationRange);
            break;
        case Layer::NoteVariationRange:
            setLayer(Layer::NoteVariationProbability);
            break;
        case Layer::NoteVariationProbability:
            setLayer(Layer::NoteOctave);
            break;
        case Layer::NoteOctave:
            setLayer(Layer::NoteOctaveProbability);
            break;
        case Layer::NoteOctaveProbability:
            setLayer(Layer::Slide);
            break;
        case Layer::Slide:
            setLayer(Layer::NoteVariationProbability);
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

int ArpSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Gate:
    case Layer::GateProbability:
    case Layer::GateOffset:
        return 0;
    case Layer::Retrigger:
    case Layer::RetriggerProbability:
        return 1;
    case Layer::Length:
    case Layer::LengthVariationRange:
    case Layer::LengthVariationProbability:
        return 2;
    case Layer::NoteOctave:
    case Layer::NoteVariationProbability:
    case Layer::NoteOctaveProbability:
    case Layer::Slide:
    case Layer::Note:
    case Layer::NoteVariationRange:
        return 3;
    case Layer::Condition:
        return 4;
    case Layer::Last:
        break;
    }

    return -1;
}

void ArpSequenceEditPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();

    // TODO should we monitor an all layers not just note?
    if (layer() == Layer::Note && !_stepSelection.isPersisted() && _stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    } else {
        trackEngine.setMonitorStep(-1);
    }
}

void ArpSequenceEditPage::drawDetail(Canvas &canvas, const ArpSequence::Step &step) {

    const auto &sequence = _project.selectedArpSequence();
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
        break;
    case Layer::GateProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateProbability() + 1, ArpSequence::GateProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.gateProbability()) / (ArpSequence::GateProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::GateOffset:
        SequencePainter::drawOffset(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateOffset(), ArpSequence::GateOffset::Min - 1, ArpSequence::GateOffset::Max + 1
        );
        str.reset();
        str("%.1f%%", 100.f * step.gateOffset() / float(ArpSequence::GateOffset::Max + 1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Retrigger:
        SequencePainter::drawRetrigger(
            canvas,
            64+ 32 + 8, 32 - 4, 64 - 16, 8,
            step.retrigger() + 1, ArpSequence::Retrigger::Range
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
            step.retriggerProbability() + 1, ArpSequence::RetriggerProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.retriggerProbability()) / (ArpSequence::RetriggerProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Length:
        SequencePainter::drawLength(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, ArpSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.length() + 1.f) / ArpSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationRange:
        SequencePainter::drawLengthRange(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, step.lengthVariationRange(), ArpSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationRange()) / (ArpSequence::Length::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.lengthVariationProbability() + 1, ArpSequence::LengthVariationProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationProbability()) / (ArpSequence::LengthVariationProbability::Range-1));
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
    case Layer::NoteOctave:
        str.reset();
        str("%d", step.noteOctave());
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::NoteOctaveProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.noteOctaveProbability() + 1, ArpSequence::NoteOctaveProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.noteOctaveProbability()) / (ArpSequence::NoteOctaveProbability::Range-1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::NoteVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.noteVariationProbability() + 1, ArpSequence::NoteVariationProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.noteVariationProbability()) / (ArpSequence::NoteVariationProbability::Range -1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Condition:
        str.reset();
        Types::printCondition(str, step.condition(), Types::ConditionFormat::Long);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 96, 32, str);
        break;
    case Layer::Last:
        break;
    }
}

void ArpSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }, doubleClick
    ));
}

void ArpSequenceEditPage::contextAction(int index) {
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

bool ArpSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteArpSequenceSteps();
    default:
        return true;
    }
}

void ArpSequenceEditPage::initSequence() {
    _project.selectedArpSequence().clearSteps();
    showMessage("STEPS INITIALIZED");
}

void ArpSequenceEditPage::copySequence() {
    _model.clipBoard().copyArpSequenceSteps(_project.selectedArpSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void ArpSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteArpSequenceSteps(_project.selectedArpSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void ArpSequenceEditPage::duplicateSequence() {
    _project.selectedArpSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}

void ArpSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show([this] (bool success, Generator::Mode mode) {
        if (success) {
            auto builder = _builderContainer.create<ArpSequenceBuilder>(_project.selectedArpSequence(), layer());

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



void ArpSequenceEditPage::quickEdit(int index) {

    if (index >= 0 && index <= 2) {
        _trackListModel.setTrack(_project.selectedTrack().arpTrack());
        _manager.pages().quickEdit.show(_trackListModel, int(quickEditTrackItems[index]));

    } else {    
        _listModel.setSequence(&_project.selectedArpSequence());
        if (quickEditItems[index] != ArpSequenceListModel::Item::Last) {
            _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
        }
    }
}

bool ArpSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedArpSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex] && !sequence.step(stepIndex).gate()) {
            return false;
        }
    }
    return true;
}

void ArpSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedArpSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(gate);
        }
    }
}
