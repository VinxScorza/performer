#include "GeneratorPage.h"

#include "ui/painters/WindowPainter.h"
#include "ui/LedPainter.h"

#include "engine/generators/AcidGenerator.h"
#include "engine/generators/Generator.h"
#include "engine/generators/EuclideanGenerator.h"
#include "engine/generators/RandomGenerator.h"

#include <cstdio>
#include <limits>

enum class ContextAction {
    RandomizeSeed,
    Init,
    Revert,
    Commit,
    VariationInfo,
    Last
};

namespace {
struct PlotArea {
    static constexpr int Top = 15;
    static constexpr int Bottom = 35;
    static constexpr int Height = Bottom - Top + 1;
    static constexpr int BankTop = 38;
    static constexpr int BankHeight = 2;
};

int stepX(int step, int steps) {
    return (CONFIG_LCD_WIDTH * step) / steps;
}

int stepWidth(int step, int steps) {
    return std::max(1, stepX(step + 1, steps) - stepX(step, steps));
}

int stepCenterX(int step, int steps) {
    return stepX(step, steps) + stepWidth(step, steps) / 2;
}

template<typename Predicate>
std::pair<int, int> noteBounds(const NoteSequence &sequence, int steps, Predicate includeStep) {
    int minNote = std::numeric_limits<int>::max();
    int maxNote = std::numeric_limits<int>::min();

    for (int i = 0; i < steps; ++i) {
        const auto &step = sequence.step(i);
        if (includeStep(i, step) && step.gate()) {
            minNote = std::min(minNote, step.note());
            maxNote = std::max(maxNote, step.note());
        }
    }

    if (minNote == std::numeric_limits<int>::max()) {
        minNote = 0;
        maxNote = 0;
    }

    if (minNote == maxNote) {
        --minNote;
        ++maxNote;
    }

    return { minNote, maxNote };
}

int noteY(int note, int minNote, int maxNote, int top, int height) {
    if (maxNote <= minNote) {
        return top + height / 2;
    }
    int span = maxNote - minNote;
    int innerHeight = std::max(1, height - 1);
    int value = top + innerHeight - ((note - minNote) * innerHeight) / span;
    return clamp(value, top, top + innerHeight);
}
}

GeneratorPage::GeneratorPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

static bool seedDrivenGenerator(Generator::Mode mode) {
    return mode == Generator::Mode::Random || mode == Generator::Mode::Acid;
}

static bool acidLayerGenerator(const Generator *generator) {
    return generator->mode() == Generator::Mode::Acid && generator->paramCount() < 5;
}

void GeneratorPage::show(Generator *generator, StepSelection<CONFIG_STEP_COUNT> *stepSelection) {
    _generator = generator;
    _stepSelection = stepSelection;
    _applied = false;

    BasePage::show();
}

void GeneratorPage::enter() {
    _valueRange.first = 0;
    _valueRange.second = 7;

    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note:
        _section = _project.selectedNoteSequence().section();
        break;
    case Track::TrackMode::Curve:
        _section = _project.selectedCurveSequence().section();
        break;
    case Track::TrackMode::Logic:
        _section = _project.selectedLogicSequence().section();
        break;
    case Track::TrackMode::Stochastic:
    case Track::TrackMode::Arp:
        if (_stepSelection && !_stepSelection->none()) {
            _section = _stepSelection->first() / StepCount;
        } else {
            _section = 0;
        }
        break;
    default:
        _section = 0;
        break;
    }

    _generator->randomizeParams();
    _generator->update();
    _generator->showPreview();
}

void GeneratorPage::exit() {
    if (!_applied) {
        _generator->revert();
    }
}

int GeneratorPage::previewStepCount() const {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Stochastic:
    case Track::TrackMode::Arp:
        return 12;
    default:
        return CONFIG_STEP_COUNT;
    }
}

int GeneratorPage::currentStep() const {
    switch (_project.selectedTrack().trackMode()) {
    case Track::TrackMode::Note: {
        const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
        const auto &sequence = _project.selectedNoteSequence();
        return trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    }
    case Track::TrackMode::Curve: {
        const auto &trackEngine = _engine.selectedTrackEngine().as<CurveTrackEngine>();
        const auto &sequence = _project.selectedCurveSequence();
        return trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    }
    case Track::TrackMode::Logic: {
        const auto &trackEngine = _engine.selectedTrackEngine().as<LogicTrackEngine>();
        const auto &sequence = _project.selectedLogicSequence();
        return trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    }
    case Track::TrackMode::Stochastic: {
        const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
        const auto &sequence = _project.selectedStochasticSequence();
        return trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    }
    case Track::TrackMode::Arp: {
        const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
        const auto &sequence = _project.selectedArpSequence();
        return trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    }
    default:
        return -1;
    }
}

bool GeneratorPage::stepInCurrentBank(int step) const {
    return step / StepCount == _section;
}

void GeneratorPage::draw(Canvas &canvas) {
    const char *functionNames[5];
    for (int i = 0; i < 5; ++i) {
        functionNames[i] = nullptr;
    }

    if (seedDrivenGenerator(_generator->mode())) {
        functionNames[0] = "A/B";
        for (int i = 1; i < 5; ++i) {
            functionNames[i] = i < _generator->paramCount() ? _generator->paramName(i) : nullptr;
        }
        if (acidLayerGenerator(_generator)) {
            functionNames[4] = "NEW RAND";
        }
    } else {
        for (int i = 0; i < 5; ++i) {
            functionNames[i] = i < _generator->paramCount() ? _generator->paramName(i) : nullptr;
        }
    }

    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "GENERATOR");
    WindowPainter::drawActiveFunction(canvas, _generator->name());
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    auto drawValue = [&] (int index, const char *str) {
        Font prevFont = canvas.font();
        Font font = (seedDrivenGenerator(_generator->mode()) && index == 0) ? Font::Tiny : Font::Small;
        canvas.setFont(font);

        int w = Width / 5;
        int x = (Width * index) / 5;
        int y = Height - 16;
        canvas.drawText(x + (w - canvas.textWidth(str)) / 2, y, str);

        canvas.setFont(prevFont);
    };

    for (int i = 0; i < _generator->paramCount(); ++i) {
        FixedStringBuilder<16> str;
        if (seedDrivenGenerator(_generator->mode()) && i == 0 && !_generator->showingPreview()) {
            str("ORIGINAL");
        } else {
            _generator->printParam(i, str);
        }
        drawValue(i, str);
    }

    switch (_generator->mode()) {
    case Generator::Mode::InitLayer:
        // no page
        break;
    case Generator::Mode::Euclidean:
        drawEuclideanGenerator(canvas, *static_cast<const EuclideanGenerator *>(_generator));
        break;
    case Generator::Mode::Random:
        drawRandomGenerator(canvas, *static_cast<const RandomGenerator *>(_generator));
        break;
    case Generator::Mode::Acid:
        drawAcidGenerator(canvas, *static_cast<const AcidGenerator *>(_generator));
        break;
    case Generator::Mode::Last:
        break;
    }
}

void GeneratorPage::updateLeds(Leds &leds) {

    int currentStep;
    switch (_project.selectedTrack().trackMode()) {
        case Track::TrackMode::Note: {
                const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
                const auto &sequence = _project.selectedNoteSequence();
                currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
                for (int i = 0; i < 16; ++i) {
                    int stepIndex = stepOffset() + i;
                    bool red = (stepIndex == currentStep) || _stepSelection->at(stepIndex);
                    bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection->at(stepIndex));
                    leds.set(MatrixMap::fromStep(i), red, green);
                }
            }
            break;
        case Track::TrackMode::Curve: {
                const auto &trackEngine = _engine.selectedTrackEngine().as<CurveTrackEngine>();
                const auto &sequence = _project.selectedCurveSequence();
                currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
                for (int i = 0; i < 16; ++i) {
                    int stepIndex = stepOffset() + i;
                    bool red = (stepIndex == currentStep) || _stepSelection->at(stepIndex);
                    bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection->at(stepIndex));
                    leds.set(MatrixMap::fromStep(i), red, green);
                }
            }
            break;
        case Track::TrackMode::Stochastic: {
                const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
                const auto &sequence = _project.selectedStochasticSequence();
                currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
                for (int i = 0; i < 12; ++i) {
                    int stepIndex = stepOffset() + i;
                    bool red = (stepIndex == currentStep) || _stepSelection->at(stepIndex);
                    bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection->at(stepIndex));
                    leds.set(MatrixMap::fromStep(i), red, green);
                }
            }
            break;    
        case Track::TrackMode::Arp: {
                const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
                const auto &sequence = _project.selectedArpSequence();
                currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
                for (int i = 0; i < 12; ++i) {
                    int stepIndex = stepOffset() + i;
                    bool red = (stepIndex == currentStep) || _stepSelection->at(stepIndex);
                    bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection->at(stepIndex));
                    leds.set(MatrixMap::fromStep(i), red, green);
                }
            }
            break;  

        default:
            return;
    }

    LedPainter::drawSelectedSequenceSection(leds, _section);
}

void GeneratorPage::keyDown(KeyEvent &event) {
    _stepSelection->keyDown(event, stepOffset());
}

void GeneratorPage::keyUp(KeyEvent &event) {
    _stepSelection->keyUp(event, stepOffset());
}

void GeneratorPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

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

    if (key.isGlobal()) {
        return;
    }

    if (seedDrivenGenerator(_generator->mode()) && key.isFunction() && key.function() == 0) {
        togglePreview();
        event.consume();
        return;
    }

    if (acidLayerGenerator(_generator) && key.isFunction() && key.function() == 4) {
        contextAction(int(ContextAction::RandomizeSeed));
        event.consume();
        return;
    }

    if (key.isStep() && key.shiftModifier()) {
        _generator->update();
        _generator->showPreview();
        event.consume();
        return;
    }

    if (key.isShift() && event.count() == 2) {
        if (_stepSelection->none()) {
            _stepSelection->selectAll();
            _generator->update();
            _generator->showPreview();
        } else {
            _stepSelection->clear();
            _generator->update();
            _generator->revert();
        }
        
        event.consume();
        return;
    }

    if (key.isLeft()) {
        _section = (_section + 3) % 4;
        event.consume();
    }
    if (key.isRight()) {
        _section = (_section + 1) % 4;
        event.consume();
    }
    

    event.consume();
}

void GeneratorPage::encoder(EncoderEvent &event) {
    bool changed = false;
    bool functionKeyHeld = false;

    for (int i = 0; i < _generator->paramCount(); ++i) {
        if (pageKeyState()[Key::F0 + i]) {
            functionKeyHeld = true;
            break;
        }
    }

    if (seedDrivenGenerator(_generator->mode()) && !functionKeyHeld) {
        if (event.value() != 0) {
            switch (_generator->mode()) {
            case Generator::Mode::Random:
                static_cast<RandomGenerator *>(_generator)->randomizeSeed();
                break;
            case Generator::Mode::Acid:
                static_cast<AcidGenerator *>(_generator)->randomizeSeed();
                break;
            default:
                break;
            }
            changed = true;
        }
    }

    for (int i = 0; i < _generator->paramCount(); ++i) {
        if (pageKeyState()[Key::F0 + i]) {
            _generator->editParam(i, event.value(), event.pressed());
            changed = true;
        }
    }

    if (changed) {
        _generator->update();
        _generator->showPreview();
    }
}

void GeneratorPage::drawEuclideanGenerator(Canvas &canvas, const EuclideanGenerator &generator) const {
    auto steps = generator.steps();
    const auto &pattern = generator.pattern();

    int stepWidth = Width / steps;
    int stepHeight = 8;
    int x = (Width - steps * stepWidth) / 2;
    int y = Height / 2 - stepHeight / 2;

    for (int i = 0; i < steps; ++i) {
        canvas.setColor(Color::Medium);
        canvas.drawRect(x + 1, y + 1, stepWidth - 2, stepHeight - 2);
        if (pattern[i]) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x + 1, y + 1, stepWidth - 2, stepHeight - 2);
        }
        x += stepWidth;
    }
}

void GeneratorPage::drawRandomGenerator(Canvas &canvas, const RandomGenerator &generator) const {
    const int steps = previewStepCount();
    const int baselineY = PlotArea::Top + PlotArea::Height / 2 - 1;
    const int amplitude = std::max(4, PlotArea::Height / 2 - 4);

    canvas.setColor(Color::Low);
    canvas.hline(0, baselineY, Width);

    if (steps > StepCount) {
        for (int bank = 0; bank <= steps / StepCount; ++bank) {
            const int separatorX = stepX(bank * StepCount, steps);
            canvas.setColor(bank == _section || bank == _section + 1 ? Color::Medium : Color::Low);
            canvas.vline(separatorX, PlotArea::Top, PlotArea::Height);
        }
    }

    int previousX = -1;
    int previousY = baselineY;
    for (int i = 0; i < steps; ++i) {
        const int x0 = stepX(i, steps);
        const int x1 = stepX(i + 1, steps);
        const int centerX = stepCenterX(i, steps);
        const int centered = generator.displayValue(i) - 127;
        const int y = baselineY - (centered * amplitude) / 127;
        const bool activeBank = steps <= StepCount || stepInCurrentBank(i);

        canvas.setColor(activeBank ? Color::Medium : Color::Low);
        canvas.vline(centerX, std::min(baselineY, y), std::abs(y - baselineY) + 1);

        canvas.setColor(activeBank ? Color::Bright : Color::Medium);
        canvas.hline(x0 + 1, y, std::max(1, x1 - x0 - 2));

        if (previousX >= 0) {
            canvas.setColor((activeBank || (steps <= StepCount || stepInCurrentBank(i - 1))) ? Color::Medium : Color::Low);
            canvas.line(previousX, previousY, centerX, y);
        }

        previousX = centerX;
        previousY = y;
    }

    if (steps > StepCount) {
        for (int bank = 0; bank < steps / StepCount; ++bank) {
            const int x0 = stepX(bank * StepCount, steps);
            const int x1 = stepX((bank + 1) * StepCount, steps);
            canvas.setColor(bank == _section ? Color::Bright : Color::Low);
            canvas.fillRect(x0 + 1, PlotArea::BankTop, std::max(1, x1 - x0 - 2), PlotArea::BankHeight);
        }
    }

    const int playheadStep = currentStep();
    if (playheadStep >= 0 && playheadStep < steps) {
        const int playX = stepCenterX(playheadStep, steps);
        canvas.setColor(Color::Bright);
        canvas.vline(playX, PlotArea::Top, PlotArea::Bottom - PlotArea::Top + 1);
    }
}

void GeneratorPage::drawAcidGenerator(Canvas &canvas, const AcidGenerator &generator) const {
    const int steps = CONFIG_STEP_COUNT;
    const auto &sequence = generator.sequence(generator.showingPreview());
    const auto playheadStep = currentStep();

    auto drawBankIndicators = [&] () {
        for (int bank = 0; bank < steps / StepCount; ++bank) {
            const int x0 = stepX(bank * StepCount, steps);
            const int x1 = stepX((bank + 1) * StepCount, steps);
            canvas.setColor(bank == _section ? Color::Bright : Color::Low);
            canvas.fillRect(x0 + 1, PlotArea::BankTop, std::max(1, x1 - x0 - 2), PlotArea::BankHeight);
        }
    };

    auto drawBankSeparators = [&] () {
        for (int bank = 0; bank <= steps / StepCount; ++bank) {
            const int separatorX = stepX(bank * StepCount, steps);
            canvas.setColor(bank == _section || bank == _section + 1 ? Color::Medium : Color::Low);
            canvas.vline(separatorX, PlotArea::Top, PlotArea::Height);
        }
    };

    auto drawPlayhead = [&] () {
        if (playheadStep >= 0 && playheadStep < steps) {
            canvas.setColor(Color::Bright);
            canvas.vline(stepCenterX(playheadStep, steps), PlotArea::Top, PlotArea::Bottom - PlotArea::Top + 1);
        }
    };

    if (generator.applyMode() == AcidSequenceBuilder::ApplyMode::Phrase) {
        constexpr int gateTop = PlotArea::Top;
        constexpr int gateHeight = 6;
        constexpr int noteTop = gateTop + gateHeight + 2;
        constexpr int noteHeight = 12;
        constexpr int slideTop = noteTop + noteHeight + 2;
        constexpr int slideHeight = 4;
        const auto bounds = noteBounds(sequence, steps, [&] (int index, const NoteSequence::Step &step) {
            (void)index;
            return step.gate();
        });
        const int minNote = bounds.first;
        const int maxNote = bounds.second;

        drawBankSeparators();

        canvas.setColor(Color::Low);
        canvas.hline(0, gateTop + gateHeight + 1, Width);
        canvas.hline(0, slideTop - 2, Width);

        int previousGateStep = -1;
        int previousGateX = -1;
        int previousGateY = 0;
        for (int i = 0; i < steps; ++i) {
            const auto &step = sequence.step(i);
            const bool activeBank = stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            const int centerX = stepCenterX(i, steps);
            const int width = std::max(1, x1 - x0 - 1);

            if (step.gate()) {
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.fillRect(x0 + 1, gateTop + 1, width, gateHeight - 2);

                const int y = noteY(step.note(), minNote, maxNote, noteTop, noteHeight);
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.hline(x0 + 1, y, width);

                if (previousGateStep >= 0) {
                    const auto &previousStep = sequence.step(previousGateStep);
                    if (step.slide()) {
                        canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                        canvas.line(previousGateX, previousGateY, centerX, y);
                        canvas.fillRect(x0 + 1, slideTop + 1, width, std::max(1, slideHeight - 1));
                    } else {
                        canvas.setColor(activeBank ? Color::Medium : Color::Low);
                        canvas.line(previousGateX, previousGateY, centerX, y);
                    }
                    (void)previousStep;
                }

                previousGateStep = i;
                previousGateX = centerX;
                previousGateY = y;
            }
        }

        drawPlayhead();
        drawBankIndicators();
        return;
    }

    switch (generator.layer()) {
    case NoteSequence::Layer::Gate: {
        constexpr int gateTop = PlotArea::Top + 3;
        constexpr int gateHeight = 12;
        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const auto &step = sequence.step(i);
            const bool activeBank = stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int width = std::max(1, stepWidth(i, steps) - 1);

            canvas.setColor(activeBank ? Color::Medium : Color::Low);
            canvas.drawRect(x0 + 1, gateTop, width, gateHeight);
            if (step.gate()) {
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.fillRect(x0 + 1, gateTop + 1, width, gateHeight - 1);
            }
        }
        break;
    }
    case NoteSequence::Layer::Note: {
        const auto bounds = noteBounds(sequence, steps, [&] (int index, const NoteSequence::Step &step) {
            (void)index;
            return step.gate();
        });
        const int minNote = bounds.first;
        const int maxNote = bounds.second;
        int previousStep = -1;
        int previousY = 0;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const auto &step = sequence.step(i);
            if (!step.gate()) {
                previousStep = -1;
                continue;
            }

            const bool activeBank = stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            const int y = noteY(step.note(), minNote, maxNote, PlotArea::Top + 2, PlotArea::Height - 5);
            canvas.setColor(activeBank ? Color::Bright : Color::Medium);
            canvas.hline(x0 + 1, y, std::max(1, x1 - x0 - 1));

            if (previousStep >= 0) {
                canvas.setColor((activeBank || stepInCurrentBank(previousStep)) ? Color::Medium : Color::Low);
                canvas.vline(x0, std::min(previousY, y), std::abs(y - previousY) + 1);
            }

            previousStep = i;
            previousY = y;
        }
        break;
    }
    case NoteSequence::Layer::Slide: {
        const auto bounds = noteBounds(sequence, steps, [&] (int index, const NoteSequence::Step &step) {
            (void)index;
            return step.gate();
        });
        const int minNote = bounds.first;
        const int maxNote = bounds.second;
        int previousGateStep = -1;
        int previousGateX = -1;
        int previousGateY = 0;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const auto &step = sequence.step(i);
            if (!step.gate()) {
                continue;
            }

            const bool activeBank = stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            const int centerX = stepCenterX(i, steps);
            const int y = noteY(step.note(), minNote, maxNote, PlotArea::Top + 2, PlotArea::Height - 7);

            canvas.setColor(activeBank ? Color::Medium : Color::Low);
            canvas.hline(x0 + 1, y, std::max(1, x1 - x0 - 1));

            if (previousGateStep >= 0) {
                if (step.slide()) {
                    canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                    canvas.line(previousGateX, previousGateY, centerX, y);
                    canvas.fillRect(x0 + 1, PlotArea::Bottom - 4, std::max(1, x1 - x0 - 2), 3);
                } else {
                    canvas.setColor(activeBank ? Color::Medium : Color::Low);
                    canvas.line(previousGateX, previousGateY, centerX, y);
                }
            }

            previousGateStep = i;
            previousGateX = centerX;
            previousGateY = y;
        }
        break;
    }
    default:
        drawBankSeparators();
        break;
    }

    drawPlayhead();
    drawBankIndicators();
}

int GeneratorPage::contextItemCount() const {
    switch (_generator->mode()) {
    case Generator::Mode::Random:
    case Generator::Mode::Acid:
        return int(ContextAction::Last);
    case Generator::Mode::Euclidean:
        return 4;
    default:
        return 4;
    }
}

void GeneratorPage::contextShow(bool doubleClick) {
    switch (_generator->mode()) {
    case Generator::Mode::Random:
    case Generator::Mode::Acid:
    case Generator::Mode::Euclidean:
        _contextMenuItems[0] = { "NEW RAND" };
        break;
    default:
        _contextMenuItems[0] = { "NEW SEED" };
        break;
    }
    _contextMenuItems[1] = { "INIT" };
    _contextMenuItems[2] = { "CANCEL" };
    _contextMenuItems[3] = { "APPLY" };

    switch (_generator->mode()) {
    case Generator::Mode::Random: {
        const auto *random = static_cast<const RandomGenerator *>(_generator);
        std::snprintf(_variationMenuLabel, sizeof(_variationMenuLabel), "VAR %d%%", random->variation());
        break;
    }
    case Generator::Mode::Acid: {
        const auto *acid = static_cast<const AcidGenerator *>(_generator);
        std::snprintf(_variationMenuLabel, sizeof(_variationMenuLabel), "VAR %d%%", acid->variation());
        break;
    }
    default:
        std::snprintf(_variationMenuLabel, sizeof(_variationMenuLabel), "VAR");
        break;
    }
    _contextMenuItems[4] = { _variationMenuLabel };

    showContextMenu(ContextMenu(
        _contextMenuItems,
        contextItemCount(),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void GeneratorPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::RandomizeSeed:
        switch (_generator->mode()) {
        case Generator::Mode::Random: {
            auto *random = static_cast<RandomGenerator *>(_generator);
            random->randomizeContextParams();
            random->update();
            _generator->showPreview();
            break;
        }
        case Generator::Mode::Acid: {
            auto *acid = static_cast<AcidGenerator *>(_generator);
            acid->randomizeContextParams();
            acid->update();
            _generator->showPreview();
            break;
        }
        case Generator::Mode::Euclidean:
            _generator->randomizeParams();
            _generator->update();
            _generator->showPreview();
            break;
        default:
            break;
        }
        break;
    case ContextAction::Init:
        init();
        break;
    case ContextAction::Revert:
        revert();
        break;
    case ContextAction::Commit:
        commit();
        break;
    case ContextAction::VariationInfo:
    case ContextAction::Last:
        break;
    }
}

bool GeneratorPage::contextActionEnabled(int index) const {
    if (index >= contextItemCount()) {
        return false;
    }
    return ContextAction(index) != ContextAction::VariationInfo;
}

void GeneratorPage::init() {
    _stepSelection->clear();
    _generator->init();
    _generator->showPreview();
}

void GeneratorPage::revert() {
    _stepSelection->clear();
    _generator->revert();
    _applied = false;
    close();
}

void GeneratorPage::commit() {
    _stepSelection->clear();
    _generator->apply();
    _applied = true;
    close();
}

void GeneratorPage::togglePreview() {
    if (_generator->showingPreview()) {
        _generator->showOriginal();
        showMessage("ORIGINAL");
    } else {
        _generator->showPreview();
        showMessage(seedDrivenGenerator(_generator->mode()) ? "CURRENT SEED" : "PREVIEW");
    }
}
