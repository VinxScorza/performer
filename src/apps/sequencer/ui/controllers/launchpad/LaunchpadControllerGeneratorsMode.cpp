#include "LaunchpadController.h"

#include "ui/ControllerManager.h"
#include "ui/PageManager.h"
#include "ui/pages/GeneratorPage.h"
#include "ui/pages/Pages.h"
#include "ui/pages/ArpSequenceEditPage.h"
#include "ui/pages/CurveSequenceEditPage.h"
#include "ui/pages/LogicSequenceEditPage.h"
#include "ui/pages/NoteSequenceEditPage.h"
#include "ui/pages/StochasticSequenceEditPage.h"

namespace {
bool noteGeneratorMapping(Track::TrackMode mode) {
    return mode == Track::TrackMode::Note;
}
}

bool LaunchpadController::generatorModeTrackSupported(Track::TrackMode mode) const {
    switch (mode) {
    case Track::TrackMode::Note:
    case Track::TrackMode::Curve:
    case Track::TrackMode::Stochastic:
    case Track::TrackMode::Logic:
    case Track::TrackMode::Arp:
        return true;
    default:
        return false;
    }
}

bool LaunchpadController::generatorModeTrackSupported() const {
    return generatorModeTrackSupported(_project.selectedTrack().trackMode());
}

bool LaunchpadController::stepEditPageGeneratorModeActive() const {
    auto *pages = _manager.pages();
    auto *pageManager = _manager.pageManager();
    if (!pages || !pageManager) {
        return false;
    }

    auto *top = pageManager->top();
    if (top == &pages->noteSequenceEdit || top == &pages->noteSequence) {
        return pages->noteSequenceEdit.launchpadGeneratorModeActive();
    }
    if (top == &pages->curveSequenceEdit || top == &pages->curveSequence) {
        return pages->curveSequenceEdit.launchpadGeneratorModeActive();
    }
    if (top == &pages->stochasticSequenceEdit || top == &pages->stochasticSequence) {
        return pages->stochasticSequenceEdit.launchpadGeneratorModeActive();
    }
    if (top == &pages->logicSequenceEdit || top == &pages->logicSequence) {
        return pages->logicSequenceEdit.launchpadGeneratorModeActive();
    }
    if (top == &pages->arpSequenceEdit || top == &pages->arpSequence) {
        return pages->arpSequenceEdit.launchpadGeneratorModeActive();
    }

    return false;
}

bool LaunchpadController::generatorModeSupported() const {
    if (generatorModePreviewPage()) {
        return true;
    }

    return stepEditPageGeneratorModeActive();
}

bool LaunchpadController::generatorModeEditPage() const {
    auto *pages = _manager.pages();
    auto *pageManager = _manager.pageManager();
    if (!pages || !pageManager) {
        return false;
    }

    auto *top = pageManager->top();
    return top == &pages->noteSequenceEdit ||
           top == &pages->noteSequence ||
           top == &pages->curveSequenceEdit ||
           top == &pages->curveSequence ||
           top == &pages->stochasticSequenceEdit ||
           top == &pages->stochasticSequence ||
           top == &pages->logicSequenceEdit ||
           top == &pages->logicSequence ||
           top == &pages->arpSequenceEdit ||
           top == &pages->arpSequence;
}

bool LaunchpadController::generatorModePreviewPage() const {
    if (_manager.uiPageKind() == ControllerManager::UiPageKind::Generator) {
        return true;
    }

    auto *pages = _manager.pages();
    auto *pageManager = _manager.pageManager();
    return pages && pageManager && pageManager->top() == &pages->generator;
}

bool LaunchpadController::generatorTrackSelectionLocked() const {
    auto *pageManager = _manager.pageManager();
    auto *pages = _manager.pages();

    if (!pageManager || !pages) {
        return false;
    }

    auto *top = pageManager->top();
    return top == &pages->contextMenu ||
           top == &pages->generatorSelect ||
           top == &pages->acidModeSelect ||
           top == &pages->chaosScopeSelect ||
           top == &pages->wreckPatternWarning;
}

bool LaunchpadController::handleGeneratorModeGlobalButtons(const Button &button, ButtonAction action) {
    if (_mode != Mode::Sequence || !_generatorMode || !generatorModeSupported()) {
        return false;
    }

    if (button.isFunction() && button.function() == 7) {
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

    if (buttonState(LaunchpadDevice::FunctionRow, 7) && button.isFunction() && button.function() == 3 && action == ButtonAction::Down) {
        _generatorApplyCanceled = true;
        auto *pages = _manager.pages();
        auto *pageManager = _manager.pageManager();

        if (pages && pageManager && pageManager->top() == &pages->generator) {
            pages->generator.revert();
            setGeneratorMode(false);
        } else {
            cancelGeneratorMode();
        }

        if (pages) {
            pages->top.setMode(TopPage::Mode::SequenceEdit);
        }
        return true;
    }

    return false;
}

bool LaunchpadController::handleGeneratorModeToggleShortcut(const Button &button) {
    if (!buttonState(LaunchpadDevice::FunctionRow, 7) || !button.isFunction() || button.function() != 3) {
        return false;
    }

    if (_mode != Mode::Sequence || !generatorModeEditPage()) {
        return false;
    }

    if (_generatorMode && generatorModeSupported()) {
        auto *pages = _manager.pages();
        auto *pageManager = _manager.pageManager();

        if (pages && pageManager && pageManager->top() == &pages->generator) {
            pages->generator.revert();
            setGeneratorMode(false);
        } else {
            cancelGeneratorMode();
        }

        if (pages) {
            pages->top.setMode(TopPage::Mode::SequenceEdit);
        }
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
    if (noteGeneratorMapping(_project.selectedTrack().trackMode())) {
        switch (gridIndex) {
        case 0:
            return LaunchpadGenerator::Random;
        case 1:
            return LaunchpadGenerator::AcidLayer;
        case 9:
            return LaunchpadGenerator::AcidPhrase;
        case 2:
            return LaunchpadGenerator::Vandalize;
        case 10:
            return LaunchpadGenerator::Wreck;
        case 3:
            return LaunchpadGenerator::Euclidean;
        case 15:
            return LaunchpadGenerator::Init;
        default:
            return LaunchpadGenerator::None;
        }
    }

    switch (gridIndex) {
    case 0:
        return LaunchpadGenerator::Random;
    case 2:
        return LaunchpadGenerator::Entropy;
    case 3:
        return LaunchpadGenerator::Euclidean;
    case 15:
        return LaunchpadGenerator::Init;
    default:
        return LaunchpadGenerator::None;
    }
}

void LaunchpadController::setGeneratorMode(bool active) {
    _generatorMode = active;
    _generatorApplyArmed = false;
    _generatorApplyCanceled = false;

    if (auto *pages = _manager.pages()) {
        pages->noteSequenceEdit.setLaunchpadGeneratorModeActive(false);
        pages->curveSequenceEdit.setLaunchpadGeneratorModeActive(false);
        pages->stochasticSequenceEdit.setLaunchpadGeneratorModeActive(false);
        pages->logicSequenceEdit.setLaunchpadGeneratorModeActive(false);
        pages->arpSequenceEdit.setLaunchpadGeneratorModeActive(false);

        if (!active) {
            return;
        }

        auto generatorMappedForTrack = [&] () {
            if (noteGeneratorMapping(_project.selectedTrack().trackMode())) {
                switch (_selectedGenerator) {
                case LaunchpadGenerator::Random:
                case LaunchpadGenerator::AcidPhrase:
                case LaunchpadGenerator::AcidLayer:
                case LaunchpadGenerator::Vandalize:
                case LaunchpadGenerator::Wreck:
                case LaunchpadGenerator::Euclidean:
                case LaunchpadGenerator::Init:
                    return true;
                default:
                    return false;
                }
            }

            switch (_selectedGenerator) {
            case LaunchpadGenerator::Random:
            case LaunchpadGenerator::Entropy:
            case LaunchpadGenerator::Euclidean:
            case LaunchpadGenerator::Init:
                return true;
            default:
                return false;
            }
        };

        if (!generatorMappedForTrack()) {
            _selectedGenerator = LaunchpadGenerator::Random;
        }

        switch (_project.selectedTrack().trackMode()) {
        case Track::TrackMode::Note:
            pages->noteSequenceEdit.setLaunchpadGeneratorModeActive(true);
            break;
        case Track::TrackMode::Curve:
            pages->curveSequenceEdit.setLaunchpadGeneratorModeActive(true);
            break;
        case Track::TrackMode::Stochastic:
            pages->stochasticSequenceEdit.setLaunchpadGeneratorModeActive(true);
            break;
        case Track::TrackMode::Logic:
            pages->logicSequenceEdit.setLaunchpadGeneratorModeActive(true);
            break;
        case Track::TrackMode::Arp:
            pages->arpSequenceEdit.setLaunchpadGeneratorModeActive(true);
            break;
        default:
            break;
        }
    }
}

void LaunchpadController::sequenceDrawGeneratorMode() {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            setGridLed(row, col, colorOff());
        }
    }

    const bool noteLayout = noteGeneratorMapping(_project.selectedTrack().trackMode());

    struct GeneratorSlot {
        int row;
        int col;
        LaunchpadGenerator generator;
    };

    const GeneratorSlot noteSlots[] = {
        {0, 0, LaunchpadGenerator::Random},
        {0, 1, LaunchpadGenerator::AcidLayer},
        {1, 1, LaunchpadGenerator::AcidPhrase},
        {0, 2, LaunchpadGenerator::Vandalize},
        {1, 2, LaunchpadGenerator::Wreck},
        {0, 3, LaunchpadGenerator::Euclidean},
        {1, 7, LaunchpadGenerator::Init},
    };
    const GeneratorSlot subsetSlots[] = {
        {0, 0, LaunchpadGenerator::Random},
        {0, 2, LaunchpadGenerator::Entropy},
        {0, 3, LaunchpadGenerator::Euclidean},
        {1, 7, LaunchpadGenerator::Init},
    };

    const auto *slots = noteLayout ? noteSlots : subsetSlots;
    const int slotCount = noteLayout ? int(sizeof(noteSlots) / sizeof(noteSlots[0])) : int(sizeof(subsetSlots) / sizeof(subsetSlots[0]));
    for (int i = 0; i < slotCount; ++i) {
        const auto &slot = slots[i];
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
                    if (generator == LaunchpadGenerator::Init) {
                        setGeneratorMode(false);
                    }
                }
                return true;
            }

            if (generatorModeEditPage()) {
                sequenceOpenGenerator(generator);
                if (generator == LaunchpadGenerator::Init) {
                    setGeneratorMode(false);
                }
                return true;
            }
        }
        return true;
    }

    if (button.isScene()) {
        cancelGeneratorMode();
        _project.setSelectedTrackIndex(button.scene());

        if (_project.selectedTrack().trackMode() != Track::TrackMode::MidiCv) {
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
        if (!noteGeneratorMapping(_project.selectedTrack().trackMode())) {
            switch (_project.selectedTrack().trackMode()) {
            case Track::TrackMode::Curve:
                switch (generator) {
                case LaunchpadGenerator::Random:
                    pages->curveSequenceEdit.openLaunchpadGenerator(Generator::Mode::Random);
                    break;
                case LaunchpadGenerator::Entropy:
                    pages->curveSequenceEdit.openLaunchpadGenerator(Generator::Mode::ChaosEntropy);
                    break;
                case LaunchpadGenerator::Euclidean:
                    pages->curveSequenceEdit.openLaunchpadGenerator(Generator::Mode::Euclidean);
                    break;
                case LaunchpadGenerator::Init:
                    pages->curveSequenceEdit.openLaunchpadGenerator(Generator::Mode::InitSteps);
                    break;
                default:
                    break;
                }
                return;
            case Track::TrackMode::Stochastic:
                switch (generator) {
                case LaunchpadGenerator::Random:
                    pages->stochasticSequenceEdit.openLaunchpadGenerator(Generator::Mode::Random);
                    break;
                case LaunchpadGenerator::Entropy:
                    pages->stochasticSequenceEdit.openLaunchpadGenerator(Generator::Mode::ChaosEntropy);
                    break;
                case LaunchpadGenerator::Euclidean:
                    pages->stochasticSequenceEdit.openLaunchpadGenerator(Generator::Mode::Euclidean);
                    break;
                case LaunchpadGenerator::Init:
                    pages->stochasticSequenceEdit.openLaunchpadGenerator(Generator::Mode::InitSteps);
                    break;
                default:
                    break;
                }
                return;
            case Track::TrackMode::Logic:
                switch (generator) {
                case LaunchpadGenerator::Random:
                    pages->logicSequenceEdit.openLaunchpadGenerator(Generator::Mode::Random);
                    break;
                case LaunchpadGenerator::Entropy:
                    pages->logicSequenceEdit.openLaunchpadGenerator(Generator::Mode::ChaosEntropy);
                    break;
                case LaunchpadGenerator::Euclidean:
                    pages->logicSequenceEdit.openLaunchpadGenerator(Generator::Mode::Euclidean);
                    break;
                case LaunchpadGenerator::Init:
                    pages->logicSequenceEdit.openLaunchpadGenerator(Generator::Mode::InitSteps);
                    break;
                default:
                    break;
                }
                return;
            case Track::TrackMode::Arp:
                switch (generator) {
                case LaunchpadGenerator::Random:
                    pages->arpSequenceEdit.openLaunchpadGenerator(Generator::Mode::Random);
                    break;
                case LaunchpadGenerator::Entropy:
                    pages->arpSequenceEdit.openLaunchpadGenerator(Generator::Mode::ChaosEntropy);
                    break;
                case LaunchpadGenerator::Euclidean:
                    pages->arpSequenceEdit.openLaunchpadGenerator(Generator::Mode::Euclidean);
                    break;
                case LaunchpadGenerator::Init:
                    pages->arpSequenceEdit.openLaunchpadGenerator(Generator::Mode::InitSteps);
                    break;
                default:
                    break;
                }
                return;
            default:
                return;
            }
        }

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
        case LaunchpadGenerator::Entropy:
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
