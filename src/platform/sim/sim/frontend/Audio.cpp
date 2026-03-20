#include "Audio.h"

namespace sim {

// ----------------------------------------------------------------------------
// Audio
// ----------------------------------------------------------------------------

Audio::Audio() {
#ifdef __EMSCRIPTEN__
    // Phase 1 browser builds prioritize runtime stability over audio output.
    _enabled = false;
#else
    _engine.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::AUTO, 44100, 512);
    _enabled = true;
#endif
}

Audio::~Audio() {
    if (_enabled) {
        _engine.deinit();
    }
}

void Audio::play(Sample &sample) {
    if (_enabled) {
        _engine.play(sample._wav);
    }
}

void Audio::stopAll() {
    if (_enabled) {
        _engine.stopAll();
    }
}

// ----------------------------------------------------------------------------
// Sample
// ----------------------------------------------------------------------------

Sample::Sample(const std::string &filename) {
#ifndef __EMSCRIPTEN__
    _wav.load(filename.c_str());
#else
    (void) filename;
#endif
}

} // namespace sim
