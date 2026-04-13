#pragma once

#if defined(PLATFORM_SIM)

#include "model/ArpSequence.h"
#include "model/StochasticSequence.h"
#include "model/Scale.h"

namespace EngineTestHooks {

float evalArpStepNoteForScale(const ArpSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, const ArpSequence &sequence, bool useVariation = false);
float evalStochasticStepNoteForScale(const StochasticSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, const StochasticSequence &sequence, bool useVariation = false);

} // namespace EngineTestHooks

#endif // defined(PLATFORM_SIM)

