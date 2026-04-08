#pragma once

#include "ListPage.h"

#include "ui/model/GeneratorSelectListModel.h"

class GeneratorSelectPage : public ListPage {
public:
    GeneratorSelectPage(PageManager &manager, PageContext &context);

    typedef std::function<void(bool, Generator::Mode)> ResultCallback;

    using ListPage::show;
    void show(ResultCallback callback);
    void show(bool allowAcid, ResultCallback callback);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual bool isModal() const override { return true; }

    virtual void keyPress(KeyPressEvent &event) override;

private:
    bool boundTrackContextValid() const;
    void closeWithResult(bool result);

    ResultCallback _callback;
    GeneratorSelectListModel _listModel;
    int _boundTrackIndex = -1;
    Track::TrackMode _boundTrackMode = Track::TrackMode::Note;
};
