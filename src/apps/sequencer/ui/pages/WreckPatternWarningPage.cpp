#include "WreckPatternWarningPage.h"

#include "Pages.h"
#include "ui/painters/WindowPainter.h"

enum class Function {
    Save = 0,
    Wreck = 2,
    Cancel = 4,
};

static const char *functionNames[] = { "PROJ PAGE", nullptr, "WRECK", nullptr, "CANCEL" };


WreckPatternWarningPage::WreckPatternWarningPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void WreckPatternWarningPage::show(ResultCallback callback) {
    _callback = callback;
    BasePage::show();
}

void WreckPatternWarningPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), 2);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setFont(Font::Tiny);

    canvas.setColor(Color::Bright);
    canvas.drawTextCentered(0, 7, Width, 8, "WARNING");
    canvas.drawTextCentered(0, 21, Width, 8, "Wreck Pattern is a wildly experimental feature");
    canvas.drawTextCentered(0, 29, Width, 8, "Pattern-wide chaos process");
    canvas.drawTextCentered(0, 37, Width, 8, "Save your project before use");
}

void WreckPatternWarningPage::keyPress(KeyPressEvent &event) {
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
        event.consume();
        return;
    }

    if (!key.isFunction()) {
        return;
    }

    switch (Function(key.function())) {
    case Function::Save:
        closeWithResult(Action::Save);
        event.consume();
        break;
    case Function::Wreck:
        closeWithResult(Action::Wreck);
        event.consume();
        break;
    case Function::Cancel:
        closeWithResult(Action::Cancel);
        event.consume();
        break;
    }
}

void WreckPatternWarningPage::closeWithResult(Action action) {
    BasePage::close();
    if (_callback) {
        _callback(action);
    }
}
