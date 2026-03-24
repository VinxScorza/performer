#include "StochasticTrack.h"
#include "ProjectVersion.h"
#include <string>
#include "core/utils/StringBuilder.h"


void StochasticTrack::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::SlideTime:
        setSlideTime(intValue, true);
        break;
    case Routing::Target::Octave:
        setOctave(intValue, true);
        break;
    case Routing::Target::Transpose:
        setTranspose(intValue, true);
        break;
    case Routing::Target::Rotate:
        setRotate(intValue, true);
        break;
    default:
        break;
    }
}

void StochasticTrack::clear() {
    setPlayMode(Types::PlayMode::Aligned);
    setFillMode(FillMode::Gates);
    setFillMuted(true);
    setCvUpdateMode(CvUpdateMode::Gate);
    setSlideTime(20);
    setOctave(0);
    setTranspose(0);
    setRotate(0);
    setGateProbabilityBias(0);
    setRetriggerProbabilityBias(0);
    setLengthBias(0);
    setNoteProbabilityBias(0);

    for (auto &sequence : _sequences) {
        sequence.clear();
    }
}

void StochasticTrack::write(VersionedSerializedWriter &writer) const {
    writer.write(_name, NameLength + 1);
    writer.write(_playMode);
    writer.write(_fillMode);
    writer.write(_fillMuted);
    writer.write(_cvUpdateMode);
    writer.write(_slideTime.base);
    writer.write(_octave.base);
    writer.write(_transpose.base);
    writer.write(_rotate.base);
    writeArray(writer, _sequences);
}

void StochasticTrack::read(VersionedSerializedReader &reader) {
    reader.backupHash();

    reader.read(_name, NameLength + 1, ProjectVersion::Version33);

    reader.read(_playMode);
    reader.read(_fillMode);
    reader.read(_fillMuted, ProjectVersion::Version26);
    reader.read(_cvUpdateMode, ProjectVersion::Version4);
    reader.read(_slideTime.base);
    reader.read(_octave.base);
    reader.read(_transpose.base);
    reader.read(_rotate.base);

    // There is a bug in previous firmware versions where writing the properties
    // of a note track did not update the hash value.
    if (reader.dataVersion() < ProjectVersion::Version23) {
        reader.restoreHash();
    }

    readArray(reader, _sequences);
}
