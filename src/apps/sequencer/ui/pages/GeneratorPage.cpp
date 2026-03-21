#include "GeneratorPage.h"

#include "ui/painters/WindowPainter.h"
#include "ui/LedPainter.h"

#include "engine/generators/AcidGenerator.h"
#include "engine/generators/Generator.h"
#include "engine/generators/EuclideanGenerator.h"
#include "engine/generators/RandomGenerator.h"

#include <cstdio>

enum class ContextAction {
    RandomizeSeed,
    Init,
    Revert,
    Commit,
    VariationInfo,
    Last
};

GeneratorPage::GeneratorPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

static bool seedDrivenGenerator(Generator::Mode mode) {
    return mode == Generator::Mode::Random || mode == Generator::Mode::Acid;
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
    int steps = generator.pattern().size();
    if (_project.selectedTrack().trackMode() == Track::TrackMode::Stochastic || _project.selectedTrack().trackMode() == Track::TrackMode::Arp) {
        steps = 12;
    } 

    int stepWidth = Width / steps;
    int stepHeight = 16;
    int x = (Width - steps * stepWidth) / 2;
    int y = 16;

    for (int i = 0; i < steps; ++i) {
        int h = stepHeight - 2;
        int h2 = (h * generator.displayValue(i)) / 255;
        if ( i / 16 == _section ) {
            canvas.setColor(Color::Medium);
        } else {
            canvas.setColor(Color::Low);
        }
        canvas.drawRect(x + 1, y + 1, stepWidth - 2, h);
        canvas.setColor(Color::Bright);
        canvas.hline(x + 1, y + 1 + h - h2, stepWidth - 2);
        // canvas.fillRect(x + 1, y + 1 + h - h2 , stepWidth - 2, h2);
        x += stepWidth;
    }
}

void GeneratorPage::drawAcidGenerator(Canvas &canvas, const AcidGenerator &generator) const {
    int steps = CONFIG_STEP_COUNT;
    int stepWidth = Width / steps;
    int stepHeight = 16;
    int x = (Width - steps * stepWidth) / 2;
    int y = 16;

    for (int i = 0; i < steps; ++i) {
        int h = stepHeight - 2;
        int h2 = (h * generator.displayValue(i)) / 255;
        if (i / 16 == _section) {
            canvas.setColor(Color::Medium);
        } else {
            canvas.setColor(Color::Low);
        }
        canvas.drawRect(x + 1, y + 1, stepWidth - 2, h);
        canvas.setColor(Color::Bright);
        canvas.hline(x + 1, y + 1 + h - h2, stepWidth - 2);
        x += stepWidth;
    }
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
    _contextMenuItems[0] = { seedDrivenGenerator(_generator->mode()) ? "NEW SEED" : "NEW RAND" };
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
            random->randomizeSeed();
            random->update();
            _generator->showPreview();
            break;
        }
        case Generator::Mode::Acid: {
            auto *acid = static_cast<AcidGenerator *>(_generator);
            acid->randomizeSeed();
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
