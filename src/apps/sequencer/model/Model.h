#pragma once

#include "Project.h"
#include "Settings.h"
#include "ClipBoard.h"
#include "Serialize.h"

#include "os/os.h"

class Model {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    class WriteLock : public os::InterruptLock {};

    class ConfigLock {
    public:
        ConfigLock() {
        }

        ~ConfigLock() {
        }
    };

    //----------------------------------------
    // Properties
    //----------------------------------------

    const Project &project() const { return _project; }
          Project &project()       { return _project; }

    const Settings &settings() const { return _settings; }
          Settings &settings()       { return _settings; }

    const ClipBoard &clipBoard() const { return _clipBoard; }
          ClipBoard &clipBoard()       { return _clipBoard; }

    bool knobPad16Armed() const { return _knobPad16Armed; }
    void setKnobPad16Armed(bool armed) { _knobPad16Armed = armed; }

    //----------------------------------------
    // Methods
    //----------------------------------------

    Model();

    void init();

private:
    Project _project;
    Settings _settings;
    ClipBoard _clipBoard;
    bool _knobPad16Armed = false;
};
