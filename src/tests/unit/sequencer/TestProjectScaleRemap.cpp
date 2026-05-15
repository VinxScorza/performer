#include "UnitTest.h"

#include "apps/sequencer/model/Project.h"
#include "apps/sequencer/model/Scale.h"

#include <array>
#include <cmath>

namespace {

bool closeEnough(float a, float b, float eps = 0.18f) {
    return std::fabs(a - b) <= eps;
}

} // namespace

UNIT_TEST("ProjectScaleRemap") {
    CASE("Project-level scale remap preserves octave register on default-scale note tracks") {
        Project project;
        project.clear();
        project.setSelectedPatternIndex(0);
        project.setRootNote(0);

        const int trackIndex = 2; // Track 3 in UI terms
        project.setTrackMode(trackIndex, Track::TrackMode::Note);

        auto &sequence = project.noteSequence(trackIndex, 0);
        sequence.setScale(-1); // Use project-level scale

        std::array<int, 4> indices = {0, 1, 2, 3};
        std::array<int, 4> inputNotes = {5, 17, 29, 41};
        std::array<float, 4> expectedVolts = {};
        const auto &oldScale = Scale::get(project.scale()); // default: semitones

        for (size_t i = 0; i < indices.size(); ++i) {
            auto &step = sequence.step(indices[i]);
            step.setGate(true);
            step.setNote(inputNotes[i]);
            expectedVolts[i] = oldScale.noteToVolts(inputNotes[i]);
        }

        project.setScale(1); // Major
        const auto &newScale = Scale::get(project.scale());

        for (size_t i = 0; i < indices.size(); ++i) {
            const auto &step = sequence.step(indices[i]);
            float actualVolts = newScale.noteToVolts(step.note());
            expectTrue(closeEnough(actualVolts, expectedVolts[i]));
        }

        expectTrue(sequence.step(indices[2]).note() > 6);
        expectTrue(sequence.step(indices[3]).note() > 6);
    }

    CASE("Sequence-level note scale remap preserves octave register") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Note);

        auto &sequence = project.selectedNoteSequence();
        sequence.setScale(0); // Semitones
        sequence.setRootNote(0);

        std::array<int, 4> indices = {0, 1, 2, 3};
        std::array<int, 4> inputNotes = {4, 16, 28, 40};
        std::array<float, 4> expectedVolts = {};
        const auto &oldScale = Scale::get(0);

        for (size_t i = 0; i < indices.size(); ++i) {
            auto &step = sequence.step(indices[i]);
            step.setGate(true);
            step.setNote(inputNotes[i]);
            expectedVolts[i] = oldScale.noteToVolts(inputNotes[i]);
        }

        sequence.setScale(1); // Major
        const auto &newScale = Scale::get(1);

        for (size_t i = 0; i < indices.size(); ++i) {
            const auto &step = sequence.step(indices[i]);
            float actualVolts = newScale.noteToVolts(step.note());
            expectTrue(closeEnough(actualVolts, expectedVolts[i]));
        }

        expectTrue(sequence.step(indices[2]).note() > 6);
        expectTrue(sequence.step(indices[3]).note() > 6);
    }
}
