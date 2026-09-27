#include "Routing.h"

#include "KnownDivisor.h"
#include "Project.h"
#include "ProjectVersion.h"

#include <algorithm>
#include <cmath>

static int quantizedDivisor(float value);

//----------------------------------------
// Routing::CvSource
//----------------------------------------

void Routing::CvSource::clear() {
    _range = Types::VoltageRange::Bipolar5V;
}

void Routing::CvSource::write(VersionedSerializedWriter &writer) const {
    writer.write(_range);
}

void Routing::CvSource::read(VersionedSerializedReader &reader) {
    reader.read(_range);
}

bool Routing::CvSource::operator==(const CvSource &other) const {
    return _range == other._range;
}

//----------------------------------------
// Routing::MidiSource
//----------------------------------------

void Routing::MidiSource::clear() {
    _source.clear();
    _event = Event::ControlAbsolute;
    _controlNumberOrNote = 0;
    _noteRange = 2;
}

void Routing::MidiSource::write(VersionedSerializedWriter &writer) const {
    _source.write(writer);
    writer.write(_event);
    writer.write(_controlNumberOrNote);
    writer.write(_noteRange);
}

void Routing::MidiSource::read(VersionedSerializedReader &reader) {
    _source.read(reader);
    reader.read(_event);
    reader.read(_controlNumberOrNote);
    reader.read(_noteRange, ProjectVersion::Version13);
}

bool Routing::MidiSource::operator==(const MidiSource &other) const {
    return (
        _source == other._source &&
        _event == other._event &&
        _controlNumberOrNote == other._controlNumberOrNote &&
        (_event != Event::NoteRange || _noteRange == other._noteRange)
    );
}

//----------------------------------------
// Routing::Route
//----------------------------------------

Routing::Route::Route() {
    clear();
}

void Routing::Route::clear() {
    _target = Target::None;
    _tracks = 0;
    _min = 0.f;
    _max = 1.f;
    _source = Source::None;
    _cvSource.clear();
    _midiSource.clear();
}

void Routing::Route::write(VersionedSerializedWriter &writer) const {
    writer.writeEnum(_target, targetSerialize);
    writer.write(_tracks);
    writer.write(_min);
    writer.write(_max);
    writer.write(_source);
    if (isCvSource(_source)) {
        _cvSource.write(writer);
    }
    if (isMidiSource(_source)) {
        _midiSource.write(writer);
    }
}

void Routing::Route::read(VersionedSerializedReader &reader) {
    reader.readEnum(_target, targetSerialize);
    reader.read(_tracks);
    reader.read(_min);
    reader.read(_max);
    reader.read(_source);
    if (isCvSource(_source)) {
        _cvSource.read(reader);
    }
    if (isMidiSource(_source)) {
        _midiSource.read(reader);
    }
}

bool Routing::Route::operator==(const Route &other) const {
    return (
        _target == other._target &&
        _tracks == other._tracks &&
        _min == other._min &&
        _max == other._max &&
        _source == other._source &&
        (!isCvSource(_source) || _cvSource == other._cvSource) &&
        (!isMidiSource(_source) || _midiSource == other._midiSource)
    );
}

//----------------------------------------
// Routing
//----------------------------------------

Routing::Routing(Project &project) :
    _project(project)
{}

void Routing::clear() {
    for (auto &route : _routes) {
        route.clear();
    }
}

int Routing::findEmptyRoute() const {
    for (size_t i = 0; i < _routes.size(); ++i) {
        if (!_routes[i].active()) {
            return i;
        }
    }
    return -1;
}

int Routing::findRoute(Target target, int trackIndex) const {
    for (size_t i = 0; i < _routes.size(); ++i) {
        const auto &route = _routes[i];
        if (route.active() && route.target() == target && (!Routing::isTrackTarget(target) || route.tracks() & (1<<trackIndex))) {
            return i;
        }
    }
    return -1;
}

int Routing::checkRouteConflict(const Route &editedRoute, const Route &existingRoute) const {
    for (size_t i = 0; i < _routes.size(); ++i) {
        const auto &route = _routes[i];
        // skip inactive routes and the one we're currently editing
        if (!route.active() || &route == &existingRoute) {
            continue;
        }
        // reject routes with mutually exclusive targets
        if ((route.target() == Target::Play && editedRoute.target() == Target::PlayToggle) ||
            (route.target() == Target::PlayToggle && editedRoute.target() == Target::Play) ||
            (route.target() == Target::Record && editedRoute.target() == Target::RecordToggle) ||
            (route.target() == Target::RecordToggle && editedRoute.target() == Target::Record)) {
            return i;
        }
        // reject routes with same target
        if (route.target() == editedRoute.target()) {
            if (isPerTrackTarget(route.target())) {
                if ((route.tracks() & editedRoute.tracks()) != 0) {
                    return i;
                }
            } else {
                return i;
            }
        }
    }

    return -1;
}

uint8_t Routing::supportedTracks(Target target, uint8_t tracks) const {
    if (!isPerTrackTarget(target)) {
        return 0;
    }

    uint8_t supported = 0;
    for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
        const uint8_t trackBit = 1 << trackIndex;
        if ((tracks & trackBit) &&
            targetSupportedByTrackMode(target, uint8_t(_project.track(trackIndex).trackMode()))) {
            supported |= trackBit;
        }
    }
    return supported;
}

void Routing::writeTarget(Target target, uint8_t tracks, float normalized) {
    float floatValue = denormalizeTargetValue(target, normalized);
    if (target == Target::Divisor) {
        floatValue = quantizedDivisor(floatValue);
    }
    int intValue = std::round(floatValue);

    if (isProjectTarget(target)) {
        _project.writeRouted(target, intValue, floatValue);
    } else if (isPlayStateTarget(target)) {
        _project.playState().writeRouted(target, tracks, intValue, floatValue);
    } else if (isTrackTarget(target) || isSequenceTarget(target)) {
        for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
            if (tracks & (1<<trackIndex)) {
                auto &track = _project.track(trackIndex);
                if (!targetSupportedByTrackMode(target, uint8_t(track.trackMode()))) {
                    continue;
                }
                switch (track.trackMode()) {
                case Track::TrackMode::Note:
                    if (isTrackTarget(target)) {
                        track.noteTrack().writeRouted(target, intValue, floatValue);
                    } else {
                        for (int patternIndex = 0; patternIndex < CONFIG_PATTERN_COUNT; ++patternIndex) {
                            track.noteTrack().sequence(patternIndex).writeRouted(target, intValue, floatValue);
                        }
                    }
                    break;
                case Track::TrackMode::Curve:
                    if (isTrackTarget(target)) {
                        track.curveTrack().writeRouted(target, intValue, floatValue);
                    } else {
                        for (int patternIndex = 0; patternIndex < CONFIG_PATTERN_COUNT; ++patternIndex) {
                            track.curveTrack().sequence(patternIndex).writeRouted(target, intValue, floatValue);
                        }
                    }
                    break;
                case Track::TrackMode::MidiCv:
                    if (isTrackTarget(target)) {
                        track.midiCvTrack().writeRouted(target, intValue, floatValue);
                    }
                    break;
                case Track::TrackMode::Stochastic:
                    if (isTrackTarget(target)) {
                        track.stochasticTrack().writeRouted(target, intValue, floatValue);
                    } else {
                        for (int patternIndex = 0; patternIndex < CONFIG_PATTERN_COUNT; ++patternIndex) {
                            track.stochasticTrack().sequence(patternIndex).writeRouted(target, intValue, floatValue);
                        }
                    }
                    break;
                case Track::TrackMode::Logic:
                    if (isTrackTarget(target)) {
                        track.logicTrack().writeRouted(target, intValue, floatValue);
                    } else {
                        for (int patternIndex = 0; patternIndex < CONFIG_PATTERN_COUNT; ++patternIndex) {
                            track.logicTrack().sequence(patternIndex).writeRouted(target, intValue, floatValue);
                        }
                    }
                    break;
                case Track::TrackMode::Arp:
                    if (isTrackTarget(target)) {
                        track.arpTrack().writeRouted(target, intValue, floatValue);
                    } else {
                        for (int patternIndex = 0; patternIndex < CONFIG_PATTERN_COUNT; ++patternIndex) {
                            track.arpTrack().sequence(patternIndex).writeRouted(target, intValue, floatValue);
                        }
                    }
                    break;                   
                case Track::TrackMode::Last:
                    break;
                }
            }
        }
    }
}

void Routing::write(VersionedSerializedWriter &writer) const {
    writeArray(writer, _routes);
}

void Routing::read(VersionedSerializedReader &reader) {
    readArray(reader, _routes);
}

static std::array<uint8_t, size_t(Routing::Target::Last)> routedSet;
static_assert(sizeof(uint8_t) * 8 >= CONFIG_TRACK_COUNT, "track bits do not fit");

bool Routing::isBooleanTarget(Target target) {
    switch (target) {
    case Target::Play:
    case Target::PlayToggle:
    case Target::Record:
    case Target::RecordToggle:
    case Target::TapTempo:
    case Target::Mute:
    case Target::Fill:
    case Target::CurrentRecordStep:
    case Target::Reseed:
        return true;
    default:
        return false;
    }
}

bool Routing::isContinuousTarget(Target target) {
    switch (target) {
    case Target::Tempo:
    case Target::Swing:
    case Target::FillAmount:
    case Target::SlideTime:
    case Target::Offset:
    case Target::CurveMin:
    case Target::CurveMax:
        return true;
    default:
        return false;
    }
}

bool Routing::targetSupportedByTrackMode(Target target, uint8_t trackMode) {
    switch (target) {
    case Target::None:
        return false;
    case Target::Play:
    case Target::PlayToggle:
    case Target::Record:
    case Target::RecordToggle:
    case Target::TapTempo:
    case Target::Tempo:
    case Target::Swing:
        return true;
    default:
        break;
    }

    const auto mode = Track::TrackMode(trackMode);
    switch (target) {
    case Target::Mute:
    case Target::Fill:
    case Target::FillAmount:
    case Target::Pattern:
        return mode != Track::TrackMode::Last;

    case Target::SlideTime:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Curve ||
               mode == Track::TrackMode::MidiCv ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp;
    case Target::Octave:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp;
    case Target::Transpose:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::MidiCv ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp;
    case Target::Offset:
    case Target::ShapeProbabilityBias:
    case Target::CurveMin:
    case Target::CurveMax:
        return mode == Track::TrackMode::Curve;
    case Target::Rotate:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Curve ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic;
    case Target::GateProbabilityBias:
    case Target::RetriggerProbabilityBias:
    case Target::LengthBias:
    case Target::NoteProbabilityBias:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp ||
               (target == Target::GateProbabilityBias && mode == Track::TrackMode::Curve);

    case Target::FirstStep:
    case Target::LastStep:
    case Target::Divisor:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Curve ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp;
    case Target::RunMode:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Curve ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic;
    case Target::Scale:
    case Target::RootNote:
        return mode == Track::TrackMode::Note ||
               mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Logic ||
               mode == Track::TrackMode::Arp;
    case Target::CurrentRecordStep:
        return mode == Track::TrackMode::Note;
    case Target::Reseed:
    case Target::SequenceFirstStep:
    case Target::SequenceLastStep:
        return mode == Track::TrackMode::Stochastic;
    case Target::RestProbability2:
    case Target::RestProbability4:
    case Target::RestProbability8:
    case Target::LowOctaveRange:
    case Target::HighOctaveRange:
    case Target::LengthModifier:
        return mode == Track::TrackMode::Stochastic ||
               mode == Track::TrackMode::Arp;

    default:
        break;
    }
    return false;
}

bool Routing::isRouted(Target target, int trackIndex) {
    size_t targetIndex = size_t(target);
    if (isPerTrackTarget(target)) {
        if (trackIndex >= 0 && trackIndex < CONFIG_TRACK_COUNT) {
            return (routedSet[targetIndex] & (1 << trackIndex)) != 0;
        }
    } else {
        return routedSet[targetIndex] != 0;
    }
    return false;
}

void Routing::setRouted(Target target, uint8_t tracks, bool routed) {
    size_t targetIndex = size_t(target);
    if (isPerTrackTarget(target)) {
        if (routed) {
            routedSet[targetIndex] |= tracks;
        } else {
            routedSet[targetIndex] &= ~tracks;
        }
    } else {
        routedSet[targetIndex] = routed ? 1 : 0;
    }
}

void Routing::printRouted(StringBuilder &str, Target target, int trackIndex) {
    if (isRouted(target, trackIndex)) {
        str("\x1a");
    }
}

struct TargetInfo {
    int16_t min;
    int16_t max;
    int16_t minDef;
    int16_t maxDef;
    int8_t shiftStep;
};

static const TargetInfo targetInfos[int(Routing::Target::Last)] = {
    [int(Routing::Target::None)]                            = { 0,      0,      0,      0,      0       },
    // Engine targets
    [int(Routing::Target::Play)]                            = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::PlayToggle)]                      = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::Record)]                          = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::RecordToggle)]                    = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::TapTempo)]                        = { 0,      1,      0,      1,      1       },
    // Project targets
    [int(Routing::Target::Tempo)]                           = { 1,      1000,   100,    200,    10      },
    [int(Routing::Target::Swing)]                           = { 50,     75,     50,     75,     5       },
    // PlayState targets
    [int(Routing::Target::Mute)]                            = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::Fill)]                            = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::FillAmount)]                      = { 0,      100,    0,      100,    10      },
    [int(Routing::Target::Pattern)]                         = { 0,      15,     0,      15,     1       },
    // Track targets
    [int(Routing::Target::SlideTime)]                       = { 0,      100,    0,      100,    10      },
    [int(Routing::Target::Octave)]                          = { -10,    10,     -1,     1,      1       },
    [int(Routing::Target::Transpose)]                       = { -60,    60,     -12,    12,     12      },
    [int(Routing::Target::Offset)]                          = { -500,   500,    -100,   100,    100     },
    [int(Routing::Target::Rotate)]                          = { -64,    64,     0,      64,     16      },
    [int(Routing::Target::GateProbabilityBias)]             = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::RetriggerProbabilityBias)]        = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::LengthBias)]                      = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::NoteProbabilityBias)]             = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::ShapeProbabilityBias)]            = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::CurveMin)]                        = { 0,      255,    0,      255,    1       },
    [int(Routing::Target::CurveMax)]                        = { 0,      255,    0,      255,    1       },
    // Sequence targets
    [int(Routing::Target::FirstStep)]                       = { 0,      63,     0,      63,     16      },
    [int(Routing::Target::LastStep)]                        = { 0,      63,     0,      63,     16      },
    [int(Routing::Target::RunMode)]                         = { 0,      5,      0,      5,      1       },
    [int(Routing::Target::Divisor)]                         = { 1,      768,    6,      24,     1       },
    [int(Routing::Target::Scale)]                           = { 0,      23,     0,      23,     1       },
    [int(Routing::Target::RootNote)]                        = { 0,      11,     0,      11,     1       },
    [int(Routing::Target::CurrentRecordStep)]               = { 0,      63,     0,      63,     16      },

    [int(Routing::Target::Reseed)]                          = { 0,      1,      0,      1,      1       },
    [int(Routing::Target::RestProbability2)]                = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::RestProbability4)]                = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::RestProbability8)]                = { -8,     8,      -8,     8,      8       },
    [int(Routing::Target::SequenceFirstStep)]               = { 0,      63,     1,      63,     16      },
    [int(Routing::Target::SequenceLastStep)]                = { 0,      63,     0,      63,     16      },
    [int(Routing::Target::LowOctaveRange)]                  = {-10,     10,     -1,     1,      1       },
    [int(Routing::Target::HighOctaveRange)]                 = {-10,     10,     -1,     1,      1       },
    [int(Routing::Target::LengthModifier)]                  = { -8,     8,      -8,     8,      8       },
};

static int divisorIndexNearest(float value) {
    int bestIndex = 0;
    float bestDistance = std::fabs(value - float(knownDivisors[0].divisor));
    for (int i = 1; i < numKnownDivisors; ++i) {
        const float distance = std::fabs(value - float(knownDivisors[i].divisor));
        if (distance < bestDistance) {
            bestIndex = i;
            bestDistance = distance;
        }
    }
    return bestIndex;
}

static int quantizedDivisor(float value) {
    return knownDivisors[divisorIndexNearest(value)].divisor;
}

static int adjustedDivisor(float value, int offset) {
    int index = divisorIndexNearest(value);
    index = clamp(index + offset, 0, numKnownDivisors - 1);
    return knownDivisors[index].divisor;
}

static float normalizedDivisor(float value) {
    const auto &info = targetInfos[int(Routing::Target::Divisor)];
    return clamp((value - info.min) / (info.max - info.min), 0.f, 1.f);
}

static float divisorStabilizerMargin(int currentDivisor, int nextDivisor) {
    const auto &info = targetInfos[int(Routing::Target::Divisor)];
    const float distance = std::fabs(float(nextDivisor - currentDivisor)) / float(info.max - info.min);
    return std::min(0.02f, std::max(0.00005f, distance * 0.1f));
}

float Routing::stabilizeTargetValue(Target target, float normalized, int16_t &lastDiscreteValue, bool &lastBooleanValue, bool &initialized) {
    normalized = clamp(normalized, 0.f, 1.f);

    if (isBooleanTarget(target)) {
        constexpr float OnThreshold = 0.55f;
        constexpr float OffThreshold = 0.45f;

        bool active = initialized ? lastBooleanValue : normalized >= 0.5f;
        if (active) {
            active = normalized > OffThreshold;
        } else {
            active = normalized >= OnThreshold;
        }

        lastBooleanValue = active;
        initialized = true;
        return active ? 1.f : 0.f;
    }

    if (!isDiscreteTarget(target)) {
        return normalized;
    }

    if (target == Target::Divisor) {
        const float value = denormalizeTargetValue(target, normalized);
        const int candidate = quantizedDivisor(value);

        if (!initialized) {
            lastDiscreteValue = candidate;
            initialized = true;
            return normalizeTargetValue(target, lastDiscreteValue);
        }

        lastDiscreteValue = quantizedDivisor(lastDiscreteValue);
        const int currentIndex = divisorIndexNearest(lastDiscreteValue);
        if (candidate > lastDiscreteValue && currentIndex < numKnownDivisors - 1) {
            const int nextDivisor = knownDivisors[currentIndex + 1].divisor;
            const float boundary = normalizedDivisor((float(lastDiscreteValue) + float(nextDivisor)) * 0.5f);
            if (normalized > boundary + divisorStabilizerMargin(lastDiscreteValue, nextDivisor)) {
                lastDiscreteValue = candidate;
            }
        } else if (candidate < lastDiscreteValue && currentIndex > 0) {
            const int prevDivisor = knownDivisors[currentIndex - 1].divisor;
            const float boundary = normalizedDivisor((float(lastDiscreteValue) + float(prevDivisor)) * 0.5f);
            if (normalized < boundary - divisorStabilizerMargin(lastDiscreteValue, prevDivisor)) {
                lastDiscreteValue = candidate;
            }
        }

        return normalizeTargetValue(target, lastDiscreteValue);
    }

    const auto &info = targetInfos[int(target)];
    const float step = 1.f / float(info.max - info.min);
    const float margin = std::min(0.05f, std::max(0.002f, step * 0.25f));
    const int candidate = std::round(denormalizeTargetValue(target, normalized));

    if (!initialized) {
        lastDiscreteValue = candidate;
        initialized = true;
        return normalizeTargetValue(target, lastDiscreteValue);
    }

    if (candidate > lastDiscreteValue) {
        const float boundary = normalizeTargetValue(target, float(lastDiscreteValue) + 0.5f);
        if (normalized > boundary + margin) {
            lastDiscreteValue = candidate;
        }
    } else if (candidate < lastDiscreteValue) {
        const float boundary = normalizeTargetValue(target, float(lastDiscreteValue) - 0.5f);
        if (normalized < boundary - margin) {
            lastDiscreteValue = candidate;
        }
    }

    return normalizeTargetValue(target, lastDiscreteValue);
}

float Routing::normalizeTargetValue(Routing::Target target, float value) {
    const auto &info = targetInfos[int(target)];
    return clamp((value - info.min) / (info.max - info.min), 0.f, 1.f);
}

float Routing::denormalizeTargetValue(Routing::Target target, float normalized) {
    const auto &info = targetInfos[int(target)];
    return normalized * (info.max - info.min) + info.min;
}

std::pair<float, float> Routing::normalizedDefaultRange(Target target) {
    const auto &info = targetInfos[int(target)];
    return { normalizeTargetValue(target, info.minDef), normalizeTargetValue(target, info.maxDef) };
}

float Routing::targetValueStep(Routing::Target target, bool shift) {
    const auto &info = targetInfos[int(target)];
    return 1.f / (info.max - info.min) * (shift ? info.shiftStep : 1);
}

float Routing::adjustTargetValue(Routing::Target target, float normalized, int value, bool shift) {
    if (target == Target::Divisor) {
        return normalizeTargetValue(target, adjustedDivisor(denormalizeTargetValue(target, normalized), value));
    }
    return normalized + value * targetValueStep(target, shift);
}

void Routing::printTargetValue(Routing::Target target, float normalized, StringBuilder &str) {
    float value = denormalizeTargetValue(target, normalized);
    int intValue = std::round(value);
    switch (target) {
    case Target::None:
        str("-");
        break;
    case Target::Tempo:
        str("%.1f", value);
        break;
    case Target::Swing:
    case Target::SlideTime:
    case Target::FillAmount:
        str("%d%%", intValue);
        break;
    case Target::Octave:
    case Target::Transpose:
    case Target::Rotate:
    case Target::LowOctaveRange:
    case Target::HighOctaveRange:
        str("%+d", intValue);
        break;
    case Target::Offset:
        str("%+.2fV", value * 0.01f);
        break;
    case Target::GateProbabilityBias:
    case Target::RetriggerProbabilityBias:
    case Target::LengthBias:
    case Target::NoteProbabilityBias:
    case Target::ShapeProbabilityBias:
    case Target::RestProbability2:
    case Target::RestProbability4:
    case Target::RestProbability8:
    case Target::SequenceFirstStep:
    case Target::SequenceLastStep:
    case Target::LengthModifier:
        str("%+.1f%%", value * 12.5f);
        break;
    case Target::Divisor:
        ModelUtils::printDivisor(str, quantizedDivisor(value));
        break;
    case Target::RunMode:
        str("%s", Types::runModeName(Types::RunMode(intValue)));
        break;
    case Target::FirstStep:
    case Target::LastStep:
    case Target::Pattern:
        str("%d", intValue + 1);
        break;
    case Target::Play:
    case Target::PlayToggle:
    case Target::Record:
    case Target::RecordToggle:
    case Target::TapTempo:
    case Target::Mute:
    case Target::Fill:
        str(intValue ? "on" : "off");
        break;
    case Target::Scale:
        str("%s", Scale::name(intValue));
        break;
    case Target::RootNote:
        Types::printNote(str, intValue);
        break;
    default:
        str("%d", intValue);
        break;
    }
}
