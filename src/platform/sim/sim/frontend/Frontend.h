#pragma once

#include "Common.h"
#include "Window.h"
#include "Audio.h"
#include "InstrumentSetup.h"
#include "Midi.h"
#include "ClockSource.h"

#include "widgets/Button.h"
#include "widgets/Display.h"
#include "widgets/Encoder.h"
#include "widgets/Label.h"
#include "widgets/Led.h"
#include "widgets/Rotary.h"
#include "widgets/Jack.h"

#include "sim/Simulator.h"

#include <string>
#include <vector>

#include <cstdint>

namespace sim {

struct MidiConfig;

class Frontend : private TargetInputHandler, TargetOutputHandler {
public:
    Frontend(Simulator &simulator);
    ~Frontend();

    int main(int argc, char *argv[]);

    void run();

    void close();
    bool setAudioEnabled(bool enabled);
    bool audioEnabled() const { return _audio.enabled(); }

private:
    bool terminate() const;
    void step();
    void update();
    void render();
    void delay(int ms);

    double ticks() const;

    void setup();
    void setupWindow();
    void setupFrontpanel();
    void setupControls();

    void setupMidi();
    void setupInstruments();
    void configureMidiPort(MidiConfig &config, const std::string &port, const std::string &portIn, const std::string &portOut);
    static std::string normalizeMidiPortValue(const std::string &value);
    static void inferUsbMidiDeviceIds(MidiConfig &config);
    static std::vector<std::string> usbMidiInputPorts(const MidiConfig &config);
    static std::vector<std::string> usbMidiOutputPorts(const MidiConfig &config);
    void updateMidiStatusLabels();
    void updateMidiStatusLabel(const Label::Ptr &label, const char *prefix, const std::shared_ptr<Midi::Port> &port);

    // TargetInputHandler
    void writeButton(int index, bool pressed) override;
    void writeEncoder(EncoderEvent event) override;
    void writeAdc(int channel, uint16_t value) override;
    void writeDigitalInput(int pin, bool value) override;
    void writeMidiInput(MidiEvent event) override;

    // TargetOutputHandler
    void writeLed(int index, bool red, bool green) override;
    void writeGateOutput(int channel, bool value) override;
    void writeDac(int channel, uint16_t value) override;
    void writeDigitalOutput(int pin, bool value) override;
    void writeLcd(const FrameBuffer &frameBuffer) override;
    void writeMidiOutput(MidiEvent event) override;

    Simulator &_simulator;
    Audio _audio;
    std::unique_ptr<InstrumentSetup> _instruments;

    MidiConfig *_midiConfig = nullptr;
    MidiConfig *_usbMidiConfig = nullptr;
    bool _traceMidi = false;
    bool _traceDio = false;

    double _timerFrequency;
    double _timerStart;

    uint32_t _lastUpdateTicks = 0;
    double _lastRenderTicks = 0.0;

    Midi _midi;
    std::shared_ptr<Midi::Port> _midiPort;
    std::shared_ptr<Midi::Port> _usbMidiPort;

    std::unique_ptr<ClockSource> _clockSource;

    Window::Ptr _window;
    Encoder::Ptr _encoder;
    Display::Ptr _lcd;
    Label::Ptr _midiStatusLabel;
    Label::Ptr _usbMidiStatusLabel;
    Jack::Ptr _midiInputJack;
    Jack::Ptr _midiOutputJack;
    std::vector<Jack::Ptr> _digitalInputJacks;
    std::vector<Jack::Ptr> _digitalOutputJacks;
    std::vector<Jack::Ptr> _cvInputJacks;
    std::vector<Jack::Ptr> _gateOutputJacks;
    std::vector<Jack::Ptr> _cvOutputJacks;
    std::vector<Button::Ptr> _buttons;
    std::vector<Led::Ptr> _leds;
    std::vector<Label::Ptr> _labels;

#ifdef __EMSCRIPTEN__
    friend void emscriptenMainLoop();
#endif
};

} // namespace sim
