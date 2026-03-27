#include "ChaosScopeSelectPage.h"

#include "Pages.h"
#include "ui/painters/WindowPainter.h"

enum class Function {
    Cancel = 3,
    OK = 4,
};

static const char *functionNames[] = { nullptr, nullptr, nullptr, "CANCEL", "OK" };

ChaosScopeSelectPage::ChaosScopeSelectPage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void ChaosScopeSelectPage::show(ResultCallback callback) {
    _callback = callback;
    setSelectedRow(0);
    ListPage::show();
}

void ChaosScopeSelectPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "CHAOS MODE");
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    ListPage::draw(canvas);
}

void ChaosScopeSelectPage::keyPress(KeyPressEvent &event) {
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
        return;
    }

    if (key.is(Key::Encoder)) {
        closeWithResult(true);
        return;
    }

    ListPage::keyPress(event);
}

void ChaosScopeSelectPage::closeWithResult(bool result) {
    Page::close();
    if (_callback) {
        _callback(result, _listModel.rowToScope(selectedRow()));
    }
}
