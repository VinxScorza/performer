#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"
#include "ui/model/LogicSequenceListModel.h"

#include "engine/generators/Generator.h"
#include "engine/generators/SequenceBuilder.h"
#include "ui/KeyPressEventTracker.h"

#include "core/utils/Container.h"

class EvalStep {
    public:
        EvalStep() {}

        int stepIndex() { return _stepIndex;}
        bool logicStep() { return _logicStep;}
        bool input1Step() { return _input1Step;}
        bool input2Step() { return _input2Step;}

        void setStepIndex(int value) { _stepIndex = value; }
        void setLogicStep(bool value) {_logicStep = value; }
        void setInput1Step(bool value) {_input1Step = value; }
        void setInput2Step(bool value) {_input2Step = value; }


    private:
        uint8_t _stepIndex;
        bool _logicStep = false;
        bool _input1Step = false;
        bool _input2Step = false; 
};

class LogicSequenceEditPage : public BasePage {
public:
    LogicSequenceEditPage(PageManager &manager, PageContext &context);

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
    void launchpadUndo();
    void setLaunchpadGeneratorModeActive(bool active) {
        _launchpadGeneratorModeActive = active;
    }
    bool launchpadGeneratorModeActive() const { return _launchpadGeneratorModeActive; }

private:
    typedef LogicSequence::Layer Layer;

    static const int StepCount = 16;

    int stepOffset() const { return _project.selectedLogicSequence().section() * StepCount; }

    void switchLayer(int functionKey, bool shift);
    int activeFunctionKey();

    void updateMonitorStep();
    void drawDetail(Canvas &canvas, const LogicSequence::Step &step);
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
    bool isSectionTracking();
    void toggleSectionTracking();

    LogicSequence::Layer layer() const { return _project.selectedLogicSequenceLayer(); };
    void setLayer(LogicSequence::Layer layer) { _project.setSelectedLogicSequenceLayer(layer); }

    bool _showDetail;
    uint32_t _showDetailTicks;

    KeyPressEventTracker _keyPressEventTracker;


    LogicSequenceListModel _listModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    Container<LogicSequenceBuilder> _builderContainer;

    LogicSequence _inMemorySequence;
    bool _launchpadGeneratorModeActive = false;
};
