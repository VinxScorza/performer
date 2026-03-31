#include "CurveSequencePage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,
    Route,
    Last
};

enum class SaveContextAction {
    Load,
    Save,
    SaveAs,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT SEQ" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "ROUTE" },
};

static const ContextMenuModel::Item saveContextMenuItems[] = {
    { "LOAD" },
    { "SAVE" },
    { "SAVE AS"},
};

CurveSequencePage::CurveSequencePage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void CurveSequencePage::enter() {
    _listModel.setSequence(&_project.selectedCurveSequence());
}

void CurveSequencePage::exit() {
    _listModel.setSequence(nullptr);
}

void CurveSequencePage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "SEQUENCE");
    WindowPainter::drawActiveFunction(canvas, Track::trackModeName(_project.selectedTrack().trackMode()));
    WindowPainter::drawFooter(canvas);

    ListPage::draw(canvas);
}

void CurveSequencePage::updateLeds(Leds &leds) {
    ListPage::updateLeds(leds);
}

void CurveSequencePage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();


    if (key.shiftModifier() && event.count() == 2) {
        saveContextShow();
        event.consume();
        return;
    }

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }


    if (key.pageModifier() && event.count() == 2) {
        contextShow(true);
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    if (key.is(Key::Encoder) && selectedRow() == 0) {
        _manager.pages().textInput.show("NAME:", _project.selectedCurveSequence().name(), CurveSequence::NameLength, [this] (bool result, const char *text) {
            if (result) {
                _project.selectedCurveSequence().setName(text);
            }
        });

        return;
    }


    if (!event.consumed()) {
        ListPage::keyPress(event);
    }
}

void CurveSequencePage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void CurveSequencePage::saveContextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        saveContextMenuItems,
        int(SaveContextAction::Last),
        [&] (int index) { saveContextAction(index); },
        [&] (int index) { return true; },
        doubleClick
    ));
}

void CurveSequencePage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        initSequence();
        break;
    case ContextAction::Copy:
        copySequence();
        break;
    case ContextAction::Paste:
        pasteSequence();
        break;
    case ContextAction::Duplicate:
        duplicateSequence();
        break;
    case ContextAction::Route:
        initRoute();
        break;
    case ContextAction::Last:
        break;
    }
}

void CurveSequencePage::saveContextAction(int index) {
    switch (SaveContextAction(index)) {
    case SaveContextAction::Load:
        loadSequence();
        break;
    case SaveContextAction::Save:
        saveSequence();
        break;
    case SaveContextAction::SaveAs:
        saveAsSequence();
        break;
    case SaveContextAction::Last:
        break;
    }
}


bool CurveSequencePage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteCurveSequence();
    case ContextAction::Route:
        return _listModel.routingTarget(selectedRow()) != Routing::Target::None;
    default:
        return true;
    }
}

void CurveSequencePage::initSequence() {
    _project.selectedCurveSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void CurveSequencePage::copySequence() {
    _model.clipBoard().copyCurveSequence(_project.selectedCurveSequence());
    showMessage("SEQUENCE COPIED");
}

void CurveSequencePage::pasteSequence() {
    _model.clipBoard().pasteCurveSequence(_project.selectedCurveSequence());
    showMessage("SEQUENCE PASTED");
}

void CurveSequencePage::duplicateSequence() {
    if (_project.selectedTrack().duplicatePattern(_project.selectedPatternIndex())) {
        showMessage("SEQUENCE DUPLICATED");
    }
}

void CurveSequencePage::initRoute() {
    _manager.pages().top.editRoute(_listModel.routingTarget(selectedRow()), _project.selectedTrackIndex());
}

void CurveSequencePage::loadSequence() {
    _manager.pages().fileSelect.show("LOAD SEQUENCE", FileType::CurveSequence, _project.selectedCurveSequence().slotAssigned() ? _project.selectedCurveSequence().slot() : 0, false, [this] (bool result, int slot) {
        if (result) {
            _manager.pages().confirmation.show("ARE YOU SURE?", [this, slot] (bool result) {
                if (result) {
                    loadSequenceFromSlot(slot);
                }
            });
        }
    });
}

void CurveSequencePage::saveSequence() {

    if (!_project.selectedCurveSequence().slotAssigned() || sizeof(_project.selectedCurveSequence().name())==0) {
        saveAsSequence();
        return;
    }

    saveSequenceToSlot(_project.selectedCurveSequence().slot());

    showMessage("SEQUENCE SAVED");
}

void CurveSequencePage::saveAsSequence() {
    _manager.pages().fileSelect.show("SAVE SEQUENCE", FileType::CurveSequence, _project.selectedCurveSequence().slotAssigned() ? _project.selectedCurveSequence().slot() : 0, true, [this] (bool result, int slot) {
        if (result) {
            if (FileManager::slotUsed(FileType::CurveSequence, slot)) {
                _manager.pages().confirmation.show("ARE YOU SURE?", [this, slot] (bool result) {
                    if (result) {
                        saveSequenceToSlot(slot);
                    }
                });
            } else {
                saveSequenceToSlot(slot);
            }
        }
    });
}

void CurveSequencePage::saveSequenceToSlot(int slot) {
    //_engine.suspend();
    _manager.pages().busy.show("SAVING SEQUENCE ...");

    FileManager::task([this, slot] () {
        return FileManager::writeCurveSequence(_project.selectedCurveSequence(), slot);
    }, [this] (fs::Error result) {
        if (result == fs::OK) {
            showMessage("SEQUENCE SAVED");
        } else {
            showMessage(FixedStringBuilder<32>("FAILED (%s)", fs::errorToString(result)));
        }
        // TODO lock ui mutex
        _manager.pages().busy.close();
        _engine.resume();
    });
}

void CurveSequencePage::loadSequenceFromSlot(int slot) {
    //_engine.suspend();
    _manager.pages().busy.show("LOADING SEQUENCE ...");

    FileManager::task([this, slot] () {
        // TODO this is running in file manager thread but model notification affect ui
        return FileManager::readCurveSequence(_project.selectedCurveSequence(), slot);
    }, [this] (fs::Error result) {
        if (result == fs::OK) {
            showMessage("SEQUENCE LOADED");
        } else if (result == fs::INVALID_CHECKSUM) {
            showMessage("INVALID SEQUENCE FILE");
        } else {
            showMessage(FixedStringBuilder<32>("FAILED (%s)", fs::errorToString(result)));
        }
        // TODO lock ui mutex
        _manager.pages().busy.close();
        _engine.resume();
    });
}
