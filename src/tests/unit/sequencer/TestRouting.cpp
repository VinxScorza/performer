#include "UnitTest.h"

#include "apps/sequencer/model/Project.h"
#include "apps/sequencer/model/Routing.h"

#include <cmath>

namespace {

bool near(float a, float b, float eps = 0.0001f) {
    return std::fabs(a - b) <= eps;
}

float patternValue(int value) {
    return value / 15.f;
}

float divisorValue(int value) {
    return (value - 1.f) / (768.f - 1.f);
}

}

UNIT_TEST("Routing") {
    CASE("Boolean targets use Schmitt thresholds") {
        int16_t discrete = 0;
        bool active = false;
        bool initialized = false;

        float out = Routing::stabilizeTargetValue(Routing::Target::Mute, 0.0f, discrete, active, initialized);
        expectTrue(near(out, 0.f));

        out = Routing::stabilizeTargetValue(Routing::Target::Mute, 0.52f, discrete, active, initialized);
        expectTrue(near(out, 0.f));

        out = Routing::stabilizeTargetValue(Routing::Target::Mute, 0.56f, discrete, active, initialized);
        expectTrue(near(out, 1.f));

        out = Routing::stabilizeTargetValue(Routing::Target::Mute, 0.48f, discrete, active, initialized);
        expectTrue(near(out, 1.f));

        out = Routing::stabilizeTargetValue(Routing::Target::Mute, 0.44f, discrete, active, initialized);
        expectTrue(near(out, 0.f));
    }

    CASE("Discrete targets resist boundary jitter") {
        int16_t discrete = 0;
        bool active = false;
        bool initialized = false;

        float out = Routing::stabilizeTargetValue(Routing::Target::Pattern, patternValue(4), discrete, active, initialized);
        expectTrue(near(out, patternValue(4)));

        out = Routing::stabilizeTargetValue(Routing::Target::Pattern, 4.6f / 15.f, discrete, active, initialized);
        expectTrue(near(out, patternValue(4)));

        out = Routing::stabilizeTargetValue(Routing::Target::Pattern, patternValue(5), discrete, active, initialized);
        expectTrue(near(out, patternValue(5)));

        out = Routing::stabilizeTargetValue(Routing::Target::Pattern, 4.4f / 15.f, discrete, active, initialized);
        expectTrue(near(out, patternValue(5)));

        out = Routing::stabilizeTargetValue(Routing::Target::Pattern, 4.2f / 15.f, discrete, active, initialized);
        expectTrue(near(out, patternValue(4)));
    }

    CASE("Routing target compatibility follows implemented track writers") {
        expectTrue(Routing::targetSupportedByTrackMode(Routing::Target::CurveMin, uint8_t(Track::TrackMode::Curve)));
        expectTrue(!Routing::targetSupportedByTrackMode(Routing::Target::CurveMin, uint8_t(Track::TrackMode::Note)));

        expectTrue(Routing::targetSupportedByTrackMode(Routing::Target::GateProbabilityBias, uint8_t(Track::TrackMode::Stochastic)));
        expectTrue(Routing::targetSupportedByTrackMode(Routing::Target::GateProbabilityBias, uint8_t(Track::TrackMode::Curve)));
        expectTrue(!Routing::targetSupportedByTrackMode(Routing::Target::ShapeProbabilityBias, uint8_t(Track::TrackMode::Note)));

        expectTrue(Routing::targetSupportedByTrackMode(Routing::Target::Divisor, uint8_t(Track::TrackMode::Arp)));
        expectTrue(!Routing::targetSupportedByTrackMode(Routing::Target::RunMode, uint8_t(Track::TrackMode::Arp)));
        expectTrue(!Routing::targetSupportedByTrackMode(Routing::Target::Scale, uint8_t(Track::TrackMode::Curve)));
    }

    CASE("Supported track mask removes incompatible tracks") {
        Project project;
        project.clear();
        project.setTrackMode(0, Track::TrackMode::Note);
        project.setTrackMode(1, Track::TrackMode::Curve);
        project.setTrackMode(2, Track::TrackMode::Stochastic);

        const uint8_t tracks = (1 << 0) | (1 << 1) | (1 << 2);
        expectEqual(uint8_t(1 << 1), project.routing().supportedTracks(Routing::Target::ShapeProbabilityBias, tracks));
        expectEqual(uint8_t((1 << 0) | (1 << 2)), project.routing().supportedTracks(Routing::Target::Scale, tracks));
    }

    CASE("Stochastic track bias routing writes existing routed parameters") {
        Project project;
        project.clear();
        project.setTrackMode(0, Track::TrackMode::Stochastic);
        auto &track = project.track(0).stochasticTrack();

        Routing::setRouted(Routing::Target::GateProbabilityBias, 1 << 0, true);
        track.writeRouted(Routing::Target::GateProbabilityBias, 3, 3.f);
        expectEqual(3, track.gateProbabilityBias());
        Routing::setRouted(Routing::Target::GateProbabilityBias, 1 << 0, false);
    }

    CASE("Routed divisor modulation snaps to known musical divisors") {
        int16_t discrete = 0;
        bool active = false;
        bool initialized = false;

        float out = Routing::stabilizeTargetValue(Routing::Target::Divisor, divisorValue(7), discrete, active, initialized);
        expectTrue(near(out, divisorValue(6)));

        out = Routing::stabilizeTargetValue(Routing::Target::Divisor, divisorValue(8), discrete, active, initialized);
        expectTrue(near(out, divisorValue(8)));

        out = Routing::stabilizeTargetValue(Routing::Target::Divisor, divisorValue(10), discrete, active, initialized);
        expectTrue(near(out, divisorValue(9)));
    }

    CASE("Route divisor min and max edit through known musical divisors") {
        Routing::Route route;
        route.setTarget(Routing::Target::Divisor);

        FixedStringBuilder<32> min;
        route.printMin(min);
        expectEqual((const char *)min, "6 1/32");

        route.editMin(1, false);
        min.reset();
        route.printMin(min);
        expectEqual((const char *)min, "8 1/16T");

        route.editMin(1, false);
        min.reset();
        route.printMin(min);
        expectEqual((const char *)min, "9 1/32.");

        route.setMax(divisorValue(25));
        FixedStringBuilder<32> max;
        route.printMax(max);
        expectEqual((const char *)max, "24 1/8");
    }

    CASE("Routing writes quantized divisor values to sequences") {
        Project project;
        project.clear();
        project.setTrackMode(0, Track::TrackMode::Note);

        Routing::setRouted(Routing::Target::Divisor, 1 << 0, true);
        project.routing().writeTarget(Routing::Target::Divisor, 1 << 0, divisorValue(7));
        expectEqual(6, project.track(0).noteTrack().sequence(0).divisor());
        Routing::setRouted(Routing::Target::Divisor, 1 << 0, false);
    }
}
