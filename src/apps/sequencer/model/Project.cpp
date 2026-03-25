#include "Project.h"
#include "ProjectVersion.h"

Project::Project() :
    _playState(*this),
    _routing(*this)
{
    for (size_t i = 0; i < _tracks.size(); ++i) {
        _tracks[i].setTrackIndex(i);
    }

    clear();
}

void Project::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::Tempo:
        setTempo(floatValue, true);
        break;
    case Routing::Target::Swing:
        setSwing(intValue, true);
        break;
    default:
        break;
    }
}

void Project::clear() {
    _slot = uint8_t(-1);
    StringUtils::copy(_name, "INIT", sizeof(_name));
    setAutoLoaded(false);
    setTempo(120.f);
    setSwing(50);
    setTimeSignature(TimeSignature());
    setSyncMeasure(1);
    setScale(0);
    setRootNote(0);
    setMonitorMode(Types::MonitorMode::Always);
    setRecordMode(Types::RecordMode::Overdub);
    setMidiInputMode(Types::MidiInputMode::All);
    setMidiIntegrationMode(Types::MidiIntegrationMode::None);
    setMidiProgramOffset(0);
    setCvGateInput(Types::CvGateInput::Off);
    setCurveCvInput(Types::CurveCvInput::Off);
    setResetCvOnStop(false);
    setUseMultiCvRec(true);

    _clockSetup.clear();

    for (auto &track : _tracks) {
        track.clear();
    }

    for (int i = 0; i < CONFIG_CHANNEL_COUNT; ++i) {
        _cvOutputTracks[i] = i;
        _gateOutputTracks[i] = i;
    }

    _song.clear();
    _playState.clear();
    _routing.clear();
    _midiOutput.clear();

    for (auto &userScale : UserScale::userScales) {
        userScale.clear();
    }

    setSelectedTrackIndex(0);
    setSelectedPatternIndex(0);

    // load demo project on simulator
#if PLATFORM_SIM
    setName("VINX");
    setTempo(144.4f);
    setSwing(53);

    setTrackMode(7, Track::TrackMode::Curve);

    {
        auto &route1 = routing().route(0);
        route1.setTarget(Routing::Target::RunMode);
        route1.setTracks(127);
        route1.setMin(float(int(Types::RunMode::Forward)));
        route1.setMax(float(int(Types::RunMode::RandomWalk)));
        route1.setSource(Routing::Source::CvOut8);
        route1.cvSource().setRange(Types::VoltageRange::Bipolar5V);
    }

    noteSequence(0, 0).setFirstStep(0);
    noteSequence(0, 0).setLastStep(63);
    noteSequence(0, 0).setGates({
        1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
        1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
        1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
        1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0
    });
    noteSequence(0, 0).step(50).setGate(true);
    noteSequence(0, 0).step(50).setGateProbability(6);

    track(1).noteTrack().clear();
    noteSequence(1, 0).setFirstStep(0);
    noteSequence(1, 0).setLastStep(63);
    noteSequence(1, 0).setGates({
        0,0,0,1,0,0,1,0,0,0,0,1,0,0,1,0,
        1,0,0,0,0,1,1,0,1,0,0,0,0,1,0,0,
        1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,
        1,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1
    });
    noteSequence(1, 0).step(29).setRetrigger(2);
    noteSequence(1, 0).step(29).setRetriggerProbability(6);

    noteSequence(3, 0).setFirstStep(0);
    noteSequence(3, 0).setLastStep(63);
    noteSequence(3, 0).setGates({
        0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0
    });

    noteSequence(2, 0).setFirstStep(0);
    noteSequence(2, 0).setLastStep(63);
    noteSequence(2, 0).setDivisor(24);
    noteSequence(2, 0).setGates({
        1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
        1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
        1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
        1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1
    });
    noteSequence(2, 0).step(63).setGateProbability(6);
    for (int step : { 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62 }) {
        noteSequence(2, 0).step(step).setCondition(static_cast<Types::Condition>(int(Types::Condition::Loop) + 30));
    }

    noteSequence(4, 0).setFirstStep(0);
    noteSequence(4, 0).setLastStep(13);
    noteSequence(4, 0).setGates({
        1,1,1,1,1,1,1,0,0,1,1,1,1,1
    });
    noteSequence(4, 0).step(13).setRetrigger(2);
    noteSequence(4, 0).step(13).setRetriggerProbability(6);

    noteSequence(5, 0).setFirstStep(0);
    noteSequence(5, 0).setLastStep(31);
    noteSequence(5, 0).setGates({
        0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
        0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,1
    });
    noteSequence(5, 0).step(29).setGateProbability(9);
    noteSequence(5, 0).step(31).setGateProbability(9);

    track(6).noteTrack().setFillMode(NoteTrack::FillMode::Gates);
    track(6).noteTrack().setFillMuted(true);
    track(6).noteTrack().setCvUpdateMode(NoteTrack::CvUpdateMode::Gate);
    track(6).noteTrack().setSlideTime(20);
    track(6).noteTrack().setOctave(-1);

    noteSequence(6, 0).setScale(4);
    noteSequence(6, 0).setRootNote(4);
    noteSequence(6, 0).setDivisor(12);
    noteSequence(6, 0).setResetMeasure(0);
    noteSequence(6, 0).setFirstStep(0);
    noteSequence(6, 0).setLastStep(59);
    noteSequence(6, 0).setGates({
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    });
    noteSequence(6, 0).setNotes({
        -8,1,8,4,6,1,4,1,-23,1,8,1,8,11,8,-8,
        4,8,-8,1,8,4,6,1,4,1,-23,1,8,1,8,4,
        8,-8,1,8,4,6,1,4,1,-23,1,8,1,8,11,8,
        -8,1,8,4,6,1,4,1,-23,1,8,1,1,1,1,1
    });
    for (int step = 0; step < CONFIG_STEP_COUNT; ++step) {
        noteSequence(6, 0).step(step).setNoteVariationRange(1);
        noteSequence(6, 0).step(step).setNoteVariationProbability(9);
    }
    for (int step : { 3,18,19,34,35,50,51 }) {
        noteSequence(6, 0).step(step).setSlide(true);
    }

    curveSequence(7, 0).setDivisor(24);
    curveSequence(7, 0).setFirstStep(0);
    curveSequence(7, 0).setLastStep(63);
    curveSequence(7, 0).step(62).setShape(Curve::rampUpHalf);
    curveSequence(7, 0).step(63).setShape(Curve::rampUpHalf);
    curveSequence(7, 0).step(62).setShapeVariation(Curve::High);
    curveSequence(7, 0).step(63).setShapeVariation(Curve::High);
#endif

    _observable.notify(ProjectCleared);
}

void Project::clearPattern(int patternIndex) {
    for (auto &track : _tracks) {
        track.clearPattern(patternIndex);
    }
}

void Project::setTrackMode(int trackIndex, Track::TrackMode trackMode) {
    // TODO make sure engine is synced to this before updating UI
    _playState.revertSnapshot();
    _tracks[trackIndex].setTrackMode(trackMode);
    _observable.notify(TrackModeChanged);
}

void Project::write(VersionedSerializedWriter &writer) const {
    writer.write(_name, NameLength + 1);
    writer.write(_tempo.base);
    writer.write(_swing.base);
    _timeSignature.write(writer);
    writer.write(_syncMeasure);
    writer.write(_scale);
    writer.write(_rootNote);
    writer.write(_monitorMode);
    writer.write(_recordMode);
    writer.write(_midiInputMode);
    _midiInputSource.write(writer);
    writer.write(_midiIntegrationMode);
    writer.write(_midiProgramOffset);
    writer.write(_cvGateInput);
    writer.write(_curveCvInput);

    _clockSetup.write(writer);

    writeArray(writer, _tracks);
    writeArray(writer, _cvOutputTracks);
    writeArray(writer, _gateOutputTracks);

    _song.write(writer);
    _playState.write(writer);
    _routing.write(writer);
    _midiOutput.write(writer);

    writeArray(writer, UserScale::userScales);

    writer.write(_selectedTrackIndex);
    writer.write(_selectedPatternIndex);
    writer.write(_resetCvOnStop);
    writer.write(_useMultiCv);

    writer.writeHash();

    _autoLoaded = false;
}

bool Project::read(VersionedSerializedReader &reader) {
    clear();

    reader.read(_name, NameLength + 1, ProjectVersion::Version5);
    reader.read(_tempo.base);
    _orinalTempo = _tempo.base;
    reader.read(_swing.base);
    if (reader.dataVersion() >= ProjectVersion::Version18) {
        _timeSignature.read(reader);
    }
    reader.read(_syncMeasure);
    reader.read(_scale);
    reader.read(_rootNote);
    reader.read(_monitorMode, ProjectVersion::Version30);
    reader.read(_recordMode);
    if (reader.dataVersion() >= ProjectVersion::Version29) {
        reader.read(_midiInputMode);
        _midiInputSource.read(reader);
    }
    if (reader.dataVersion() >= ProjectVersion::Version32) {
        reader.skip<bool>(ProjectVersion::Version32, ProjectVersion::Version38);
        reader.read(_midiIntegrationMode);
        reader.read(_midiProgramOffset);
    }

    reader.read(_cvGateInput, ProjectVersion::Version6);
    reader.read(_curveCvInput, ProjectVersion::Version11);

    _clockSetup.read(reader);

    readArray(reader, _tracks);
    readArray(reader, _cvOutputTracks);
    readArray(reader, _gateOutputTracks);

    _song.read(reader);
    _playState.read(reader);
    _routing.read(reader);
    _midiOutput.read(reader);

    if (reader.dataVersion() >= ProjectVersion::Version5) {
        readArray(reader, UserScale::userScales);
    }

    reader.read(_selectedTrackIndex);
    reader.read(_selectedPatternIndex);
    reader.read(_resetCvOnStop, ProjectVersion::Version38);
    reader.read(_useMultiCv, ProjectVersion::Version39);

    bool success = reader.checkHash();
    if (success) {
        _observable.notify(ProjectRead);
    } else {
        clear();
    }

    return success;
}
