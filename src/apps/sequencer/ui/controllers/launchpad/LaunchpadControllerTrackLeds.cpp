#include "LaunchpadController.h"

void LaunchpadController::drawTracksGateAndSelected(const Engine &engine, int selectedTrack) {
    for (int track = 0; track < 8; ++track) {
        const auto &trackEngine = engine.trackEngine(track);
        bool unmutedActivity = trackEngine.activity() && !trackEngine.mute();
        bool mutedActivity = trackEngine.activity() && trackEngine.mute();
        bool selected = track == selectedTrack;
        setSceneLed(
            track,
            color(
                (mutedActivity || (selected && !unmutedActivity)),
                (unmutedActivity || (selected && !mutedActivity))
            )
        );
    }
}

void LaunchpadController::drawTracksGateAndMute(const Engine &engine, const PlayState &playState) {
    (void)playState;

    for (int track = 0; track < 8; ++track) {
        const auto &trackEngine = engine.trackEngine(track);
        setSceneLed(
            track,
            color(
                trackEngine.mute(),
                trackEngine.activity()
            )
        );
    }
}
