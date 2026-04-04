#include "AcidModeSelectPage.h"

#include "Pages.h"
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
    _boundTrackIndex = _project.selectedTrackIndex();
    _boundTrackMode = _project.selectedTrack().trackMode();
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
}

void AcidModeSelectPage::closeWithResult(bool result) {
    bool effectiveResult = result && boundTrackContextValid();
    Page::close();
    if (_callback) {
        _callback(effectiveResult, _listModel.rowToMode(selectedRow()));
    }
}

bool AcidModeSelectPage::boundTrackContextValid() const {
    return _project.selectedTrackIndex() == _boundTrackIndex &&
           _project.selectedTrack().trackMode() == _boundTrackMode;
}
