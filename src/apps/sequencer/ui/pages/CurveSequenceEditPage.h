#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"
#include "ui/model/CurveSequenceListModel.h"

#include "engine/generators/Generator.h"
#include "engine/generators/SequenceBuilder.h"

#include "core/utils/Container.h"

class CurveSequenceEditPage : public BasePage {
public:
    CurveSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;
    void openLaunchpadGenerator(Generator::Mode mode);
    void launchpadUndo();
    void setLaunchpadGeneratorModeActive(bool active) {
        _launchpadGeneratorModeActive = active;
    }
    bool launchpadGeneratorModeActive() const { return _launchpadGeneratorModeActive; }

private:
    typedef CurveSequence::Layer Layer;

    static const int StepCount = 16;

    int stepOffset() const { return _project.selectedCurveSequence().section() * StepCount; }

    void switchLayer(int functionKey, bool shift);
    int activeFunctionKey();

    void updateMonitorStep();

    void drawDetail(Canvas &canvas, const CurveSequence::Step &step);
    void drawLaunchpadGeneratorOverlay(Canvas &canvas);

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initSequence();
    void copySequence();
    void pasteSequence();
    void duplicateSequence();
    void generateSequence();

    void quickEdit(int index);

    CurveSequence::Layer layer() const { return _project.selectedCurveSequenceLayer(); }
    void setLayer(CurveSequence::Layer layer) { _project.setSelectedCurveSequenceLayer(layer); }

    ContextMenu _contextMenu;

    bool _showDetail;
    uint32_t _showDetailTicks;

    CurveSequenceListModel _listModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    Container<CurveSequenceBuilder> _builderContainer;

    CurveSequence _inMemorySequence;
    bool _launchpadGeneratorModeActive = false;
};
