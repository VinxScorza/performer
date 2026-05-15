#pragma once

#include "BasePage.h"
#include "ui/StepSelection.h"
#include "ui/pages/ContextMenu.h"

class Generator;
class AcidGenerator;
class ChaosGenerator;
class ChaosEntropyGenerator;
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
    void invalidateChaosPreview(bool fromEdit = false);
    void triggerChaosPreview();
    void launchpadRandomize();
    void showPreviewStateMessage();
    bool launchpadShowingPreview() const;
    bool launchpadResetState() const;
    bool launchpadTrackRetargetLocked() const;

private:
    bool boundTrackContextValid() const;
    bool ensureBoundTrackContext();

    void drawEuclideanGenerator(Canvas &canvas, const EuclideanGenerator &generator) const;
    void drawRandomGenerator(Canvas &canvas, const RandomGenerator &generator) const;
    void drawAcidGenerator(Canvas &canvas, const AcidGenerator &generator) const;
    void drawChaosGenerator(Canvas &canvas, const ChaosGenerator &generator) const;
    void drawChaosEntropyGenerator(Canvas &canvas, const ChaosEntropyGenerator &generator) const;
    int contextItemCount() const;
    int previewStepCount() const;
    int currentStep() const;
    bool stepInCurrentBank(int step) const;

    Generator *_generator;

    std::pair<uint8_t, uint8_t> _valueRange;
    StepSelection<CONFIG_STEP_COUNT> *_stepSelection;
    int _section = 0;
    int _chaosCursor = 0;
    bool _previewArmed = false;
    bool _launchpadResetState = false;
    bool _applied = false;
    int _boundTrackIndex = -1;
    Track::TrackMode _boundTrackMode = Track::TrackMode::Note;
    char _contextMenuAuxLabel[16] = "";
    char _variationMenuLabel[16] = "VAR";
    ContextMenuModel::Item _contextMenuItems[5];
};
