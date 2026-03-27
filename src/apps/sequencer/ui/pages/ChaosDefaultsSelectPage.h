#pragma once

#include "ListPage.h"

#include "ui/model/ChaosDefaultsListModel.h"

class ChaosDefaultsSelectPage : public ListPage {
public:
    ChaosDefaultsSelectPage(PageManager &manager, PageContext &context);

    typedef std::function<void(bool, ChaosDefaultsListModel::Mode)> ResultCallback;

    using ListPage::show;
    void show(ResultCallback callback);

    virtual void draw(Canvas &canvas) override;
    virtual bool isModal() const override { return true; }
    virtual void keyPress(KeyPressEvent &event) override;

private:
    void closeWithResult(bool result);

    ResultCallback _callback;
    ChaosDefaultsListModel _listModel;
};
