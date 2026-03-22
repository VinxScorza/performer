#pragma once

#include "core/gfx/Canvas.h"
#include "model/NoteSequence.h"
#include "model/StochasticSequence.h"
#include "model/LogicSequence.h"
#include "model/ArpSequence.h"



class SequencePainter {
public:
    static Color dimSequenceColor(uint8_t dimSetting);

    static void drawLoopStart(Canvas &canvas, int x, int y, int w);
    static void drawLoopEnd(Canvas &canvas, int x, int y, int w);

    static void drawOffset(Canvas &canvas, int x, int y, int w, int h, int offset, int minOffset, int maxOffset);
    static void drawRetrigger(Canvas &canvas, int x, int y, int w, int h, int retrigger, int maxRetrigger);
    static void drawProbability(Canvas &canvas, int x, int y, int w, int h, int probability, int maxProbability);
    static void drawLength(Canvas &canvas, int x, int y, int w, int h, int length, int maxLength);
    static void drawLengthRange(Canvas &canvas, int x, int y, int w, int h, int length, int range, int maxLength);
    static void drawSlide(Canvas &canvas, int x, int y, int w, int h, bool active);
    static void drawBypassScale(Canvas &canvas, int x, int y, int w, int h, bool active);

    static void drawStageRepeatMode(Canvas &canvas, int x, int y, int w, int h, Types::StageRepeatMode mode);

    static void drawGateLogicMode(Canvas &canvas, int x, int y, int w, int h, LogicSequence::GateLogicMode mode);
    static void drawNoteLogicMode(Canvas &canvas, int x, int y, int w, int h, LogicSequence::NoteLogicMode mode);


    static void drawSequenceProgress(Canvas &canvas, int x, int y, int w, int h, float progress);
};
