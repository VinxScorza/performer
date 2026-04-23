#include "QuickEditPage.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"

QuickEditPage::QuickEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
}

void QuickEditPage::show(ListModel &listModel, int row) {
    _listModel = &listModel;
    _row = row;
    _compact = false;
    _compactSlot = 0;
    BasePage::show();
}

void QuickEditPage::showCompact(ListModel &listModel, int row, int slot) {
    _listModel = &listModel;
    _row = row;
    _compact = true;
    _compactSlot = clamp(slot, 0, 4);
    BasePage::show();
}

void QuickEditPage::enter() {
}

void QuickEditPage::exit() {
}

void QuickEditPage::draw(Canvas &canvas) {
    if (_compact) {
        constexpr int FooterHeight = 9;
        const int x0 = (Width * _compactSlot) / 5;
        const int x1 = (Width * (_compactSlot + 1)) / 5;
        const int w = x1 - x0 + 1;
        const int h = 15;
        const int y = Height - FooterHeight - 1 - h;
        const int textY = Height - 16;

        canvas.setColor(Color::None);
        canvas.fillRect(x0, y, w, h);
        canvas.setColor(Color::Bright);
        canvas.drawRect(x0, y, w, h);

        canvas.setBlendMode(BlendMode::Set);
        canvas.setFont(Font::Small);
        canvas.setColor(Color::Bright);

        FixedStringBuilder<16> str;
        _listModel->cell(_row, 1, str);
        canvas.drawText(x0 + (w - canvas.textWidth(str)) / 2, textY, str);

        str.reset();
        _listModel->cell(_row, 0, str);
        canvas.setFont(Font::Tiny);
        const int labelWidth = canvas.textWidth(str);
        const int labelX = x0 + (w - labelWidth) / 2;
        canvas.setColor(Color::None);
        canvas.fillRect(labelX - 1, Height - FooterHeight + 1, labelWidth + 2, FooterHeight - 1);
        canvas.setColor(Color::Bright);
        canvas.drawText(labelX, Height - 3, str);
        return;
    }

    WindowPainter::drawFrame(canvas, 16, 16, 256 - 32, 32);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setFont(Font::Small);
    canvas.setColor(Color::Bright);

    FixedStringBuilder<16> str;
    _listModel->cell(_row, 0, str);

    canvas.drawText(32, 34, str);

    str.reset();
    _listModel->cell(_row, 1, str);
    canvas.drawText(128 + 16, 34, str);
}

void QuickEditPage::updateLeds(Leds &leds) {
    leds.clear();
    if (!_compact) {
        LedPainter::drawSelectedQuickEditValue(leds, _listModel->indexed(_row), _listModel->indexedCount(_row));
    }
}

void QuickEditPage::keyDown(KeyEvent &event) {
    event.consume();
}

void QuickEditPage::keyUp(KeyEvent &event) {
    event.consume();

    if (_compact) {
        return;
    }

    if (globalKeyState()[Key::Page]) {
        return;
    }
    for (int i = 8; i < 16; ++i) {
        if (globalKeyState()[MatrixMap::fromStep(i)]) {
            return;
        }
    }

    close();
}

void QuickEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (_compact && key.isFunction()) {
        close();
        event.consume();
        return;
    }

    if (key.isLeft()) {
        _listModel->edit(_row, 1, -1, key.shiftModifier());
        _listModel->setSelectedScale(_project.scale(), true);
    } else if (key.isRight()) {
        _listModel->edit(_row, 1, 1, key.shiftModifier());
        _listModel->setSelectedScale(_project.scale(), true);
    } else if (key.isStep()) {
        _listModel->setIndexed(_row, key.step());
    }

    if (key.isEncoder()) {
        _listModel->setSelectedScale(_project.scale(), true);
        close();
    }
}

void QuickEditPage::encoder(EncoderEvent &event) {
    _listModel->edit(_row, 1, event.value(), event.pressed() || globalKeyState()[Key::Shift]);
    event.consume();
}
