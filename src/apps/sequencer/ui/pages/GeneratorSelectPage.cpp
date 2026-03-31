#include "GeneratorSelectPage.h"

#include "Pages.h"
#include "ui/painters/WindowPainter.h"

enum class Function {
    Cancel  = 3,
    OK      = 4,
};

static const char *functionNames[] = { nullptr, nullptr, nullptr, "CANCEL", "OK" };


GeneratorSelectPage::GeneratorSelectPage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void GeneratorSelectPage::show(ResultCallback callback) {
    show(false, callback);
}

void GeneratorSelectPage::show(bool allowAcid, ResultCallback callback) {
    _callback = callback;
    _listModel.setAllowAcid(allowAcid);
    setSelectedRow(0);
    ListPage::show();
}

void GeneratorSelectPage::enter() {
}

void GeneratorSelectPage::exit() {
}

void GeneratorSelectPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "GENERATOR");
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    canvas.setFont(Font::Tiny);
    canvas.setBlendMode(BlendMode::Set);

    constexpr int listTop = 12;
    constexpr int lineHeight = 8;

    for (int row = 0; row < _listModel.rows(); ++row) {
        FixedStringBuilder<24> str;
        _listModel.cell(row, 0, str);

        const int y = listTop + row * lineHeight;
        const bool selected = row == selectedRow();

        canvas.setColor(selected ? Color::Bright : Color::Medium);
        canvas.drawText(8, y + 6, str);
    }
}

void GeneratorSelectPage::updateLeds(Leds &leds) {
}

void GeneratorSelectPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isPlay()) {
        if (key.pageModifier()) {
            _engine.toggleRecording();
        } else {
            _engine.togglePlay(key.shiftModifier());
        }
        event.consume();
        return;
    }

    if (key.isTempo()) {
        if (!key.pageModifier()) {
            _manager.pages().tempo.show();
        }
        event.consume();
        return;
    }

    if (key.isFunction()) {
        switch (Function(key.function())) {
        case Function::Cancel:
            closeWithResult(false);
            break;
        case Function::OK:
            closeWithResult(true);
            break;
        }
    }

    if (key.is(Key::Encoder)) {
        closeWithResult(true);
        return;
    }

    ListPage::keyPress(event);
}

void GeneratorSelectPage::closeWithResult(bool result) {
    Page::close();
    if (_callback) {
        _callback(result, _listModel.rowToMode(selectedRow()));
    }
}
