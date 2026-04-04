#include "LaunchpadController.h"
#include "ui/ControllerManager.h"
#include "ui/PageManager.h"
#include "ui/pages/Pages.h"

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

    auto *pageManager = _manager.pageManager();
    if (pageManager && pageManager->top()->isModal()) {
        return;
    }

    if (generatorTrackSelectionLocked()) {
        return;
    }

    auto *pages = _manager.pages();
    if (pages &&
        _manager.uiPageKind() == ControllerManager::UiPageKind::Generator &&
        pages->generator.launchpadTrackRetargetLocked()) {
        return;
    }

    _project.setSelectedTrackIndex(button.scene());
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
