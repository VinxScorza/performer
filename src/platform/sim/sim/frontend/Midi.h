#pragma once

#include "RtMidi.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <deque>
#include <mutex>
#include <vector>

#include <cstdint>

namespace sim {

class Midi {
public:
    class Port {
    public:
        typedef std::function<void(const std::vector<uint8_t> &message)> RecvHandler;
        typedef std::function<void()> ConnectHandler;
        typedef std::function<void()> DisconnectHandler;

        Port(
            const std::string &portIn,
            const std::string &portOut,
            RecvHandler recvHandler
        ) : Port(std::vector<std::string>{portIn}, std::vector<std::string>{portOut}, recvHandler) {}

        Port(
            const std::string &portIn,
            const std::string &portOut,
            RecvHandler recvHandler,
            ConnectHandler connectHandler,
            DisconnectHandler disconnectHandler
        ) : Port(std::vector<std::string>{portIn}, std::vector<std::string>{portOut}, recvHandler, connectHandler, disconnectHandler) {}

        Port(
            const std::vector<std::string> &portIns,
            const std::vector<std::string> &portOuts,
            RecvHandler recvHandler
        ) : _portIns(portIns), _portOuts(portOuts), _recvHandler(recvHandler) {}

        Port(
            const std::vector<std::string> &portIns,
            const std::vector<std::string> &portOuts,
            RecvHandler recvHandler,
            ConnectHandler connectHandler,
            DisconnectHandler disconnectHandler
        ) : _portIns(portIns), _portOuts(portOuts), _recvHandler(recvHandler), _connectHandler(connectHandler), _disconnectHandler(disconnectHandler) {}

        bool isOpen() const { return _open; }
        bool inputOpen() const { return _inputOpen; }
        bool outputOpen() const { return _outputOpen; }
        bool inputEnabled() const { return !_portIns.empty(); }
        bool outputEnabled() const { return !_portOuts.empty(); }
        const std::string &inputPortName() const { return _portIns.front(); }
        const std::string &outputPortName() const { return _portOuts.front(); }

        bool send(uint8_t data);
        bool send(const uint8_t *data, size_t length);

        void update();
        void setPorts(const std::vector<std::string> &portIns, const std::vector<std::string> &portOuts);

        void notifyError();
        void receive(const std::vector<uint8_t> &message);

    private:
        void open();
        void close();

        std::vector<std::string> _portIns;
        std::vector<std::string> _portOuts;
        RecvHandler _recvHandler;
        ConnectHandler _connectHandler;
        DisconnectHandler _disconnectHandler;

        bool _open = false;
        bool _inputOpen = false;
        bool _outputOpen = false;
        std::vector<std::unique_ptr<RtMidiIn>> _inputs;
        std::vector<std::unique_ptr<RtMidiOut>> _outputs;

        bool _firstOpenAttempt = true;
        bool _error = false;

        std::deque<std::vector<uint8_t>> _recvQueue;
        std::mutex _recvQueueMutex;
    };

    Midi();

    void registerPort(std::shared_ptr<Port> port);

    void update();

    void dumpPorts();

private:
    std::vector<std::shared_ptr<Port>> _ports;
};

} // namespace sim
