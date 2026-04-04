#pragma once

#include "ListPage.h"

#include "ui/model/AcidModeSelectListModel.h"

class AcidModeSelectPage : public ListPage {
public:
    AcidModeSelectPage(PageManager &manager, PageContext &context);

    typedef std::function<void(bool, AcidSequenceBuilder::ApplyMode)> ResultCallback;

    using ListPage::show;
    void show(bool allowLayer, ResultCallback callback);

    virtual void draw(Canvas &canvas) override;
    virtual bool isModal() const override { return true; }
    virtual void keyPress(KeyPressEvent &event) override;

private:
    bool boundTrackContextValid() const;
    void closeWithResult(bool result);

    ResultCallback _callback;
    AcidModeSelectListModel _listModel;
    int _boundTrackIndex = -1;
    Track::TrackMode _boundTrackMode = Track::TrackMode::Note;
};
