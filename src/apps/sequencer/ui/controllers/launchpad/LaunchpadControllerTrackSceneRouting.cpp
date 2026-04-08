#include "LaunchpadController.h"
#include "ui/ControllerManager.h"
#include "ui/PageManager.h"
#include "ui/pages/Pages.h"

bool LaunchpadController::modalTrackSelectionLocked() const {
    auto *pageManager = _manager.pageManager();
    return pageManager && pageManager->top()->isModal();
}

bool LaunchpadController::generatorTrackSelectionLockedByUiKind() const {
    auto *pages = _manager.pages();
    return pages &&
           _manager.uiPageKind() == ControllerManager::UiPageKind::Generator &&
           pages->generator.launchpadTrackRetargetLocked();
}

bool LaunchpadController::generatorTrackSelectionLockedByTopPage() const {
    auto *pages = _manager.pages();
    auto *pageManager = _manager.pageManager();
    return pages &&
           pageManager &&
           pageManager->top() == &pages->generator &&
           pages->generator.launchpadTrackRetargetLocked();
}

void LaunchpadController::sequenceSceneMute(const Button &button) {
    if (button.isScene()) {
        _project.playState().toggleMuteTrack(button.scene());
    }
}

void LaunchpadController::sequenceSceneSolo(const Button &button) {
    if (button.isScene()) {
        _project.playState().toggleSoloTrack(button.scene());
    }
}

void LaunchpadController::sequenceSceneFill(const Button &button, bool active) {
    if (button.isScene()) {
        _project.playState().fillTrack(button.scene(), active);
    }
}

void LaunchpadController::sequenceSceneSelectTrack(const Button &button) {
    if (!button.isScene()) {
        return;
    }

    if (modalTrackSelectionLocked()) {
        return;
    }

    if (generatorTrackSelectionLocked()) {
        return;
    }

    if (generatorTrackSelectionLockedByUiKind()) {
        return;
    }

    _project.setSelectedTrackIndex(button.scene());

    auto *pages = _manager.pages();
    auto *pageManager = _manager.pageManager();
    if (!pages || !pageManager) {
        return;
    }

    auto *top = pageManager->top();
    if (top == &pages->project ||
        top == &pages->layout ||
        top == &pages->routing ||
        top == &pages->midiOutput ||
        top == &pages->userScale ||
        top == &pages->clockSetup) {
        pages->top.setMode(TopPage::Mode::SequenceEdit);
    }
}

void LaunchpadController::patternSceneMute(const Button &button) {
    if (button.isScene()) {
        _project.playState().toggleMuteTrack(button.scene());
    }
}

void LaunchpadController::patternSceneFill(const Button &button, bool active) {
    if (button.isScene()) {
        _project.playState().fillTrack(button.scene(), active);
    }
}

void LaunchpadController::patternSceneSelectPattern(const Button &button, PlayState::ExecuteType executeType) {
    if (button.isScene()) {
        int pattern = button.scene() - _pattern.navigation.row * 8;
        _project.playState().selectPattern(pattern, executeType);
    }
}

void LaunchpadController::performerSceneMute(const Button &button) {
    if (button.isScene()) {
        _project.playState().toggleMuteTrack(button.scene());
    }
}

void LaunchpadController::performerSceneSolo(const Button &button) {
    if (button.isScene()) {
        _project.playState().toggleSoloTrack(button.scene());
    }
}

void LaunchpadController::performerSceneFill(const Button &button, bool active) {
    if (button.isScene()) {
        _project.playState().fillTrack(button.scene(), active);
    }
}

void LaunchpadController::performerSceneSelectTrack(const Button &button) {
    if (button.isScene()) {
        _project.setSelectedTrackIndex(button.scene());
    }
}
