#pragma once

#include <cstdint>
#include <string>

namespace sim {

struct MidiConfig {
    std::string portIn;
    std::string portOut;
    uint16_t vendorId;
    uint16_t productId;
};

static const MidiConfig defaultMidiPortConfig = {
    .portIn = "",
    .portOut = ""
};

static const MidiConfig defaultUsbMidiPortConfig = {
    .portIn = "",
    .portOut = "",
    .vendorId = 0x1235,
    .productId = 0x0037
};

} // namespace sim
