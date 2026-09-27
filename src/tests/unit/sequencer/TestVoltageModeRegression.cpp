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
    CASE("Built-in Voltage scale uses 1.2V octave span with 0.1V steps") {
        const int voltageScaleIndex = findScaleIndexByName("Voltage");
        expectTrue(voltageScaleIndex >= 0);

        const auto &scale = Scale::get(voltageScaleIndex);
        expectTrue(scale.notesPerOctave() == 12);
        expectTrue(almostEqual(scale.noteToVolts(0), 0.f));
        expectTrue(almostEqual(scale.noteToVolts(scale.notesPerOctave()), 1.2f));
    }

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

    CASE("Arp selected minor pentatonic scale converts legacy semitone slots to scale degrees") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(minorPentatonicIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(3); // Eb is semitone slot 3, scale degree 1 in C minor pentatonic.
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedSelectedScale = scale.noteToVolts(1);
        float wrongScaleDegreeInterpretation = scale.noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedSelectedScale));
        expectTrue(!almostEqual(actual, wrongScaleDegreeInterpretation));
    }

    CASE("Arp selected minor pentatonic scale masks off-scale legacy semitone slots") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(minorPentatonicIndex);

        auto &offScaleStep = sequence.step(1);
        offScaleStep.clear();
        offScaleStep.setNote(1); // C# is not present in C minor pentatonic.
        offScaleStep.setBypassScale(true);

        auto &inScaleStep = sequence.step(3);
        inScaleStep.clear();
        inScaleStep.setNote(3); // Eb is present.
        inScaleStep.setBypassScale(true);

        const auto &scale = sequence.selectedScale(project.scale());
        expectTrue(!EngineTestHooks::arpPitchSlotActiveForScale(offScaleStep, 1, scale));
        expectTrue(EngineTestHooks::arpPitchSlotActiveForScale(inScaleStep, 3, scale));
    }

    CASE("Arp semitones scale keeps explicit chromatic behavior") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(0); // Semitones
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedSemitone = Scale::get(0).noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedSemitone));
    }

    CASE("Arp routed transpose stays inside selected minor pentatonic scale") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(minorPentatonicIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(3); // Eb
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, 0, 0, 1, sequence, false, true);
        float expectedScale = scale.noteToVolts(2); // Eb slot converted to degree 1, then transposed to degree 2.
        float expectedChromaticRootShift = Scale::get(0).noteToVolts(4);

        expectTrue(almostEqual(actual, expectedScale));
        expectTrue(!almostEqual(actual, expectedChromaticRootShift));
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

    CASE("Stochastic selected minor pentatonic scale converts legacy semitone slots to scale degrees") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(minorPentatonicIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(3); // Eb is semitone slot 3, scale degree 1 in C minor pentatonic.
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedSelectedScale = scale.noteToVolts(1);
        float wrongScaleDegreeInterpretation = scale.noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedSelectedScale));
        expectTrue(!almostEqual(actual, wrongScaleDegreeInterpretation));
    }

    CASE("Stochastic selected minor pentatonic scale masks off-scale legacy semitone slots") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(minorPentatonicIndex);

        auto &offScaleStep = sequence.step(1);
        offScaleStep.clear();
        offScaleStep.setNote(1); // C# is not present in C minor pentatonic.
        offScaleStep.setBypassScale(true);

        auto &inScaleStep = sequence.step(3);
        inScaleStep.clear();
        inScaleStep.setNote(3); // Eb is present.
        inScaleStep.setBypassScale(true);

        const auto &scale = sequence.selectedScale(project.scale());
        expectTrue(!EngineTestHooks::stochasticPitchSlotActiveForScale(offScaleStep, 1, scale));
        expectTrue(EngineTestHooks::stochasticPitchSlotActiveForScale(inScaleStep, 3, scale));
    }

    CASE("Stochastic semitones scale keeps explicit chromatic behavior") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(0); // Semitones
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(1);
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, 0, 0, 0, sequence, false);
        float expectedSemitone = Scale::get(0).noteToVolts(step.note());

        expectTrue(almostEqual(actual, expectedSemitone));
    }

    CASE("Stochastic routed transpose stays inside selected minor pentatonic scale") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        const int minorPentatonicIndex = findScaleIndexByName("Minor Pent.");
        expectTrue(minorPentatonicIndex >= 0);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(minorPentatonicIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(3); // Eb
        step.setBypassScale(true);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, 0, 0, 1, sequence, false, true);
        float expectedScale = scale.noteToVolts(2); // Eb slot converted to degree 1, then transposed to degree 2.
        float expectedChromaticRootShift = Scale::get(0).noteToVolts(4);

        expectTrue(almostEqual(actual, expectedScale));
        expectTrue(!almostEqual(actual, expectedChromaticRootShift));
    }

    CASE("Arp octave transposition on built-in Voltage advances by 1.2V") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Arp);

        const int voltageScaleIndex = findScaleIndexByName("Voltage");
        expectTrue(voltageScaleIndex >= 0);

        auto &sequence = project.selectedArpSequence();
        sequence.setScale(voltageScaleIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(0);
        step.setBypassScale(false);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalArpStepNoteForScale(step, 0, scale, 0, 1, 0, sequence, false);
        expectTrue(almostEqual(actual, 1.2f));
    }

    CASE("Stochastic octave transposition on built-in Voltage advances by 1.2V") {
        Project project;
        project.clear();
        project.setSelectedTrackIndex(0);
        project.setSelectedPatternIndex(0);
        project.setTrackMode(0, Track::TrackMode::Stochastic);

        const int voltageScaleIndex = findScaleIndexByName("Voltage");
        expectTrue(voltageScaleIndex >= 0);

        auto &sequence = project.selectedStochasticSequence();
        sequence.setScale(voltageScaleIndex);
        sequence.setRootNote(0);

        auto &step = sequence.step(0);
        step.clear();
        step.setNote(0);
        step.setBypassScale(false);
        step.setNoteOctaveProbability(0);
        step.setNoteVariationProbability(0);

        const auto &scale = sequence.selectedScale(project.scale());
        float actual = EngineTestHooks::evalStochasticStepNoteForScale(step, 0, scale, 0, 1, 0, sequence, false);
        expectTrue(almostEqual(actual, 1.2f));
    }
}
