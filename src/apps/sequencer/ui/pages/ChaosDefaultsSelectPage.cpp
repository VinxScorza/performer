#include "ChaosDefaultsSelectPage.h"

#include "ui/painters/WindowPainter.h"

enum class Function {
    Cancel = 3,
    OK = 4,
};

static const char *functionNames[] = { nullptr, nullptr, nullptr, "CANCEL", "OK" };

ChaosDefaultsSelectPage::ChaosDefaultsSelectPage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void ChaosDefaultsSelectPage::show(ResultCallback callback) {
    _callback = callback;
    setSelectedRow(0);
    ListPage::show();
}

void ChaosDefaultsSelectPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "CHAOS DEFAULTS");
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    ListPage::draw(canvas);
}

void ChaosDefaultsSelectPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isFunction()) {
        switch (Function(key.function())) {
        case Function::Cancel:
            closeWithResult(false);
            event.consume();
            break;
        case Function::OK:
            closeWithResult(true);
            event.consume();
            break;
        }
        return;
    }

    if (key.is(Key::Encoder)) {
        closeWithResult(true);
        event.consume();
        return;
    }

    ListPage::keyPress(event);
    event.consume();
}

void ChaosDefaultsSelectPage::closeWithResult(bool result) {
    Page::close();
    if (_callback) {
        _callback(result, _listModel.rowToMode(selectedRow()));
    }
}
