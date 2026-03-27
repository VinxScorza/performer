#pragma once

#include "BasePage.h"

#include <functional>

class WreckPatternWarningPage : public BasePage {
public:
    WreckPatternWarningPage(PageManager &manager, PageContext &context);

    enum class Action {
        Save,
        Wreck,
        Cancel,
    };

    typedef std::function<void(Action)> ResultCallback;

    using BasePage::show;
    void show(ResultCallback callback);

    virtual void draw(Canvas &canvas) override;
    virtual bool isModal() const override { return true; }
    virtual void keyPress(KeyPressEvent &event) override;

private:
    void closeWithResult(Action action);

    ResultCallback _callback;
};
