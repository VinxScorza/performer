#include "GeneratorPage.h"

#include "Pages.h"
#include "ui/painters/WindowPainter.h"
#include "ui/LedPainter.h"

#include "engine/generators/AcidGenerator.h"
#include "engine/generators/ChaosGenerator.h"
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
namespace PlotArea {
constexpr int Top = 15;
constexpr int Bottom = 35;
constexpr int Height = Bottom - Top + 1;
constexpr int BankTop = 38;
}

int stepX(int step, int steps) {
    return (CONFIG_LCD_WIDTH * step) / steps;
}

int stepWidth(int step, int steps) {
    return std::max(1, stepX(step + 1, steps) - stepX(step, steps));
}

int stepCenterX(int step, int steps) {
    return stepX(step, steps) + stepWidth(step, steps) / 2;
}

int chaosScanIndexFromCell(int cell) {
    constexpr int columns = 4;
    constexpr int rows = 4;
    return (cell % columns) * rows + (cell / columns);
}

int chaosCellFromScanIndex(int index) {
    constexpr int columns = 4;
    constexpr int rows = 4;
    return (index % rows) * columns + (index / rows);
}

constexpr int ChaosAllOnCell = 11;
constexpr int ChaosAllOffCell = 15;

bool chaosCellTarget(int cell, ChaosGenerator::Target &target) {
    switch (cell) {
    case 0:  target = ChaosGenerator::Target::Gate; return true;
    case 1:  target = ChaosGenerator::Target::Length; return true;
    case 2:  target = ChaosGenerator::Target::Note; return true;
    case 3:  target = ChaosGenerator::Target::BypassScale; return true;
    case 4:  target = ChaosGenerator::Target::GateOffset; return true;
    case 5:  target = ChaosGenerator::Target::LengthVariationRange; return true;
    case 6:  target = ChaosGenerator::Target::Slide; return true;
    case 7:  target = ChaosGenerator::Target::Condition; return true;
    case 8:  target = ChaosGenerator::Target::GateProbability; return true;
    case 9:  target = ChaosGenerator::Target::LengthVariationProbability; return true;
    case 10: target = ChaosGenerator::Target::NoteVariationRange; return true;
    case 12: target = ChaosGenerator::Target::Retrigger; return true;
    case 13: target = ChaosGenerator::Target::RetriggerProbability; return true;
    case 14: target = ChaosGenerator::Target::NoteVariationProbability; return true;
    default: break;
    }
    return false;
}

const char *chaosCellLabel(int cell) {
    if (cell == ChaosAllOnCell) {
        return "All On";
    }
    if (cell == ChaosAllOffCell) {
        return "All Off";
    }

    ChaosGenerator::Target target;
    return chaosCellTarget(cell, target) ? ChaosGenerator::targetCellLabel(target) : "";
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

void drawStairStep(Canvas &canvas, int x0, int x1, int y, Color color) {
    canvas.setColor(color);
    canvas.hline(x0 + 1, y, std::max(1, x1 - x0 - 1));
}
}

GeneratorPage::GeneratorPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

static bool seedDrivenGenerator(Generator::Mode mode) {
    return mode == Generator::Mode::Random || mode == Generator::Mode::Acid;
}

static bool abPreviewGenerator(Generator::Mode mode) {
    return seedDrivenGenerator(mode) || mode == Generator::Mode::Euclidean;
}

static bool chaosGeneratorMode(Generator::Mode mode) {
    return mode == Generator::Mode::Chaos;
}

static bool euclideanGeneratorMode(Generator::Mode mode) {
    return mode == Generator::Mode::Euclidean;
}

static bool acidLayerGenerator(const Generator *generator) {
    return generator->mode() == Generator::Mode::Acid && generator->paramCount() < 5;
}

void GeneratorPage::show(Generator *generator, StepSelection<CONFIG_STEP_COUNT> *stepSelection) {
    _generator = generator;
    _stepSelection = stepSelection;
    _applied = false;
    _boundTrackIndex = _project.selectedTrackIndex();
    _boundTrackMode = _project.selectedTrack().trackMode();

    BasePage::show();
}

bool GeneratorPage::boundTrackContextValid() const {
    return _project.selectedTrackIndex() == _boundTrackIndex &&
           _project.selectedTrack().trackMode() == _boundTrackMode;
}

bool GeneratorPage::ensureBoundTrackContext() {
    if (boundTrackContextValid()) {
        return true;
    }

    revert();
    showMessage("GEN CANCELED");
    return false;
}

void GeneratorPage::enter() {
    _valueRange.first = 0;
    _valueRange.second = 7;
    _chaosCursor = 0;
    _chaosPreviewArmed = false;
    _launchpadResetState = false;

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

    if (chaosGeneratorMode(_generator->mode())) {
        _launchpadResetState = true;
        _generator->showOriginal();
        return;
    }

    _generator->randomizeParams();
    _generator->update();
    _generator->showPreview();
    _launchpadResetState = false;
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
    FixedStringBuilder<24> activeFunction;
    switch (_generator->mode()) {
    case Generator::Mode::Acid: {
        const auto &acid = *static_cast<const AcidGenerator *>(_generator);
        activeFunction(acid.applyMode() == AcidSequenceBuilder::ApplyMode::Phrase ? "ACID PHRASE" : "ACID LAYER");
        break;
    }
    case Generator::Mode::Chaos: {
        const auto &chaos = *static_cast<const ChaosGenerator *>(_generator);
        activeFunction(chaos.patternScope() ? "WRECK PAT" : "VNDLZ SEQ");
        break;
    }
    default:
        activeFunction(_generator->name());
        break;
    }

    const char *functionNames[5];
    for (int i = 0; i < 5; ++i) {
        functionNames[i] = nullptr;
    }

    if (chaosGeneratorMode(_generator->mode())) {
        functionNames[0] = "A/B";
        functionNames[1] = "AMT";
        functionNames[2] = nullptr;
        functionNames[3] = "CANCEL";
        functionNames[4] = "APPLY";
    } else if (seedDrivenGenerator(_generator->mode())) {
        functionNames[0] = "A/B";
        for (int i = 1; i < 5; ++i) {
            functionNames[i] = i < _generator->paramCount() ? _generator->paramName(i) : nullptr;
        }
        if (acidLayerGenerator(_generator)) {
            functionNames[4] = "NEW RAND";
        }
    } else if (euclideanGeneratorMode(_generator->mode())) {
        functionNames[0] = "A/B";
        functionNames[1] = "NEW EUCL";
        functionNames[2] = _generator->paramName(0);
        functionNames[3] = _generator->paramName(1);
        functionNames[4] = _generator->paramName(2);
    } else {
        for (int i = 0; i < 5; ++i) {
            functionNames[i] = i < _generator->paramCount() ? _generator->paramName(i) : nullptr;
        }
    }

    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "GENERATOR");
    WindowPainter::drawActiveFunction(canvas, activeFunction);
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());
    if (chaosGeneratorMode(_generator->mode())) {
        const int x0 = (Width * 2) / 5;
        const int x1 = (Width * 3) / 5;
        canvas.setFont(Font::Small);
        canvas.setBlendMode(BlendMode::Set);
        if (pageKeyState()[Key::F2]) {
            canvas.setColor(Color::None);
            canvas.fillRect(x0, Height - 10, x1 - x0 + 1, 10);
            canvas.setColor(Color::Bright);
        } else {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x0, Height - 10, x1 - x0 + 1, 10);
            canvas.setBlendMode(BlendMode::Sub);
            canvas.setColor(Color::Bright);
        }
        canvas.drawText(x0 + ((x1 - x0 + 1) - canvas.textWidth("CHAOS")) / 2, Height - 2, "CHAOS");
        canvas.setBlendMode(BlendMode::Set);
    }

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    auto drawValue = [&] (int index, const char *str) {
        Font prevFont = canvas.font();
        Color color = Color::Bright;
        Font font = Font::Small;
        if ((seedDrivenGenerator(_generator->mode()) || chaosGeneratorMode(_generator->mode()) || euclideanGeneratorMode(_generator->mode())) && index == 0) {
            font = Font::Tiny;
            if (chaosGeneratorMode(_generator->mode())) {
                color = Color::Medium;
            }
        } else if (chaosGeneratorMode(_generator->mode()) && index == 1) {
            font = Font::Tiny;
            color = Color::Medium;
        }
        canvas.setFont(font);
        canvas.setColor(color);

        int w = Width / 5;
        int x = (Width * index) / 5;
        int y = Height - 16;
        canvas.drawText(x + (w - canvas.textWidth(str)) / 2, y, str);

        canvas.setColor(Color::Bright);
        canvas.setFont(prevFont);
    };

    if (chaosGeneratorMode(_generator->mode())) {
        const auto &generator = *static_cast<const ChaosGenerator *>(_generator);
        Font prevFont = canvas.font();
        FixedStringBuilder<16> seedStr;
        if (!_generator->showingPreview()) {
            seedStr("ORIGINAL");
        } else {
            seedStr("%08X", generator.seed());
        }
        canvas.setFont(Font::Tiny);
        const int amtX0 = Width / 5;
        const int amtX1 = (Width * 2) / 5;
        const int chaosX0 = (Width * 2) / 5;
        const int chaosX1 = (Width * 3) / 5;
        const int y = Height - 13;

        FixedStringBuilder<16> amountStr("%d%%", generator.amount());

        canvas.setColor(Color::Medium);
        canvas.drawText(chaosX0 + ((chaosX1 - chaosX0 + 1) - canvas.textWidth(seedStr)) / 2, y, seedStr);

        canvas.setColor(pageKeyState()[Key::F1] ? Color::Bright : Color::Medium);
        canvas.drawText(amtX0 + ((amtX1 - amtX0 + 1) - canvas.textWidth(amountStr)) / 2, y, amountStr);
        canvas.setColor(Color::Bright);
        canvas.setFont(prevFont);
    } else {
        if (euclideanGeneratorMode(_generator->mode())) {
            drawValue(0, _generator->showingPreview() ? "CURRENT" : "ORIGINAL");
        }
        for (int i = 0; i < _generator->paramCount(); ++i) {
            FixedStringBuilder<16> str;
            if (seedDrivenGenerator(_generator->mode()) && i == 0 && !_generator->showingPreview()) {
                str("ORIGINAL");
            } else {
                _generator->printParam(i, str);
            }
            drawValue(euclideanGeneratorMode(_generator->mode()) ? i + 2 : i, str);
        }
    }

    switch (_generator->mode()) {
    case Generator::Mode::InitLayer:
    case Generator::Mode::InitSteps:
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
    case Generator::Mode::Chaos:
        drawChaosGenerator(canvas, *static_cast<const ChaosGenerator *>(_generator));
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
    auto leaveWithCancel = [&] () {
        revert();
    };

    if (!ensureBoundTrackContext()) {
        event.consume();
        return;
    }

    if (key.isContextMenu()) {
        if (chaosGeneratorMode(_generator->mode())) {
            event.consume();
            return;
        }
        contextShow();
        event.consume();
        return;
    }


    if (key.pageModifier() && event.count() == 2) {
        if (chaosGeneratorMode(_generator->mode())) {
            event.consume();
            return;
        }
        contextShow(true);
        event.consume();
        return;
    }

    if (key.isPlay()) {
        if (key.pageModifier()) {
            _engine.toggleRecording();
        } else {
            _engine.togglePlay(key.shiftModifier());
        }
        event.consume();
        return;
    }

    if (key.isTempo()) {
        event.consume();
        return;
    }

    if (chaosGeneratorMode(_generator->mode())) {
        auto *chaos = static_cast<ChaosGenerator *>(_generator);
        auto refreshChaosView = [&] () {
            if (_chaosPreviewArmed && _generator->showingPreview()) {
                _generator->showPreview();
            }
        };

        if (key.isFunction()) {
            switch (key.function()) {
            case 0:
                togglePreview();
                event.consume();
                return;
            case 2:
                {
                bool wasShowingPreview = _generator->showingPreview();
                _chaosPreviewArmed = true;
                chaos->randomizeSeed();
                chaos->update();
                chaos->showPreview();
                _launchpadResetState = false;
                if (!wasShowingPreview) {
                    showPreviewStateMessage();
                }
                event.consume();
                return;
                }
            case 3:
                revert();
                event.consume();
                return;
            case 4:
                commit();
                event.consume();
                return;
            default:
                break;
            }
        }

        if (key.isEncoder()) {
            if (_chaosCursor == ChaosAllOnCell) {
                chaos->setAllTargets(true);
                chaos->update();
                refreshChaosView();
                _launchpadResetState = false;
            } else if (_chaosCursor == ChaosAllOffCell) {
                chaos->setAllTargets(false);
                chaos->update();
                refreshChaosView();
                _launchpadResetState = false;
            } else {
                ChaosGenerator::Target target;
                if (chaosCellTarget(_chaosCursor, target)) {
                    chaos->toggleTarget(target);
                    chaos->update();
                    refreshChaosView();
                    _launchpadResetState = false;
                }
            }
            event.consume();
            return;
        }

    }

    if ((seedDrivenGenerator(_generator->mode()) || euclideanGeneratorMode(_generator->mode())) && key.isEncoder()) {
        commit();
        event.consume();
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

    if (euclideanGeneratorMode(_generator->mode()) && key.isFunction()) {
        switch (key.function()) {
        case 0:
            togglePreview();
            event.consume();
            return;
        case 1:
            _generator->randomizeParams();
            _generator->update();
            _generator->showPreview();
            event.consume();
            return;
        default:
            break;
        }
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
        return;
    }

    auto functionKeyInContext = [&] () {
        if (!key.isFunction()) {
            return false;
        }

        if (chaosGeneratorMode(_generator->mode())) {
            return key.function() >= 0 && key.function() <= 4;
        }

        if (seedDrivenGenerator(_generator->mode())) {
            return key.function() == 0 ||
                   (key.function() > 0 && key.function() < _generator->paramCount()) ||
                   (acidLayerGenerator(_generator) && key.function() == 4);
        }

        if (euclideanGeneratorMode(_generator->mode())) {
            return key.function() == 0 || key.function() == 1 ||
                   key.function() == 2 || key.function() == 3 || key.function() == 4;
        }

        return false;
    };

    bool contextKey =
        key.isStep() ||
        key.isEncoder() ||
        key.isLeft() ||
        key.isRight() ||
        key.isTempo() ||
        key.isPage() ||
        key.isShift() ||
        key.isContextMenu() ||
        functionKeyInContext() ||
        (key.isShift() && event.count() == 2) ||
        (key.isStep() && key.shiftModifier());

    if (!contextKey) {
        leaveWithCancel();
        return;
    }

    event.consume();
}

void GeneratorPage::encoder(EncoderEvent &event) {
    if (!ensureBoundTrackContext()) {
        return;
    }

    if (chaosGeneratorMode(_generator->mode())) {
        auto *chaos = static_cast<ChaosGenerator *>(_generator);

        if (pageKeyState()[Key::F1]) {
            chaos->setAmount(chaos->amount() + event.value());
            chaos->update();
            if (_generator->showingPreview()) {
                chaos->showPreview();
            }
            if (event.value() != 0) {
                _launchpadResetState = false;
            }
            return;
        }

        if (event.value() != 0) {
            constexpr int chaosCellCount = 16;
            int scanIndex = chaosScanIndexFromCell(_chaosCursor);
            scanIndex = (scanIndex + event.value()) % chaosCellCount;
            if (scanIndex < 0) {
                scanIndex += chaosCellCount;
            }
            _chaosCursor = chaosCellFromScanIndex(scanIndex);
        }
        return;
    }

    bool changed = false;
    bool functionKeyHeld = false;

    for (int i = 0; i < _generator->paramCount(); ++i) {
        int functionIndex = euclideanGeneratorMode(_generator->mode()) ? i + 2 : i;
        if (pageKeyState()[Key::F0 + functionIndex]) {
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

    if (euclideanGeneratorMode(_generator->mode()) && !functionKeyHeld) {
        if (event.value() != 0) {
            _generator->randomizeParams();
            changed = true;
        }
    }

    for (int i = 0; i < _generator->paramCount(); ++i) {
        int functionIndex = euclideanGeneratorMode(_generator->mode()) ? i + 2 : i;
        if (pageKeyState()[Key::F0 + functionIndex]) {
            _generator->editParam(i, event.value(), event.pressed());
            changed = true;
        }
    }

    if (changed) {
        _launchpadResetState = false;
        _generator->update();
        if (!abPreviewGenerator(_generator->mode()) || _generator->showingPreview()) {
            _generator->showPreview();
        }
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

    auto drawBankSeparators = [&] () {
        for (int bank = 0; bank <= steps / StepCount; ++bank) {
            const int separatorX = stepX(bank * StepCount, steps);
            canvas.setColor(bank == _section || bank == _section + 1 ? Color::Medium : Color::MediumLow);
            canvas.vline(separatorX, PlotArea::Top, PlotArea::BankTop - PlotArea::Top + 1);
        }
    };

    auto drawBankFrame = [&] () {
        if (steps <= StepCount) {
            return;
        }
        const int x0 = stepX(_section * StepCount, steps);
        const int x1 = stepX((_section + 1) * StepCount, steps);
        canvas.setColor(Color::Medium);
        canvas.hline(x0 + 1, PlotArea::Top, std::max(1, x1 - x0 - 1));
        canvas.hline(x0 + 1, PlotArea::BankTop, std::max(1, x1 - x0 - 1));
    };

    auto drawPlayhead = [&] () {
        const int playheadStep = currentStep();
        if (playheadStep >= 0 && playheadStep < steps) {
            const int playX = stepCenterX(playheadStep, steps);
            canvas.setColor(Color::MediumBright);
            canvas.vline(playX, PlotArea::Top, PlotArea::BankTop - PlotArea::Top + 1);
        }
    };

    auto drawProfile = [&] (bool centered, bool highlightMarkers) {
        if (centered) {
            canvas.setColor(Color::MediumLow);
            canvas.hline(0, baselineY, Width);
        }

        drawBankSeparators();

        int previousX = -1;
        int previousY = baselineY;
        for (int i = 0; i < steps; ++i) {
            const int centerX = stepCenterX(i, steps);
            const int rawValue = generator.displayValue(i);
            const int y = centered ?
                baselineY - ((rawValue - 127) * amplitude) / 127 :
                PlotArea::Bottom - (rawValue * (PlotArea::Height - 2)) / 255;
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);

            if (previousX >= 0) {
                canvas.setColor((activeBank || (steps <= StepCount || stepInCurrentBank(i - 1))) ? Color::Medium : Color::MediumLow);
                canvas.line(previousX, previousY, centerX, y);
            } else {
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.fillRect(centerX, y, 1, 1);
            }

            if (highlightMarkers && activeBank) {
                canvas.setColor(Color::Bright);
                canvas.fillRect(std::max(0, centerX - 1), std::max(PlotArea::Top, y - 1), 3, 3);
            } else if (highlightMarkers) {
                canvas.setColor(Color::Medium);
                canvas.fillRect(centerX, y, 1, 1);
            }

            previousX = centerX;
            previousY = y;
        }
    };

    auto drawGateBlocks = [&] () {
        constexpr int gateTop = PlotArea::Top + 3;
        constexpr int gateHeight = 12;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int width = std::max(1, stepWidth(i, steps) - 1);

            canvas.setColor(activeBank ? Color::Medium : Color::MediumLow);
            canvas.drawRect(x0 + 1, gateTop, width, gateHeight);
            if (generator.displayValue(i) >= 128) {
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.fillRect(x0 + 1, gateTop + 1, width, gateHeight - 1);
            }
        }
    };

    auto drawNoteContour = [&] () {
        int minValue = 255;
        int maxValue = 0;
        for (int i = 0; i < steps; ++i) {
            const int value = generator.displayValue(i);
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
        if (minValue == maxValue) {
            minValue = std::max(0, minValue - 1);
            maxValue = std::min(255, maxValue + 1);
        }

        int previousStep = -1;
        int previousY = 0;
        drawBankSeparators();

        for (int i = 0; i < steps; ++i) {
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            const int y = noteY(generator.displayValue(i), minValue, maxValue, PlotArea::Top + 2, PlotArea::Height - 5);
            drawStairStep(canvas, x0, x1, y, activeBank ? Color::Bright : Color::Medium);

            if (previousStep >= 0) {
                canvas.setColor((activeBank || stepInCurrentBank(previousStep)) ? Color::Medium : Color::MediumLow);
                canvas.vline(x0, std::min(previousY, y), std::abs(y - previousY) + 1);
            }

            previousStep = i;
            previousY = y;
        }
    };

    auto drawSlideMarkers = [&] () {
        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            if (generator.displayValue(i) < 128) {
                continue;
            }
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            canvas.setColor(activeBank ? Color::Bright : Color::Medium);
            canvas.fillRect(x0 + 1, PlotArea::Bottom - 4, std::max(1, x1 - x0 - 2), 3);
        }
    };

    auto drawBooleanMarkers = [&] () {
        constexpr int markerY = PlotArea::Top + PlotArea::Height / 2 - 1;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            if (generator.displayValue(i) < 128) {
                continue;
            }
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int centerX = stepCenterX(i, steps);
            canvas.setColor(activeBank ? Color::Bright : Color::Medium);
            canvas.fillRect(std::max(0, centerX - 1), markerY - 1, 3, 3);
        }
    };

    auto drawLengthBars = [&] () {
        constexpr int barTop = PlotArea::Top + 6;
        constexpr int barHeight = 8;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int cellWidth = std::max(2, stepWidth(i, steps) - 1);
            const int barWidth = std::max(1, (cellWidth * generator.displayValue(i)) / 255);
            canvas.setColor(activeBank ? Color::Medium : Color::MediumLow);
            canvas.drawRect(x0 + 1, barTop, cellWidth, barHeight);
            canvas.setColor(activeBank ? Color::Bright : Color::Medium);
            canvas.fillRect(x0 + 1, barTop + 1, barWidth, barHeight - 1);
        }
    };

    auto drawRepeatStripes = [&] () {
        constexpr int stripeTop = PlotArea::Top + 4;
        constexpr int stripeHeight = 12;

        drawBankSeparators();
        for (int i = 0; i < steps; ++i) {
            const bool activeBank = steps <= StepCount || stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int width = std::max(2, stepWidth(i, steps) - 2);
            const int stripes = std::max(0, (generator.displayValue(i) * 4 + 127) / 255);

            canvas.setColor(activeBank ? Color::Medium : Color::MediumLow);
            canvas.drawRect(x0 + 1, stripeTop, width, stripeHeight);
            canvas.setColor(activeBank ? Color::Bright : Color::Medium);
            for (int stripe = 0; stripe < stripes; ++stripe) {
                const int stripeX = x0 + 2 + (stripe * width) / std::max(1, stripes);
                canvas.vline(stripeX, stripeTop + 1, stripeHeight - 1);
            }
        }
    };

    bool drewSpecialized = false;
    if (_project.selectedTrack().trackMode() == Track::TrackMode::Note) {
        switch (_project.selectedNoteSequenceLayer()) {
        case NoteSequence::Layer::Gate:
            drawGateBlocks();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::Note:
            drawNoteContour();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::Slide:
            drawSlideMarkers();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::BypassScale:
            drawBooleanMarkers();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::Length:
            drawLengthBars();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::Retrigger:
        case NoteSequence::Layer::StageRepeats:
            drawRepeatStripes();
            drewSpecialized = true;
            break;
        case NoteSequence::Layer::GateOffset:
            drawProfile(true, true);
            drewSpecialized = true;
            break;
        default:
            break;
        }
    }

    if (!drewSpecialized) {
        drawProfile(false, true);
    }

    drawBankFrame();
    drawPlayhead();
}

void GeneratorPage::drawAcidGenerator(Canvas &canvas, const AcidGenerator &generator) const {
    const int steps = CONFIG_STEP_COUNT;
    const auto &sequence = generator.sequence(generator.showingPreview());
    const auto playheadStep = currentStep();

    auto drawBankIndicators = [&] () {
        const int x0 = stepX(_section * StepCount, steps);
        const int x1 = stepX((_section + 1) * StepCount, steps);
        canvas.setColor(Color::Medium);
        canvas.hline(x0 + 1, PlotArea::Top, std::max(1, x1 - x0 - 1));
        canvas.hline(x0 + 1, PlotArea::BankTop, std::max(1, x1 - x0 - 1));
    };

    auto drawBankSeparators = [&] () {
        for (int bank = 0; bank <= steps / StepCount; ++bank) {
            const int separatorX = stepX(bank * StepCount, steps);
            canvas.setColor(bank == _section || bank == _section + 1 ? Color::Medium : Color::MediumLow);
            canvas.vline(separatorX, PlotArea::Top, PlotArea::BankTop - PlotArea::Top + 1);
        }
    };

    auto drawPlayhead = [&] () {
        if (playheadStep >= 0 && playheadStep < steps) {
            const int playX = stepCenterX(playheadStep, steps);
            canvas.setColor(Color::MediumBright);
            canvas.vline(playX, PlotArea::Top, PlotArea::BankTop - PlotArea::Top + 1);
        }
    };

    if (generator.applyMode() == AcidSequenceBuilder::ApplyMode::Phrase) {
        constexpr int gateTop = PlotArea::Top;
        constexpr int gateHeight = 6;
        constexpr int noteTop = gateTop + gateHeight + 2;
        constexpr int noteHeight = 10;
        constexpr int slideTop = 35;
        constexpr int slideHeight = 2;
        const auto bounds = noteBounds(sequence, steps, [&] (int index, const NoteSequence::Step &step) {
            (void)index;
            return step.gate();
        });
        const int minNote = bounds.first;
        const int maxNote = bounds.second;

        drawBankSeparators();

        canvas.setColor(Color::MediumLow);
        canvas.hline(0, gateTop + gateHeight + 1, Width);
        canvas.hline(0, slideTop - 2, Width);

        int previousGateStep = -1;
        int previousGateY = 0;
        for (int i = 0; i < steps; ++i) {
            const auto &step = sequence.step(i);
            const bool activeBank = stepInCurrentBank(i);
            const int x0 = stepX(i, steps);
            const int x1 = stepX(i + 1, steps);
            const int width = std::max(1, x1 - x0 - 1);

            if (step.gate()) {
                canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                canvas.fillRect(x0 + 1, gateTop + 1, width, gateHeight - 2);

                const int y = noteY(step.note(), minNote, maxNote, noteTop, noteHeight);
                drawStairStep(canvas, x0, x1, y, activeBank ? Color::Bright : Color::Medium);

                if (previousGateStep >= 0) {
                    if (step.slide()) {
                        canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                        const int underscoreWidth = std::max(2, width - 2);
                        const int underscoreX = x0 + 1 + std::max(0, (width - underscoreWidth) / 2);
                        canvas.fillRect(underscoreX, slideTop + 1, underscoreWidth, std::max(1, slideHeight - 1));
                    }
                    canvas.setColor((activeBank || stepInCurrentBank(previousGateStep)) ? Color::Medium : Color::MediumLow);
                    canvas.vline(x0, std::min(previousGateY, y), std::abs(y - previousGateY) + 1);
                }

                previousGateStep = i;
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

            canvas.setColor(activeBank ? Color::Medium : Color::MediumLow);
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
            drawStairStep(canvas, x0, x1, y, activeBank ? Color::Bright : Color::Medium);

            if (previousStep >= 0) {
                canvas.setColor((activeBank || stepInCurrentBank(previousStep)) ? Color::Medium : Color::MediumLow);
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
            const int y = noteY(step.note(), minNote, maxNote, PlotArea::Top + 2, PlotArea::Height - 7);

            drawStairStep(canvas, x0, x1, y, activeBank ? Color::Medium : Color::MediumLow);

            if (previousGateStep >= 0) {
                if (step.slide()) {
                    canvas.setColor(activeBank ? Color::Bright : Color::Medium);
                    canvas.fillRect(x0 + 1, PlotArea::Bottom - 4, std::max(1, x1 - x0 - 2), 3);
                }
                canvas.setColor((activeBank || stepInCurrentBank(previousGateStep)) ? Color::Medium : Color::MediumLow);
                canvas.vline(x0, std::min(previousGateY, y), std::abs(y - previousGateY) + 1);
            }

            previousGateStep = i;
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

void GeneratorPage::drawChaosGenerator(Canvas &canvas, const ChaosGenerator &generator) const {
    constexpr int columns = 4;
    constexpr int rows = 4;
    constexpr int gridTop = 11;
    constexpr int gridBottom = 46;
    constexpr int cellGap = 2;
    const int cellWidth = (Width - (columns + 1) * cellGap) / columns;
    const int cellHeight = (gridBottom - gridTop - (rows - 1) * cellGap) / rows;

    auto drawCell = [&] (int index, const char *label, bool enabled, bool selected, bool actionCell) {
        int row = index / columns;
        int col = index % columns;
        int x = cellGap + col * (cellWidth + cellGap);
        int y = gridTop + row * (cellHeight + cellGap);
        const auto prevBlendMode = canvas.blendMode();

        if (enabled && !actionCell) {
            canvas.setColor(selected ? Color::MediumBright : Color::Medium);
            canvas.fillRect(x, y, cellWidth, cellHeight);
            canvas.setColor(selected ? Color::Bright : Color::Medium);
            canvas.drawRect(x, y, cellWidth, cellHeight);
        } else {
            canvas.setColor(selected ? Color::Medium : Color::Low);
            canvas.drawRect(x, y, cellWidth, cellHeight);
            canvas.setColor(selected ? Color::Bright : Color::MediumBright);
        }

        Font prevFont = canvas.font();
        canvas.setFont(Font::Tiny);
        if (enabled && !actionCell) {
            canvas.setBlendMode(BlendMode::Sub);
            canvas.setColor(Color::Bright);
        }
        canvas.drawTextCentered(x, y - 1, cellWidth, cellHeight, label);
        canvas.setBlendMode(prevBlendMode);
        canvas.setFont(prevFont);
    };

    for (int cell = 0; cell < columns * rows; ++cell) {
        if (cell == ChaosAllOnCell) {
            drawCell(cell, chaosCellLabel(cell), generator.allTargetsEnabled(), cell == _chaosCursor, true);
            continue;
        }
        if (cell == ChaosAllOffCell) {
            drawCell(cell, chaosCellLabel(cell), !generator.allTargetsEnabled(), cell == _chaosCursor, true);
            continue;
        }

        ChaosGenerator::Target target;
        if (!chaosCellTarget(cell, target)) {
            continue;
        }
        drawCell(cell, chaosCellLabel(cell), generator.targetEnabled(target), cell == _chaosCursor, false);
    }

}

int GeneratorPage::contextItemCount() const {
    switch (_generator->mode()) {
    case Generator::Mode::Random:
    case Generator::Mode::Acid:
        return int(ContextAction::Last);
    case Generator::Mode::Euclidean:
        return int(ContextAction::Last);
    default:
        return 4;
    }
}

void GeneratorPage::contextShow(bool doubleClick) {
    if (euclideanGeneratorMode(_generator->mode())) {
        _contextMenuItems[0] = { "" };
        _contextMenuItems[1] = { "RESETGEN" };
        _contextMenuItems[2] = { "CANCEL" };
        _contextMenuItems[3] = { "APPLY" };
        _contextMenuItems[4] = { "" };
    } else {
        switch (_generator->mode()) {
        case Generator::Mode::Random:
        case Generator::Mode::Acid:
            _contextMenuItems[0] = { "NEW RAND" };
            break;
        default:
            _contextMenuItems[0] = { "NEW SEED" };
            break;
        }
        _contextMenuItems[1] = { "RESETGEN" };
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
    }

    showContextMenu(ContextMenu(
        _contextMenuItems,
        contextItemCount(),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void GeneratorPage::contextAction(int index) {
    if (!ensureBoundTrackContext()) {
        return;
    }

    switch (ContextAction(index)) {
    case ContextAction::RandomizeSeed:
        switch (_generator->mode()) {
        case Generator::Mode::Random: {
            auto *random = static_cast<RandomGenerator *>(_generator);
            random->randomizeContextParams();
            random->update();
            _generator->showPreview();
            _launchpadResetState = false;
            break;
        }
        case Generator::Mode::Acid: {
            auto *acid = static_cast<AcidGenerator *>(_generator);
            acid->randomizeContextParams();
            acid->update();
            _generator->showPreview();
            _launchpadResetState = false;
            break;
        }
        case Generator::Mode::Euclidean:
            _generator->randomizeParams();
            _generator->update();
            _generator->showPreview();
            _launchpadResetState = false;
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
    if (euclideanGeneratorMode(_generator->mode())) {
        return ContextAction(index) == ContextAction::Init ||
               ContextAction(index) == ContextAction::Revert ||
               ContextAction(index) == ContextAction::Commit;
    }
    return ContextAction(index) != ContextAction::VariationInfo;
}

void GeneratorPage::init() {
    if (!ensureBoundTrackContext()) {
        return;
    }

    _stepSelection->clear();
    _generator->init();
    if (_generator->showingPreview()) {
        _generator->showPreview();
    }
    _launchpadResetState = true;
}

void GeneratorPage::revert() {
    _stepSelection->clear();
    _generator->revert();
    _applied = false;
    close();
}

void GeneratorPage::commit() {
    if (!ensureBoundTrackContext()) {
        return;
    }

    _stepSelection->clear();
    _generator->apply();
    _applied = true;
    close();
}

void GeneratorPage::togglePreview() {
    if (!ensureBoundTrackContext()) {
        return;
    }

    if (chaosGeneratorMode(_generator->mode()) && !_chaosPreviewArmed && !_generator->showingPreview()) {
        return;
    }

    if (_generator->showingPreview()) {
        _generator->showOriginal();
        showMessage("ORIGINAL");
    } else {
        _generator->showPreview();
        showPreviewStateMessage();
    }
}

void GeneratorPage::launchpadRandomize() {
    if (!ensureBoundTrackContext()) {
        return;
    }

    if (chaosGeneratorMode(_generator->mode())) {
        auto *chaos = static_cast<ChaosGenerator *>(_generator);
        bool wasShowingPreview = _generator->showingPreview();
        _chaosPreviewArmed = true;
        chaos->randomizeSeed();
        chaos->update();
        chaos->showPreview();
        _launchpadResetState = false;
        if (!wasShowingPreview) {
            showPreviewStateMessage();
        }
        return;
    }

    contextAction(int(ContextAction::RandomizeSeed));
}

void GeneratorPage::showPreviewStateMessage() {
    if (chaosGeneratorMode(_generator->mode())) {
        const auto *chaos = static_cast<const ChaosGenerator *>(_generator);
        showMessage(chaos->patternScope() ? "WRECKED" : "VANDALIZED");
    } else if (euclideanGeneratorMode(_generator->mode())) {
        showMessage("CURRENT EUCLIDEAN");
    } else {
        showMessage(seedDrivenGenerator(_generator->mode()) ? "CURRENT SEED" : "PREVIEW");
    }
}

bool GeneratorPage::launchpadShowingPreview() const {
    return _generator && _generator->showingPreview();
}

bool GeneratorPage::launchpadResetState() const {
    return _launchpadResetState;
}
