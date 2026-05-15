#include "QuickEditPage.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"

#include <algorithm>

QuickEditPage::QuickEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
}

void QuickEditPage::show(ListModel &listModel, int row) {
    _listModel = &listModel;
    _row = row;
    _compact = false;
    _compactSlot = 0;
    _indexedPage = 0;
    syncIndexedPageToCurrent();
    BasePage::show();
}

void QuickEditPage::showCompact(ListModel &listModel, int row, int slot) {
    _listModel = &listModel;
    _row = row;
    _compact = true;
    _compactSlot = clamp(slot, 0, 4);
    _indexedPage = 0;
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
        if (indexedPagingEnabled()) {
            const int count = _listModel->indexedCount(_row);
            const int pageCount = indexedPageCount();
            _indexedPage = clamp(_indexedPage, 0, pageCount - 1);
            const int pageBase = _indexedPage * 16;
            const int selected = clamp(_listModel->indexed(_row), 0, count - 1);

            for (int i = 0; i < 16; ++i) {
                const int index = pageBase + i;
                leds.set(MatrixMap::fromStep(i), index == selected, index < count);
            }

            leds.set(Key::Left, _indexedPage > 0, _indexedPage > 0);
            leds.set(Key::Right, _indexedPage + 1 < pageCount, _indexedPage + 1 < pageCount);
        } else {
            LedPainter::drawSelectedQuickEditValue(leds, _listModel->indexed(_row), _listModel->indexedCount(_row));
        }
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
        if (indexedPagingEnabled() && !key.shiftModifier()) {
            _indexedPage = clamp(_indexedPage - 1, 0, indexedPageCount() - 1);
            event.consume();
            return;
        }
        _listModel->edit(_row, 1, -1, key.shiftModifier());
        _listModel->setSelectedScale(_project.scale(), true);
    } else if (key.isRight()) {
        if (indexedPagingEnabled() && !key.shiftModifier()) {
            _indexedPage = clamp(_indexedPage + 1, 0, indexedPageCount() - 1);
            event.consume();
            return;
        }
        _listModel->edit(_row, 1, 1, key.shiftModifier());
        _listModel->setSelectedScale(_project.scale(), true);
    } else if (key.isStep()) {
        if (indexedPagingEnabled()) {
            const int index = _indexedPage * 16 + key.step();
            if (index < _listModel->indexedCount(_row)) {
                _listModel->setIndexed(_row, index);
            }
        } else {
            _listModel->setIndexed(_row, key.step());
        }
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

bool QuickEditPage::indexedPagingEnabled() const {
    return !_compact &&
        _listModel &&
        _listModel->indexedSupportsPaging(_row) &&
        _listModel->indexedCount(_row) > 16;
}

int QuickEditPage::indexedPageCount() const {
    if (!_listModel) {
        return 1;
    }
    const int count = _listModel->indexedCount(_row);
    return std::max(1, (count + 15) / 16);
}

void QuickEditPage::syncIndexedPageToCurrent() {
    if (!indexedPagingEnabled()) {
        _indexedPage = 0;
        return;
    }

    const int currentIndex = clamp(_listModel->indexed(_row), 0, std::max(0, _listModel->indexedCount(_row) - 1));
    _indexedPage = clamp(currentIndex / 16, 0, indexedPageCount() - 1);
}
