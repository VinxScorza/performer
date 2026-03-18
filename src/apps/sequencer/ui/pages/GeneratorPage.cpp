#include "GeneratorPage.h"

#include "ui/painters/WindowPainter.h"
#include "ui/LedPainter.h"

#include "engine/generators/Generator.h"
#include "engine/generators/EuclideanGenerator.h"
#include "engine/generators/RandomGenerator.h"

enum class ContextAction {
    Init,
    TogglePreview,
    Regenerate,
    Revert,
    Commit,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
    { "A/B" },
    { "REGEN" },
    { "CANCEL" },
    { "APPLY" },
};

GeneratorPage::GeneratorPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void GeneratorPage::show(Generator *generator, StepSelection<CONFIG_STEP_COUNT> *stepSelection) {
    _generator = generator;
    _stepSelection = stepSelection;
    _applied = false;

    BasePage::show();
}

void GeneratorPage::enter() {
    _valueRange.first = 0;
    _valueRange.second = 7;
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
        functionNames[i] = i < _generator->paramCount() ? _generator->paramName(i) : nullptr;
    }

    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "GENERATOR");
    WindowPainter::drawActiveFunction(canvas, _generator->name());
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    canvas.setFont(Font::Small);
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    auto drawValue = [&] (int index, const char *str) {
        int w = Width / 5;
        int x = (Width * index) / 5;
        int y = Height - 16;
        canvas.drawText(x + (w - canvas.textWidth(str)) / 2, y, str);
    };

    for (int i = 0; i < _generator->paramCount(); ++i) {
        FixedStringBuilder<8> str;
        _generator->printParam(i, str);
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
        _section = std::max(0, _section - 1);
        event.consume();
    }
    if (key.isRight()) {
        _section = std::min(3, _section + 1);
        event.consume();
    }
    

    event.consume();
}

void GeneratorPage::encoder(EncoderEvent &event) {
    bool changed = false;

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
    const auto &pattern = generator.pattern();
    int steps = pattern.size();
    if (_project.selectedTrack().trackMode() == Track::TrackMode::Stochastic || _project.selectedTrack().trackMode() == Track::TrackMode::Arp) {
        steps = 12;
    } 

    int stepWidth = Width / steps;
    int stepHeight = 16;
    int x = (Width - steps * stepWidth) / 2;
    int y = 16;

    for (int i = 0; i < steps; ++i) {
        int h = stepHeight - 2;
        int h2 = (h * pattern[i]) / 255;
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

void GeneratorPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void GeneratorPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        init();
        break;
    case ContextAction::TogglePreview:
        togglePreview();
        break;
    case ContextAction::Regenerate:
        regenerate();
        break;
    case ContextAction::Revert:
        revert();
        break;
    case ContextAction::Commit:
        commit();
        break;
    case ContextAction::Last:
        break;
    }
}

bool GeneratorPage::contextActionEnabled(int index) const {
    if (ContextAction(index) == ContextAction::Regenerate) {
        return _generator->mode() == Generator::Mode::Random;
    }
    return true;
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
        showMessage("PREVIEW");
    }
}

void GeneratorPage::regenerate() {
    if (_generator->mode() == Generator::Mode::Random) {
        auto *random = static_cast<RandomGenerator *>(_generator);
        random->randomizeSeed();
        random->update();
        _generator->showPreview();
    }
}
