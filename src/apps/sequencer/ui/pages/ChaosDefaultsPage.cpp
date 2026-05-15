#include "ChaosDefaultsPage.h"

#include "Pages.h"
#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "engine/generators/ChaosEntropyGenerator.h"
#include "engine/generators/ChaosGenerator.h"

static const char *functionNames[] = { nullptr, nullptr, nullptr, nullptr, "CLOSE" };

namespace {
int chaosScanIndexFromCell(int cell) {
    constexpr int columns = 4;
    constexpr int rows = 4;
    return (cell % columns) * rows + (cell / columns);
}

int chaosCellFromScanIndex(int index) {
    constexpr int columns = 4;
    constexpr int rows = 4;
    return (index % rows) * columns + (index / rows);
}

constexpr int ChaosAllOnCell = 11;
constexpr int ChaosAllOffCell = 15;
constexpr int EntropyAllOnCell = 11;
constexpr int EntropyAllOffCell = 15;

bool chaosCellTarget(int cell, ChaosGenerator::Target &target) {
    switch (cell) {
    case 0:  target = ChaosGenerator::Target::Gate; return true;
    case 1:  target = ChaosGenerator::Target::Length; return true;
    case 2:  target = ChaosGenerator::Target::Note; return true;
    case 3:  target = ChaosGenerator::Target::BypassScale; return true;
    case 4:  target = ChaosGenerator::Target::GateOffset; return true;
    case 5:  target = ChaosGenerator::Target::LengthVariationRange; return true;
    case 6:  target = ChaosGenerator::Target::Slide; return true;
    case 7:  target = ChaosGenerator::Target::Condition; return true;
    case 8:  target = ChaosGenerator::Target::GateProbability; return true;
    case 9:  target = ChaosGenerator::Target::LengthVariationProbability; return true;
    case 10: target = ChaosGenerator::Target::NoteVariationRange; return true;
    case 12: target = ChaosGenerator::Target::Retrigger; return true;
    case 13: target = ChaosGenerator::Target::RetriggerProbability; return true;
    case 14: target = ChaosGenerator::Target::NoteVariationProbability; return true;
    default: break;
    }
    return false;
}

const char *chaosCellLabel(int cell) {
    if (cell == ChaosAllOnCell) {
        return "All On";
    }
    if (cell == ChaosAllOffCell) {
        return "All Off";
    }

    ChaosGenerator::Target target;
    return chaosCellTarget(cell, target) ? ChaosGenerator::targetCellLabel(target) : "";
}

bool entropyCellTarget(int cell, ChaosEntropyGenerator::Target &target) {
    switch (cell) {
    case 0:  target = ChaosEntropyGenerator::Target::Gate; return true;
    case 4:  target = ChaosEntropyGenerator::Target::GateOffset; return true;
    case 8:  target = ChaosEntropyGenerator::Target::GateProbability; return true;
    case 12: target = ChaosEntropyGenerator::Target::Retrigger; return true;
    case 1:  target = ChaosEntropyGenerator::Target::EventLength; return true;
    case 5:  target = ChaosEntropyGenerator::Target::EventLengthVariationRange; return true;
    case 9:  target = ChaosEntropyGenerator::Target::EventLengthVariationProbability; return true;
    case 13: target = ChaosEntropyGenerator::Target::RetriggerProbability; return true;
    case 2:  target = ChaosEntropyGenerator::Target::PrimaryValue; return true;
    case 6:  target = ChaosEntropyGenerator::Target::PrimaryValueVariationRange; return true;
    case 10: target = ChaosEntropyGenerator::Target::PrimaryValueVariationProbability; return true;
    case 14: target = ChaosEntropyGenerator::Target::Register; return true;
    case 3:  target = ChaosEntropyGenerator::Target::Motion; return true;
    case 7:  target = ChaosEntropyGenerator::Target::LogicRepeatRules; return true;
    default: break;
    }
    return false;
}

const char *entropyCellLabel(int cell) {
    if (cell == EntropyAllOnCell) {
        return "All On";
    }
    if (cell == EntropyAllOffCell) {
        return "All Off";
    }

    ChaosEntropyGenerator::Target target;
    return entropyCellTarget(cell, target) ? ChaosEntropyGenerator::targetCellLabel(target) : "";
}
}

ChaosDefaultsPage::ChaosDefaultsPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void ChaosDefaultsPage::show(ChaosDefaultsListModel::Mode mode) {
    _mode = mode;
    _cursor = 0;
    BasePage::show();
}

void ChaosDefaultsPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "");
    WindowPainter::drawActiveFunction(canvas, activeFunction());
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    constexpr int columns = 4;
    constexpr int rows = 4;
    constexpr int gridTop = 11;
    constexpr int gridBottom = 46;
    constexpr int cellGap = 2;
    const int cellWidth = (Width - (columns + 1) * cellGap) / columns;
    const int cellHeight = (gridBottom - gridTop - (rows - 1) * cellGap) / rows;

    auto drawCell = [&] (int index, const char *label, bool enabled, bool selected, bool actionCell) {
        int row = index / columns;
        int col = index % columns;
        int x = cellGap + col * (cellWidth + cellGap);
        int y = gridTop + row * (cellHeight + cellGap);
        const auto prevBlendMode = canvas.blendMode();

        if (enabled && !actionCell) {
            canvas.setColor(selected ? Color::MediumBright : Color::Medium);
            canvas.fillRect(x, y, cellWidth, cellHeight);
            canvas.setColor(selected ? Color::Bright : Color::Medium);
            canvas.drawRect(x, y, cellWidth, cellHeight);
        } else {
            canvas.setColor(selected ? Color::Medium : Color::Low);
            canvas.drawRect(x, y, cellWidth, cellHeight);
            canvas.setColor(selected ? Color::Bright : Color::MediumBright);
        }

        Font prevFont = canvas.font();
        canvas.setFont(Font::Tiny);
        if (enabled && !actionCell) {
            canvas.setBlendMode(BlendMode::Sub);
            canvas.setColor(Color::Bright);
        }
        canvas.drawTextCentered(x, y - 1, cellWidth, cellHeight, label);
        canvas.setBlendMode(prevBlendMode);
        canvas.setFont(prevFont);
    };

    for (int cell = 0; cell < columns * rows; ++cell) {
        if (_mode == ChaosDefaultsListModel::Mode::Entropy) {
            if (cell == EntropyAllOnCell) {
                drawCell(cell, entropyCellLabel(cell), allTargetsEnabled(), cell == _cursor, true);
                continue;
            }
            if (cell == EntropyAllOffCell) {
                drawCell(cell, entropyCellLabel(cell), !allTargetsEnabled(), cell == _cursor, true);
                continue;
            }

            ChaosEntropyGenerator::Target target;
            if (!entropyCellTarget(cell, target)) {
                continue;
            }
            drawCell(cell, entropyCellLabel(cell), targetEnabled(int(target)), cell == _cursor, false);
            continue;
        }

        if (cell == ChaosAllOnCell) {
            drawCell(cell, chaosCellLabel(cell), allTargetsEnabled(), cell == _cursor, true);
            continue;
        }
        if (cell == ChaosAllOffCell) {
            drawCell(cell, chaosCellLabel(cell), !allTargetsEnabled(), cell == _cursor, true);
            continue;
        }

        ChaosGenerator::Target target;
        if (!chaosCellTarget(cell, target)) {
            continue;
        }
        drawCell(cell, chaosCellLabel(cell), targetEnabled(int(target)), cell == _cursor, false);
    }
}

void ChaosDefaultsPage::updateLeds(Leds &leds) {
    LedPainter::drawSelectedSequenceSection(leds, 0);
}

void ChaosDefaultsPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isFunction() && key.function() == 4) {
        close();
        event.consume();
        return;
    }

    if (key.is(Key::Encoder)) {
        if ((_mode == ChaosDefaultsListModel::Mode::Entropy && _cursor == EntropyAllOnCell) ||
            (_mode != ChaosDefaultsListModel::Mode::Entropy && _cursor == ChaosAllOnCell)) {
            setAllTargets(true);
        } else if ((_mode == ChaosDefaultsListModel::Mode::Entropy && _cursor == EntropyAllOffCell) ||
                   (_mode != ChaosDefaultsListModel::Mode::Entropy && _cursor == ChaosAllOffCell)) {
            setAllTargets(false);
        } else if (_mode == ChaosDefaultsListModel::Mode::Entropy) {
            ChaosEntropyGenerator::Target target;
            if (entropyCellTarget(_cursor, target)) {
                setTargetEnabled(int(target), !targetEnabled(int(target)));
            }
        } else {
            ChaosGenerator::Target target;
            if (chaosCellTarget(_cursor, target)) {
                setTargetEnabled(int(target), !targetEnabled(int(target)));
            }
        }
        event.consume();
        return;
    }

    event.consume();
}

void ChaosDefaultsPage::encoder(EncoderEvent &event) {
    if (event.value() != 0) {
        constexpr int cellCount = 16;
        int scanIndex = chaosScanIndexFromCell(_cursor);
        scanIndex = (scanIndex + event.value()) % cellCount;
        if (scanIndex < 0) {
            scanIndex += cellCount;
        }
        _cursor = chaosCellFromScanIndex(scanIndex);
    }
    event.consume();
}

uint16_t &ChaosDefaultsPage::targetMask() {
    if (_mode == ChaosDefaultsListModel::Mode::Sequence) {
        return _model.settings().userSettings().get<ChaosSeqLayersSetting>(SettingChaosSeqLayers)->getValue();
    }
    if (_mode == ChaosDefaultsListModel::Mode::Entropy) {
        return _model.settings().userSettings().get<EntropyLayersSetting>(SettingEntropyLayers)->getValue();
    }
    return _model.settings().userSettings().get<ChaosPatLayersSetting>(SettingChaosPatLayers)->getValue();
}

const char *ChaosDefaultsPage::activeFunction() const {
    switch (_mode) {
    case ChaosDefaultsListModel::Mode::Sequence:
        return "Seq Layers to Vandalize";
    case ChaosDefaultsListModel::Mode::Pattern:
        return "Pat Layers to Wreck";
    case ChaosDefaultsListModel::Mode::Entropy:
        return "EntropyLayerToUnleash";
    }
    return "";
}

bool ChaosDefaultsPage::targetEnabled(int index) const {
    return (const_cast<ChaosDefaultsPage *>(this)->targetMask() >> index) & 0x1;
}

void ChaosDefaultsPage::setTargetEnabled(int index, bool enabled) {
    auto &mask = targetMask();
    if (enabled) {
        mask |= (1u << index);
    } else {
        mask &= ~(1u << index);
    }
}

void ChaosDefaultsPage::setAllTargets(bool enabled) {
    targetMask() = enabled ? (_mode == ChaosDefaultsListModel::Mode::Entropy ? DefaultEntropyTargetMask : DefaultChaosTargetMask) : 0u;
}

bool ChaosDefaultsPage::allTargetsEnabled() const {
    const uint16_t fullMask = _mode == ChaosDefaultsListModel::Mode::Entropy ? DefaultEntropyTargetMask : DefaultChaosTargetMask;
    return const_cast<ChaosDefaultsPage *>(this)->targetMask() == fullMask;
}
