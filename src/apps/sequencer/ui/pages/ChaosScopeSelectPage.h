#pragma once

#include "ListPage.h"

#include "ui/model/ChaosScopeSelectListModel.h"

class ChaosScopeSelectPage : public ListPage {
public:
    ChaosScopeSelectPage(PageManager &manager, PageContext &context);

    typedef std::function<void(bool, ChaosGenerator::Scope)> ResultCallback;

    using ListPage::show;
    void show(ResultCallback callback);

    virtual void draw(Canvas &canvas) override;
    virtual bool isModal() const override { return true; }
    virtual void keyPress(KeyPressEvent &event) override;

private:
    bool boundTrackContextValid() const;
    void closeWithResult(bool result);

    ResultCallback _callback;
    ChaosScopeSelectListModel _listModel;
    int _boundTrackIndex = -1;
    Track::TrackMode _boundTrackMode = Track::TrackMode::Note;
};
