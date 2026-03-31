#pragma once

#include "BasePage.h"

#include "model/UserSettings.h"
#include "ui/model/ChaosDefaultsListModel.h"

class ChaosDefaultsPage : public BasePage {
public:
    ChaosDefaultsPage(PageManager &manager, PageContext &context);

    using BasePage::show;
    void show(ChaosDefaultsListModel::Mode mode);

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;
    virtual bool isModal() const override { return true; }
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:
    uint16_t &targetMask();
    const char *activeFunction() const;
    bool targetEnabled(int index) const;
    void setTargetEnabled(int index, bool enabled);
    void setAllTargets(bool enabled);
    bool allTargetsEnabled() const;

    ChaosDefaultsListModel::Mode _mode = ChaosDefaultsListModel::Mode::Sequence;
    int _cursor = 0;
};
