#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"
#include "ui/model/StochasticSequenceListModel.h"

#include "engine/generators/Generator.h"
#include "engine/generators/SequenceBuilder.h"
#include "ui/KeyPressEventTracker.h"

#include "core/utils/Container.h"

class StochasticSequenceEditPage : public BasePage {
public:
    StochasticSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;
    virtual void midi(MidiEvent &event) override;
    void openLaunchpadGenerator(Generator::Mode mode);
    void setLaunchpadGeneratorModeActive(bool active) {
        _launchpadGeneratorModeActive = active;
    }
    bool launchpadGeneratorModeActive() const { return _launchpadGeneratorModeActive; }

private:
    typedef StochasticSequence::Layer Layer;

    static const int StepCount = 16;

    int stepOffset() const { return _section * StepCount; }

    void switchLayer(int functionKey, bool shift);
    int activeFunctionKey();

    void updateMonitorStep();
    void drawDetail(Canvas &canvas, const StochasticSequence::Step &step);
    void drawLaunchpadGeneratorOverlay(Canvas &canvas);

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initSequence();
    void copySequence();
    void pasteSequence();
    void duplicateSequence();
    void tieNotes();
    void generateSequence();

    void quickEdit(int index);

    bool allSelectedStepsActive() const;
    void setSelectedStepsGate(bool gate);

    void setSectionTracking(bool track);

    void displayMessage(StochasticSequence &sequence);

    StochasticSequence::Layer layer() const { return _project.selectedStochasticSequenceLayer(); };
    void setLayer(StochasticSequence::Layer layer) { _project.setSelectedStochasticSequenceLayer(layer); }

    int _section = 0;
    bool _sectionTracking = false;
    bool _showDetail;
    uint32_t _showDetailTicks;

    KeyPressEventTracker _keyPressEventTracker;


    StochasticSequenceListModel _listModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    Container<StochasticSequenceBuilder> _builderContainer;
    bool _launchpadGeneratorModeActive = false;

};
