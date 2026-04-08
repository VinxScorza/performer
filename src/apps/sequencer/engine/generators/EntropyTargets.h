#pragma once

#include <cstdint>

enum class EntropyTarget : uint8_t {
    Gate,
    GateOffset,
    GateProbability,
    Retrigger,
    RetriggerProbability,
    EventLength,
    EventLengthVariationRange,
    EventLengthVariationProbability,
    PrimaryValue,
    PrimaryValueVariationRange,
    PrimaryValueVariationProbability,
    Register,
    Motion,
    LogicRepeatRules,
    Last
};

static constexpr uint16_t DefaultEntropyTargetMask = (1u << int(EntropyTarget::Last)) - 1u;

inline const char *entropyTargetLabel(EntropyTarget target) {
    switch (target) {
    case EntropyTarget::Gate:                           return "Gate";
    case EntropyTarget::GateOffset:                     return "G Offset";
    case EntropyTarget::GateProbability:                return "G Prob";
    case EntropyTarget::Retrigger:                      return "Retrig";
    case EntropyTarget::RetriggerProbability:           return "R Prob";
    case EntropyTarget::EventLength:                    return "Ev Length";
    case EntropyTarget::EventLengthVariationRange:      return "Ev L Range";
    case EntropyTarget::EventLengthVariationProbability:return "Ev L Prob";
    case EntropyTarget::PrimaryValue:                   return "Prmry Val";
    case EntropyTarget::PrimaryValueVariationRange:     return "P Val Rng";
    case EntropyTarget::PrimaryValueVariationProbability:return "P Val Prob";
    case EntropyTarget::Register:                       return "Register";
    case EntropyTarget::Motion:                         return "Motion";
    case EntropyTarget::LogicRepeatRules:               return "Logic/Rpt";
    case EntropyTarget::Last:                           break;
    }
    return "";
}
