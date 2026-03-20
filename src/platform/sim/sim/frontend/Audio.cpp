#include "Audio.h"

namespace sim {

// ----------------------------------------------------------------------------
// Audio
// ----------------------------------------------------------------------------

Audio::Audio() {
#ifdef __EMSCRIPTEN__
    _enabled = false;
#else
    enable();
#endif
}

Audio::~Audio() {
    disable();
}

bool Audio::enable() {
    if (_enabled) {
        _lastError.clear();
        return true;
    }

#ifdef __EMSCRIPTEN__
    auto result = _engine.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::SDL2, 44100, 512);
#else
    auto result = _engine.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::AUTO, 44100, 512);
#endif

    if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR) {
        _lastError = _engine.getErrorString(result);
        _enabled = false;
        return false;
    }

    _enabled = true;
    _lastError.clear();
    return true;
}

void Audio::disable() {
    if (_enabled) {
        _engine.stopAll();
        _engine.deinit();
        _enabled = false;
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
    _wav.load(filename.c_str());
}

} // namespace sim
