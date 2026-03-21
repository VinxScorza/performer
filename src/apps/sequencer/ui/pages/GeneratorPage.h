#pragma once

#include "BasePage.h"
#include "ui/StepSelection.h"
#include "ui/pages/ContextMenu.h"
#include "engine/generators/SequenceBuilder.h"

class Generator;
class AcidGenerator;
class EuclideanGenerator;
class RandomGenerator;

class GeneratorPage : public BasePage {
public:
    GeneratorPage(PageManager &manager, PageContext &context);

    using BasePage::show;
    void show(Generator *generator, StepSelection<CONFIG_STEP_COUNT> *_stepSelection);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual bool isModal() const override { return true; }

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

        static const int StepCount = 16;

    int stepOffset() const { return _section * StepCount; }

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;
    void init();
    void revert();
    void commit();
    void togglePreview();

private:
    void drawEuclideanGenerator(Canvas &canvas, const EuclideanGenerator &generator) const;
    void drawRandomGenerator(Canvas &canvas, const RandomGenerator &generator) const;
    void drawAcidGenerator(Canvas &canvas, const AcidGenerator &generator) const;
    int contextItemCount() const;

    Generator *_generator;

    std::pair<uint8_t, uint8_t> _valueRange;
    StepSelection<CONFIG_STEP_COUNT> *_stepSelection;
    int _section = 0;
    bool _applied = false;
    char _variationMenuLabel[16] = "VAR";
    ContextMenuModel::Item _contextMenuItems[5];

    Container<NoteSequenceBuilder> _builderContainer;
};
