#include "StochasticSequencePage.h"

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

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT SEQ" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "ROUTE" },
};


StochasticSequencePage::StochasticSequencePage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void StochasticSequencePage::enter() {
    _listModel.setSequence(&_project.selectedStochasticSequence());
}

void StochasticSequencePage::exit() {
    _listModel.setSequence(nullptr);
}

void StochasticSequencePage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "SEQUENCE");
    WindowPainter::drawActiveFunction(canvas, Track::trackModeName(_project.selectedTrack().trackMode()));
    WindowPainter::drawFooter(canvas);

    ListPage::draw(canvas);
}

void StochasticSequencePage::updateLeds(Leds &leds) {
    ListPage::updateLeds(leds);
}

void StochasticSequencePage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    if (!event.consumed()) {
        ListPage::keyPress(event);
    }

    if (key.isEncoder()) {
        auto row = ListPage::selectedRow();
        if (row == 3) {
            _listModel.setSelectedScale(_project.scale());
        }
    }
}

void StochasticSequencePage::contextShow() {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }
    ));
}

void StochasticSequencePage::contextAction(int index) {
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

bool StochasticSequencePage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteStochasticSequence();
    case ContextAction::Route:
        return _listModel.routingTarget(selectedRow()) != Routing::Target::None;
    default:
        return true;
    }
}

void StochasticSequencePage::initSequence() {
    _project.selectedStochasticSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void StochasticSequencePage::copySequence() {
    _model.clipBoard().copyStochasticSequence(_project.selectedStochasticSequence());
    showMessage("SEQUENCE COPIED");
}

void StochasticSequencePage::pasteSequence() {
    _model.clipBoard().pasteStochasticSequence(_project.selectedStochasticSequence());
    showMessage("SEQUENCE PASTED");
}

void StochasticSequencePage::duplicateSequence() {
    if (_project.selectedTrack().duplicatePattern(_project.selectedPatternIndex())) {
        showMessage("SEQUENCE DUPLICATED");
    }
}

void StochasticSequencePage::initRoute() {
    _manager.pages().top.editRoute(_listModel.routingTarget(selectedRow()), _project.selectedTrackIndex());
}
