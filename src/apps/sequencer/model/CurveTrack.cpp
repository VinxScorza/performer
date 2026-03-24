#include "CurveTrack.h"
#include "ProjectVersion.h"
#include "Routing.h"

void CurveTrack::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::SlideTime:
        setSlideTime(intValue, true);
        break;
    case Routing::Target::Offset:
        setOffset(intValue, true);
        break;
    case Routing::Target::Rotate:
        setRotate(intValue, true);
        break;
    case Routing::Target::ShapeProbabilityBias:
        setShapeProbabilityBias(intValue, true);
        break;
    case Routing::Target::GateProbabilityBias:
        setGateProbabilityBias(intValue, true);
        break;
    case Routing::Target::CurveMin:
        setMin(floatValue, true);
        break;
    case Routing::Target::CurveMax:
        setMax(floatValue, true);
        break;
    default:
        break;
    }
}

void CurveTrack::clear() {
    setPlayMode(Types::PlayMode::Aligned);
    setFillMode(FillMode::None);
    setMuteMode(MuteMode::LastValue);
    setSlideTime(20);
    setOffset(0);
    setRotate(0);
    setShapeProbabilityBias(0);
    setGateProbabilityBias(0);
    setCurveCvInput(Types::CurveCvInput::Off);
    setMin(0);
    setMax(CurveSequence::Max::max());

    for (auto &sequence : _sequences) {
        sequence.clear();
    }
}


void CurveTrack::write(VersionedSerializedWriter &writer) const {
    writer.write(_name, NameLength + 1);
    writer.write(_playMode);
    writer.write(_fillMode);
    writer.write(_muteMode);
    writer.write(_slideTime.base);
    writer.write(_offset.base);
    writer.write(_rotate.base);
    writer.write(_shapeProbabilityBias.base);
    writer.write(_gateProbabilityBias.base);
    writer.write(_curveCvInput);
    writer.write(_min);
    writer.write(_max);
    writeArray(writer, _sequences);
    writer.write(_patternFollow);
}

void CurveTrack::read(VersionedSerializedReader &reader) {
    reader.read(_name, NameLength + 1, ProjectVersion::Version33);
    reader.read(_playMode);
    reader.read(_fillMode);
    reader.read(_muteMode, ProjectVersion::Version22);
    reader.read(_slideTime.base, ProjectVersion::Version8);
    reader.read(_offset.base, ProjectVersion::Version28);
    reader.read(_rotate.base);
    reader.read(_shapeProbabilityBias.base, ProjectVersion::Version15);
    reader.read(_gateProbabilityBias.base, ProjectVersion::Version15);
    reader.read(_curveCvInput, ProjectVersion::Version36);
    reader.read(_min, ProjectVersion::Version37);
    reader.read(_max, ProjectVersion::Version37);
    readArray(reader, _sequences);
    reader.read(_patternFollow, ProjectVersion::Version39);
}
