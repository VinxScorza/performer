#include "Midi.h"

#include <algorithm>

namespace sim {

template<typename T>
static int findPort(T &midi, const std::string &port) {
    for (size_t i = 0; i < midi.getPortCount(); ++i) {
        if (midi.getPortName(i) == port) {
            return i;
        }
    }
    return -1;
}

static void recvCallback(double timeStamp, std::vector<uint8_t> *message, void *userData) {
    auto &port = *static_cast<Midi::Port *>(userData);
    port.receive(*message);
}

static void errorCallback(RtMidiError::Type type, const std::string &errorText, void *userData) {
    auto &port = *static_cast<Midi::Port *>(userData);
    port.notifyError();
}

bool Midi::Port::send(uint8_t data) {
    if (_outputOpen) {
        std::vector<uint8_t> message = { data };
        for (auto &output : _outputs) {
            output->sendMessage(&message);
        }
        return true;
    }

    return false;
}

bool Midi::Port::send(const uint8_t *data, size_t length) {
    if (_outputOpen) {
        std::vector<uint8_t> message(data, data + length);
        for (auto &output : _outputs) {
            output->sendMessage(&message);
        }
        return true;
    }

    return false;
}

void Midi::Port::update() {
    if (!_open) {
        open();
    } else {
        bool inputMissing = false;
        for (size_t i = 0; i < _inputs.size() && !inputMissing; ++i) {
            if (findPort(*_inputs[i], _portIns[i]) == -1) {
                inputMissing = true;
            }
        }
        bool outputMissing = false;
        for (size_t i = 0; i < _outputs.size() && !outputMissing; ++i) {
            if (findPort(*_outputs[i], _portOuts[i]) == -1) {
                outputMissing = true;
            }
        }
        if (_error || inputMissing || outputMissing) {
            close();
        }

        std::lock_guard<std::mutex> lock(_recvQueueMutex);
        while (!_recvQueue.empty()) {
            _recvHandler(_recvQueue.front());
            _recvQueue.pop_front();
        }
    }
}

void Midi::Port::setPorts(const std::vector<std::string> &portIns, const std::vector<std::string> &portOuts) {
    if (_portIns == portIns && _portOuts == portOuts) {
        return;
    }

    close();
    _portIns = portIns;
    _portOuts = portOuts;
}

void Midi::Port::notifyError() {
    _error = true;
}

void Midi::Port::receive(const std::vector<uint8_t> &message) {
    std::lock_guard<std::mutex> lock(_recvQueueMutex);
    _recvQueue.emplace_back(message);
}

void Midi::Port::open() {
    if (_open) {
        return;
    }

    bool openedAny = false;

    _inputs.clear();
    for (const auto &portIn : _portIns) {
        if (portIn.empty()) {
            continue;
        }

        try {
            std::unique_ptr<RtMidiIn> input(new RtMidiIn());
            int index = findPort(*input, portIn);
            if (index >= 0) {
                input->openPort(index);
                input->ignoreTypes(false, false, false);
                input->setCallback(recvCallback, this);
                input->setErrorCallback(errorCallback, this);
                _inputs.emplace_back(std::move(input));
                _inputOpen = true;
                openedAny = true;
            }
        } catch (RtMidiError &error) {
            if (_firstOpenAttempt) {
                std::cout << "Failed to open MIDI input port '" << portIn << "'" << std::endl;
                error.printMessage();
                _firstOpenAttempt = false;
            }
        }
    }

    _outputs.clear();
    for (const auto &portOut : _portOuts) {
        if (portOut.empty()) {
            continue;
        }

        try {
            std::unique_ptr<RtMidiOut> output(new RtMidiOut());
            int index = findPort(*output, portOut);
            if (index >= 0) {
                output->openPort(index);
                output->setErrorCallback(errorCallback, this);
                _outputs.emplace_back(std::move(output));
                _outputOpen = true;
                openedAny = true;
            }
        } catch (RtMidiError &error) {
            if (_firstOpenAttempt) {
                std::cout << "Failed to open MIDI output port '" << portOut << "'" << std::endl;
                error.printMessage();
                _firstOpenAttempt = false;
            }
        }
    }

    if (!openedAny) {
        return;
    }

    if (_connectHandler) {
        _connectHandler();
    }

    _error = false;
    _open = true;
}

void Midi::Port::close() {
    if (!_open) {
        return;
    }

    if (_inputOpen) {
        for (auto &input : _inputs) {
            input->closePort();
        }
        _inputs.clear();
        _inputOpen = false;
    }

    if (_outputOpen) {
        for (auto &output : _outputs) {
            output->closePort();
        }
        _outputs.clear();
        _outputOpen = false;
    }

    if (_disconnectHandler) {
        _disconnectHandler();
    }

    _open = false;
    _firstOpenAttempt = true;
}

Midi::Midi() {
}

void Midi::registerPort(std::shared_ptr<Port> port) {
    _ports.emplace_back(port);
}

void Midi::dumpPorts() {
    try {
        std::vector<std::string> ports;

        std::unique_ptr<RtMidiIn> input(new RtMidiIn());
        for (size_t i = 0; i < input->getPortCount(); ++i) {
            ports.emplace_back(input->getPortName(i));
        }

        std::unique_ptr<RtMidiOut> output(new RtMidiOut());
        for (size_t i = 0; i < output->getPortCount(); ++i) {
            if (!std::count(ports.begin(), ports.end(), output->getPortName(i))) {
                ports.emplace_back(output->getPortName(i));
            }
        }

        std::cout << "MIDI ports:" << std::endl;
        for (const auto &port : ports) {
            std::cout << "  - " << port << std::endl;
        }
    } catch (RtMidiError &error) {
        error.printMessage();
    }
}

void Midi::update() {
    for (const auto &port : _ports) {
        port->update();
    }
}


} // namespace sim
