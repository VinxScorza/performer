#include "UnitTest.h"

#include "apps/sequencer/engine/EngineTestHooks.h"
#include "apps/sequencer/model/Project.h"
#include "apps/sequencer/model/Scale.h"
#include "apps/sequencer/model/UserScale.h"

#include <cmath>
#include <cstring>

namespace {

int findScaleIndexByName(const char *name) {
    for (int i = 0; i < Scale::Count; ++i) {
        const char *scaleName = Scale::name(i);
        if (scaleName && std::strcmp(scaleName, name) == 0) {
            return i;
        }
    }
    return -1;
}

int configureVoltageUserScale(Project &project, const char *name) {
    auto &userScale = project.userScale(0);
    userScale.clear();
    userScale.setName(name);
    userScale.setMode(UserScale::Mode::Voltage);
    userScale.setSize(4);
    userScale.setItem(0, 0);
    userScale.setItem(1, 173);
    userScale.setItem(2, 497);
    userScale.setItem(3, 1000);
    const int userCount = int(UserScale::userScales.size());
    const int builtinCount = Scale::Count - userCount;
    if (builtinCount >= 0 && userCount > 0) {
        return builtinCount; // User scale slot 0
    }
    return findScaleIndexByName(name);
}

bool almostEqual(float a, float b, float eps = 0.0001f) {
    return std::fabs(a - b) <= eps;
}

} // namespace

UNIT_TEST("VoltageModeRegression") {

    CASE("Arp bypass does not force semitone scale on non-chromatic user voltage scale") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        const int voltageScaleIndex = configureVoltageUserScale(project, "VOLT_REG_ARP");
        expectTrue(voltageScaleIndex >= 0);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(voltageScaleIndex);
        sequence.setRootNote(7);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        const int rootNote = sequence.selectedRootNote(project.rootNote());

        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, rootNote, 0, 0, sequence, false);
        float expectedScale = scale.noteToVolts(step.note());
        float expectedSemitone = Scale::get(0).noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedScale));
        expectTrue(!almostEqual(actual, expectedSemitone));
    }

    CASE("Arp bypass keeps semitone path on chromatic scales") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(1); // Major scale (chromatic)
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedBypass = Scale::get(0).noteToVolts(step.note());
        float expectedSelectedScale = scale.noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedBypass));
        expectTrue(!almostEqual(actual, expectedSelectedScale));
    }

    CASE("Stochastic bypass does not force semitone scale on non-chromatic user voltage scale") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        const int voltageScaleIndex = configureVoltageUserScale(project, "VOLT_REG_STOCH");
        expectTrue(voltageScaleIndex >= 0);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(voltageScaleIndex);
        sequence.setRootNote(9);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        const int rootNote = sequence.selectedRootNote(project.rootNote());

        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, rootNote, 0, 0, sequence, false);
        float expectedScale = scale.noteToVolts(step.note());
        float expectedSemitone = Scale::get(0).noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedScale));
        expectTrue(!almostEqual(actual, expectedSemitone));
    }

    CASE("Stochastic bypass keeps semitone path on chromatic scales") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(1); // Major scale (chromatic)
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedBypass = Scale::get(0).noteToVolts(step.note());
        float expectedSelectedScale = scale.noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedBypass));
        expectTrue(!almostEqual(actual, expectedSelectedScale));
    }
}
