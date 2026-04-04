#include "LaunchpadController.h"

#include "ui/ControllerManager.h"
#include "ui/PageManager.h"
#include "ui/pages/NoteSequenceEditPage.h"
#include "ui/pages/GeneratorPage.h"
#include "ui/pages/Pages.h"

bool LaunchpadController::generatorModeSupported() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::NoteSequenceEdit ||
           _manager.uiPageKind() == ControllerManager::UiPageKind::Generator;
}

bool LaunchpadController::generatorModeEditPage() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::NoteSequenceEdit;
}

bool LaunchpadController::generatorModePreviewPage() const {
    return _manager.uiPageKind() == ControllerManager::UiPageKind::Generator;
}

bool LaunchpadController::generatorTrackSelectionLocked() const {
    auto *pageManager = _manager.pageManager();
    auto *pages = _manager.pages();

    if (!pageManager || !pages) {
        return false;
    }

    auto *top = pageManager->top();
    return top == &pages->acidModeSelect ||
           top == &pages->chaosScopeSelect ||
           top == &pages->wreckPatternWarning;
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
    switch (gridIndex) {
    case 0:
        return LaunchpadGenerator::Random;
    case 1:
        return LaunchpadGenerator::AcidPhrase;
    case 9:
        return LaunchpadGenerator::AcidLayer;
    case 2:
        return LaunchpadGenerator::Vandalize;
    case 10:
        return LaunchpadGenerator::Wreck;
    case 3:
        return LaunchpadGenerator::Euclidean;
    case 7:
        return LaunchpadGenerator::Init;
    default:
        return LaunchpadGenerator::None;
    }
}

void LaunchpadController::setGeneratorMode(bool active) {
    _generatorMode = active;
    _generatorApplyArmed = false;
    _generatorApplyCanceled = false;
}

void LaunchpadController::sequenceDrawGeneratorMode() {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            setGridLed(row, col, colorOff());
        }
    }

    const struct {
        int row;
        int col;
        LaunchpadGenerator generator;
    } slots[] = {
        { 0, 0, LaunchpadGenerator::Random },
        { 0, 1, LaunchpadGenerator::AcidPhrase },
        { 1, 1, LaunchpadGenerator::AcidLayer },
        { 0, 2, LaunchpadGenerator::Vandalize },
        { 1, 2, LaunchpadGenerator::Wreck },
        { 0, 3, LaunchpadGenerator::Euclidean },
        { 0, 7, LaunchpadGenerator::Init },
    };

    for (const auto &slot : slots) {
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
                }
                return true;
            }

            if (generatorModeEditPage()) {
                sequenceOpenGenerator(generator);
                return true;
            }
        }
        return true;
    }

    if (button.isScene()) {
        cancelGeneratorMode();
        _project.setSelectedTrackIndex(button.scene());

        if (_project.selectedTrack().trackMode() == Track::TrackMode::Note) {
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
