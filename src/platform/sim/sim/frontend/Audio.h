#pragma once

#include "Common.h"

#include "soloud.h"
#include "soloud_wav.h"

#include <string>

namespace sim {

class Sample;

class Audio {
public:
    Audio();
    ~Audio();

    SoLoud::Soloud &engine() { return _engine; }

    bool enable();
    void disable();

    void play(Sample &sample);
    void stopAll();
    bool enabled() const { return _enabled; }
    const std::string &lastError() const { return _lastError; }

private:
    SoLoud::Soloud _engine;
    bool _enabled = false;
    std::string _lastError;
};

class Sample {
public:
    typedef std::shared_ptr<Sample> Ptr;

    Sample(const std::string &filename);

private:
    SoLoud::Wav _wav;

    friend class Audio;
};

} // namespace sim
