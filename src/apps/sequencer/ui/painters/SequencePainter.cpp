#include "SequencePainter.h"
#include "core/gfx/Canvas.h"
#include "model/NoteSequence.h"
#include "model/StochasticSequence.h"
#include "model/LogicSequence.h"
#include "model/ArpSequence.h"
#include <algorithm>
#include <bitset>

namespace {

void drawMiniGlyph(Canvas &canvas, int x, int y, const uint8_t *rows, int height, int width = 3) {
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if ((rows[row] >> (width - 1 - col)) & 0x1) {
                canvas.point(x + col, y + row);
            }
        }
    }
}

void drawMiniConditionMark(Canvas &canvas, int x, int y) {
    static const uint8_t cRows[] = {
        0b110,
        0b100,
        0b110,
    };
    drawMiniGlyph(canvas, x, y, cRows, 3);
}

}

Color SequencePainter::dimSequenceColor(uint8_t dimSetting) {
    switch (dimSetting) {
    default:
    case 0:
        return Color::Bright;
    case 1:
        return Color::MediumLow;
    case 2:
        return Color::Low;
    }
}

void SequencePainter::drawLoopStart(Canvas &canvas, int x, int y, int w) {
    canvas.vline(x, y - 1, 3);
    canvas.point(x + 1, y);
}

void SequencePainter::drawLoopEnd(Canvas &canvas, int x, int y, int w) {
    x += w - 1;
    canvas.vline(x, y - 1, 3);
    canvas.point(x - 1, y);
}

void SequencePainter::drawStepIndex(Canvas &canvas, int x, int y, int stepWidth, int stepNumber, bool selected, bool hasCondition) {
    canvas.setColor(selected ? Color::Bright : Color::Medium);
    FixedStringBuilder<8> str("%d", stepNumber);
    int textX = x + (stepWidth - canvas.textWidth(str) + 1) / 2;
    canvas.drawText(textX, y - 2, str);

    if (hasCondition) {
        canvas.setColor(selected ? Color::Bright : Color::MediumLow);
        int numberHeight = canvas.textHeight(str);
        int numberWidth = canvas.textWidth(str);
        int condX = textX + numberWidth + 1;
        int condY = (y - 2) + std::max(0, (numberHeight - 3) / 2) - 3;
        drawMiniConditionMark(canvas, condX, condY);
    }
}

void SequencePainter::drawGateBody(Canvas &canvas, int x, int y, int stepWidth, int gateOffset, int maxGateOffset, int length, int maxLength, int retrigger, int maxRetrigger, bool slide) {
    constexpr int gateInset = 3;
    constexpr int gateShiftRange = 3;
    int gateArea = stepWidth - 2 * gateInset;
    int gateOffsetShift = (gateOffset * gateShiftRange) / (maxGateOffset + 1);
    int gateWidth = 3 + ((gateArea - 3) * (length + 1)) / maxLength;
    int gateX = x + gateInset + gateOffsetShift + (gateArea - gateWidth) / 2;
    int gateY = y + gateInset;

    canvas.fillRect(gateX, gateY, gateWidth, gateArea);

    if (retrigger > 0) {
        int stepBoxY = y + 2;
        int stepBoxSize = stepWidth - 4;
        int dashHeight = 2;
        int dashY = stepBoxY + std::max(0, (stepBoxSize - dashHeight) / 2);
        canvas.setColor(Color::Bright);
        SequencePainter::drawRetrigger(canvas, x, dashY, stepWidth, dashHeight, retrigger + 1, maxRetrigger);
    }

    if (slide) {
        canvas.setColor(Color::MediumBright);
        int tieStartX = x + stepWidth - 3;
        int tieStartY = y + stepWidth - 3;
        int tieMidX = x + stepWidth;
        int tieMidY = tieStartY + 2;
        int tieEndX = x + stepWidth + 3;
        int tieEndY = tieStartY;
        canvas.line(tieStartX, tieStartY, tieMidX, tieMidY);
        canvas.line(tieMidX, tieMidY, tieEndX, tieEndY);
    }
}

void SequencePainter::drawOffset(Canvas &canvas, int x, int y, int w, int h, int offset, int minOffset, int maxOffset) {
    auto remap = [w, minOffset, maxOffset] (int value) {
        return ((w - 1) * (value - minOffset)) / (maxOffset - minOffset);
    };

    canvas.setBlendMode(BlendMode::Set);

    canvas.setColor(Color::Medium);
    canvas.fillRect(x, y, w, h);

    canvas.setColor(Color::None);
    canvas.vline(x + remap(0), y, h);

    canvas.setColor(Color::Bright);
    canvas.vline(x + remap(offset), y, h);
}

void SequencePainter::drawRetrigger(Canvas &canvas, int x, int y, int w, int h, int retrigger, int maxRetrigger) {
    canvas.setBlendMode(BlendMode::Set);

    int bw = w / maxRetrigger;
    x += (w - bw * retrigger) / 2;

    canvas.setColor(Color::Bright);

    for (int i = 0; i < retrigger; ++i) {
        canvas.fillRect(x, y, bw / 2, h);
        x += bw;
    }
}

void SequencePainter::drawProbability(Canvas &canvas, int x, int y, int w, int h, int probability, int maxProbability) {
    canvas.setBlendMode(BlendMode::Set);

    int pw = (w * probability) / maxProbability;

    canvas.setColor(Color::Bright);
    canvas.fillRect(x, y, pw, h);

    canvas.setColor(Color::Medium);
    canvas.fillRect(x + pw, y, w - pw, h);
}

void SequencePainter::drawLength(Canvas &canvas, int x, int y, int w, int h, int length, int maxLength) {
    canvas.setBlendMode(BlendMode::Set);

    int gw = ((w - 1) * length) / maxLength;

    canvas.setColor(Color::Bright);

    canvas.vline(x, y, h);
    canvas.hline(x, y, gw);
    canvas.vline(x + gw, y, h);
    canvas.hline(x + gw, y + h - 1, w - gw);
}

void SequencePainter::drawLengthRange(Canvas &canvas, int x, int y, int w, int h, int length, int range, int maxLength) {
    canvas.setBlendMode(BlendMode::Set);

    int gw = ((w - 1) * length) / maxLength;
    int rw = ((w - 1) * std::max(0, std::min(maxLength, length + range))) / maxLength;

    canvas.setColor(Color::Medium);

    canvas.vline(x, y, h);
    canvas.hline(x, y, gw);
    canvas.vline(x + gw, y, h);
    canvas.hline(x + gw, y + h - 1, w - gw);

    canvas.setColor(Color::Bright);

    canvas.fillRect(x + std::min(gw, rw), y + 2, std::max(gw, rw) - std::min(gw, rw) + 1, h - 4);
}

void SequencePainter::drawSlide(Canvas &canvas, int x, int y, int w, int h, bool active) {
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    if (active) {
        canvas.line(x, y + h, x + w, y);
    } else {
        canvas.hline(x, y + h, w);
    }
}

void SequencePainter::drawBypassScale(Canvas &canvas, int x, int y, int w, int h, bool active) {
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    if (active) {
        canvas.drawText(x,y+4, "1");
    } else {
        canvas.drawText(x,y+4, "0");
    }
}

const std::bitset<4> mask = 0x1;
void SequencePainter::drawStageRepeatMode(Canvas &canvas, int x, int y, int w, int h, Types::StageRepeatMode mode) {
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Bright);
    int bottom = y + h - 1;
    std::bitset<4> enabled;
    x += (w - 8) / 2;

    switch (mode) {
        case Types::StageRepeatMode::Each:
           enabled = 0xf;
            break;
        case Types::StageRepeatMode::First:
            enabled = 0x1;
            break;
        case Types::StageRepeatMode::Middle:
            enabled = 0x1 << 2;
            break;
        case Types::StageRepeatMode::Last:
            enabled = 0x8;
            break;
        case Types::StageRepeatMode::Odd:
            enabled = 0x5;
            break;
        case Types::StageRepeatMode::Even:
            enabled = 0x5 << 1;
            break;
        case Types::StageRepeatMode::Triplets:
            enabled = 0x9;
            break;
        case Types::StageRepeatMode::Random:
            enabled = 0xf;
            break;
    }

    for (int i = 0; i < 4; i++) {
        if (mode == Types::StageRepeatMode::Random) {
            canvas.drawText(x-1, y+4, "????");
        } else {
            if (((enabled >> i) & mask) == 1) {
                canvas.vline(x + 2 * i, y, h);
            } else {
                canvas.hline(x + 2 * i, bottom, 1);
            }
        }
    }
}

void SequencePainter::drawGateLogicMode(Canvas &canvas, int x, int y, int w, int h, LogicSequence::GateLogicMode mode) {
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Bright);
    x += (w - 8) / 2;

    switch (mode) {
        case LogicSequence::GateLogicMode::One:
            canvas.drawTextCentered(x, y+4, 8, -8, "1");
            break;
        case LogicSequence::GateLogicMode::Two:
            canvas.drawTextCentered(x, y+4, 8, -8, "2");
            break;
        case LogicSequence::GateLogicMode::And:
            canvas.drawTextCentered(x, y+4, 8, -8, "&");
            break;
        case LogicSequence::GateLogicMode::Or:
            canvas.drawTextCentered(x, y+4, 8, -8, "|");
            break;
        case LogicSequence::GateLogicMode::Xor:
            canvas.drawTextCentered(x, y+4, 8, -8, "x|");
            break;
        case LogicSequence::GateLogicMode::Nand:
            canvas.drawTextCentered(x, y+4, 8, -8, "!&");
            break;
        case LogicSequence::GateLogicMode::RandomInput:
            canvas.drawTextCentered(x, y+4, 8, -8, "1?2");
            break;
        case LogicSequence::GateLogicMode::RandomLogic:
            canvas.drawTextCentered(x, y+4, 8, -8, "????");
            break;
    }
}

void SequencePainter::drawNoteLogicMode(Canvas &canvas, int x, int y, int w, int h, LogicSequence::NoteLogicMode mode) {
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Bright);
    x += (w - 8) / 2;

    switch (mode) {
        case LogicSequence::NoteLogicMode::NOne:
            canvas.drawTextCentered(x, y+4, 8, -8, "1");
            break;
        case LogicSequence::NoteLogicMode::NTwo:
            canvas.drawTextCentered(x, y+4, 8, -8, "2");
            break;
        case LogicSequence::NoteLogicMode::Min:
            canvas.drawTextCentered(x, y+4, 8, -8, "<");
            break;
        case LogicSequence::NoteLogicMode::Max:
            canvas.drawTextCentered(x, y+4, 8, -8, ">");
            break;
        case LogicSequence::NoteLogicMode::Sum:
            canvas.drawTextCentered(x, y+4, 8, -8, "+");
            break;
        case LogicSequence::NoteLogicMode::Avg:
            canvas.drawTextCentered(x, y+4, 8, -8, "~");
            break;
        case LogicSequence::NoteLogicMode::NRandomInput:
            canvas.drawTextCentered(x, y+4, 8, -8, "1?2");
            break;
        case LogicSequence::NoteLogicMode::NRandomLogic:
            canvas.drawTextCentered(x, y+4, 8, -8, "????");
            break;
    }
}

void SequencePainter::drawSequenceProgress(Canvas &canvas, int x, int y, int w, int h, float progress) {
    if (progress < 0.f) {
        return;
    }

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Medium);
    canvas.fillRect(x, y, w, h);
    canvas.setColor(Color::Bright);
    canvas.vline(x + int(std::floor(progress * w)), y, h);
}
