#include "ControllerManager.h"


ControllerManager::ControllerManager(Model &model, Engine &engine) :
    _model(model),
    _engine(engine)
{
    _port = MidiPort::UsbMidi;
}

void ControllerManager::connect(uint16_t vendorId, uint16_t productId) {
    auto info = findController(vendorId, productId);
    if (info) {
        switch (info->type) {
        case ControllerInfo::Type::Launchpad:
            _controller = _controllerContainer.create<LaunchpadController>(*this, _model, _engine, *info);
            break;
        }
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
    if (_controller && port == _port) {
        _controller->recvMidi(cable, message);
        return true;
    }

    return false;
}

bool ControllerManager::sendMidi(uint8_t cable, const MidiMessage &message) {
    return _engine.sendMidi(_port, cable, message);
}
