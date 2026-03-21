#include "AcidModeSelectPage.h"

#include "ui/painters/WindowPainter.h"

enum class Function {
    Cancel = 3,
    OK = 4,
};

static const char *functionNames[] = { nullptr, nullptr, nullptr, "CANCEL", "OK" };

AcidModeSelectPage::AcidModeSelectPage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void AcidModeSelectPage::show(bool allowLayer, ResultCallback callback) {
    _callback = callback;
    _listModel.setAllowLayer(allowLayer);
    setSelectedRow(0);
    ListPage::show();
}

void AcidModeSelectPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "ACID MODE");
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    ListPage::draw(canvas);
}

void AcidModeSelectPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

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

void AcidModeSelectPage::closeWithResult(bool result) {
    Page::close();
    if (_callback) {
        _callback(result, _listModel.rowToMode(selectedRow()));
    }
}
