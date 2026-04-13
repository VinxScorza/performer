#include "ControllerManager.h"

namespace {
bool matchesKnobPad16LcxlProbe(const MidiMessage &message) {
    if (!message.isControlChange() || message.channel() != 8) {
        return false;
    }
    const uint8_t cc = message.controlNumber();
    return (cc >= 13 && cc <= 20) || (cc >= 29 && cc <= 36) || cc == 106 || cc == 107;
}

bool matchesKnobPad16BspProbe(const MidiMessage &message) {
    if (!message.isControlChange() || message.channel() != 8) {
        return false;
    }
    const uint8_t cc = message.controlNumber();
    // BSP PERFORMERstep16 template aligned with LCXL logical map.
    return (cc >= 13 && cc <= 20) || (cc >= 29 && cc <= 36) || cc == 106 || cc == 107;
}
}


ControllerManager::ControllerManager(Model &model, Engine &engine) :
    _model(model),
    _engine(engine)
{
    _port = MidiPort::UsbMidi;
}

void ControllerManager::connect(uint16_t vendorId, uint16_t productId) {
    auto info = findController(vendorId, productId);
    if (!info) {
        return;
    }

    if (_controller) {
        disconnect();
    }

    switch (info->type) {
    case ControllerInfo::Type::Launchpad:
        _controller = _controllerContainer.create<LaunchpadController>(*this, _model, _engine, *info);
        break;
    case ControllerInfo::Type::KnobPad16LaunchControlXL:
        _controller = _controllerContainer.create<KnobPad16Controller>(
            *this,
            _model,
            _engine,
            KnobPad16Controller::Profile::LaunchControlXL
        );
        break;
    case ControllerInfo::Type::KnobPad16BeatStepPro:
        _controller = _controllerContainer.create<KnobPad16Controller>(
            *this,
            _model,
            _engine,
            KnobPad16Controller::Profile::BeatStepPro
        );
        break;
    }
}

void ControllerManager::disconnect() {
    if (_controller) {
        _controllerContainer.destroy(_controller);
        _controller = nullptr;
    }
}

void ControllerManager::update() {
    if (_controller) {
        _controller->update();
    }
}

void ControllerManager::setUiPageKind(UiPageKind uiPageKind) {
    _uiPageKind = uiPageKind;
    if (_controller) {
        _controller->uiPageChanged();
    }
}

bool ControllerManager::recvMidi(MidiPort port, uint8_t cable, const MidiMessage &message) {
    if (port != _port) {
        return false;
    }

    // Fallback attach for knob/pad controllers in case a USB VID/PID was not
    // recognized by the host stack but MIDI stream clearly matches a known map.
    if (!_controller) {
        if (matchesKnobPad16LcxlProbe(message)) {
            _controller = _controllerContainer.create<KnobPad16Controller>(
                *this,
                _model,
                _engine,
                KnobPad16Controller::Profile::LaunchControlXL
            );
        } else if (matchesKnobPad16BspProbe(message)) {
            _controller = _controllerContainer.create<KnobPad16Controller>(
                *this,
                _model,
                _engine,
                KnobPad16Controller::Profile::BeatStepPro
            );
        }
    }

    if (_controller) {
        _controller->recvMidi(cable, message);
        return true;
    }

    return false;
}

bool ControllerManager::sendMidi(uint8_t cable, const MidiMessage &message) {
    return _engine.sendMidi(_port, cable, message);
}
