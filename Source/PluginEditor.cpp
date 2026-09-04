#include "PluginEditor.h"

using namespace zx;
using namespace zx::theme;

namespace
{
    constexpr int kDefaultWidth   = 940;
    constexpr int kDefaultHeight  = 660;
    constexpr int kFileBarHeight  = 54;
}

// ===========================================================================

ZinoxVocalsEditor::ZinoxVocalsEditor (ZinoxVocalsProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p),
      isStandaloneApp (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone),
      designHeight (kDefaultHeight + (isStandaloneApp ? kFileBarHeight : 0))
{
    setLookAndFeel (&lnf);

    buildFileBar();
    buildHeader();
    buildLeftColumn();
    buildCentre();
    buildRightColumn();

    processor.getPresetManager().addChangeListener (this);
    refreshPresetList();

    const auto& trial = processor.getTrialStatus();

    if (trial.expired && ! processor.getLicenseInfo().valid)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon, "Trial Ended",
            "Your 7-day free trial of Zinox Vocals has ended, and audio processing is "
            "now disabled.\n\nEnter a license key at any time to keep using it - click "
            "the badge in the top right.");
    }

    setResizable (true, true);
    setResizeLimits (kDefaultWidth * 3 / 4, designHeight * 3 / 4,
                     kDefaultWidth * 3 / 2, designHeight * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) kDefaultWidth / (double) designHeight);
    setSize (kDefaultWidth, designHeight);

    startTimerHz (30);
}

ZinoxVocalsEditor::~ZinoxVocalsEditor()
{
    processor.getPresetManager().removeChangeListener (this);
    setLookAndFeel (nullptr);
}

// ===========================================================================
//  Construction
// ===========================================================================

void ZinoxVocalsEditor::buildFileBar()
{
    if (! isStandaloneApp)
        return;

    fileDropZone = std::make_unique<zx::FileDropZone>();
    fileDropZone->onFileDropped = [this] (const juce::File& f) { setImportedFile (f); };
    addAndMakeVisible (*fileDropZone);

    importButton = std::make_unique<juce::TextButton> ("IMPORT");
    importButton->setTooltip ("Load an audio file to audition live or export.");
    importButton->onClick = [this] { chooseImportFile(); };
    addAndMakeVisible (*importButton);

    playButton = std::make_unique<juce::TextButton> ("PLAY");
    playButton->setTooltip ("Loop the imported file through the plugin and out to your speakers - "
                            "turn any knob and hear the change immediately.");
    playButton->setClickingTogglesState (true);
    playButton->setEnabled (false);
    playButton->onClick = [this] { togglePlayback(); };
    addAndMakeVisible (*playButton);

    exportButton = std::make_unique<juce::TextButton> ("EXPORT");
    exportButton->setTooltip ("Render the imported file through the current settings and save it.");
    exportButton->setEnabled (false);
    exportButton->onClick = [this] { chooseExportFile(); };
    addAndMakeVisible (*exportButton);
}

void ZinoxVocalsEditor::buildHeader()
{
    addAndMakeVisible (logo);

    presetBox.setTextWhenNothingSelected ("PRESETS");
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this]
    {
        const auto name = presetBox.getText();

        if (name.isNotEmpty() && name != processor.getPresetManager().getCurrentPresetName())
            processor.getPresetManager().loadPreset (name);
    };
    addAndMakeVisible (presetBox);

    prevPresetButton.setTooltip ("Previous preset");
    prevPresetButton.onClick = [this] { processor.getPresetManager().previousPreset(); };
    addAndMakeVisible (prevPresetButton);

    nextPresetButton.setTooltip ("Next preset");
    nextPresetButton.onClick = [this] { processor.getPresetManager().nextPreset(); };
    addAndMakeVisible (nextPresetButton);

    savePresetButton.setTooltip ("Save the current settings as a user preset");
    savePresetButton.onClick = [this] { showSaveDialog(); };
    addAndMakeVisible (savePresetButton);

    bypassButton.setClickingTogglesState (true);
    bypassButton.setTooltip ("Bypass the whole plugin");
    // The bypass veil is painted by the editor itself, so it has to repaint
    // when the state changes from anywhere — including host automation.
    bypassButton.onStateChange = [this] { repaint(); };
    addAndMakeVisible (bypassButton);
    bypassAttach = std::make_unique<ButtonAttach> (processor.getAPVTS(), ParamID::bypass, bypassButton);

    licenseBadge.setTooltip ("View license status or activate Zinox Vocals.");
    licenseBadge.onClick = [this] { showLicenseDialog(); };
    addAndMakeVisible (licenseBadge);
    updateLicenseBadge();
}

void ZinoxVocalsEditor::buildLeftColumn()
{
    addAndMakeVisible (inputKnob);
    inputKnob.attach (processor.getAPVTS(), ParamID::input);
    inputKnob.getSlider().setTooltip ("Level going into the strip. Set this so Control sees a healthy signal.");

    addAndMakeVisible (denoiseKnob);
    denoiseKnob.attach (processor.getAPVTS(), ParamID::denoise);
    denoiseKnob.getSlider().setTooltip ("Adaptive noise reduction. Continuously learns the background noise "
                                        "floor and subtracts it - most effective on steady noise "
                                        "like hiss, hum, fans and room tone.");

    addAndMakeVisible (lowCutKnob);
    lowCutKnob.attach (processor.getAPVTS(), ParamID::lowCut);
    lowCutKnob.getSlider().setTooltip ("24 dB/oct high-pass. Fully anticlockwise is OFF.");

    addAndMakeVisible (highCutKnob);
    highCutKnob.attach (processor.getAPVTS(), ParamID::highCut);
    highCutKnob.getSlider().setTooltip ("24 dB/oct low-pass. Fully clockwise is OFF.");
}

void ZinoxVocalsEditor::buildCentre()
{
    auto& apvts = processor.getAPVTS();

    auto setupCombo = [] (juce::ComboBox& box, const juce::StringArray& items)
    {
        box.addItemList (items, 1);
        box.setJustificationType (juce::Justification::centredLeft);
    };

    // --- tone row -----------------------------------------------------------
    addAndMakeVisible (lowKnob);
    lowKnob.attach (apvts, ParamID::lowGain);
    lowKnob.getSlider().setTooltip ("Low band gain, +/- 12 dB.");

    addAndMakeVisible (lowShapeSwitch);
    lowShapeSwitch.attach (apvts, ParamID::lowShape);

    addAndMakeVisible (lowFreqSlider);
    lowFreqSlider.attach (apvts, ParamID::lowFreq);
    lowFreqSlider.getSlider().setTooltip ("Centre/corner frequency of the low band.");

    addAndMakeVisible (midKnob);
    midKnob.attach (apvts, ParamID::midGain);
    midKnob.getSlider().setTooltip ("Mid band gain, +/- 12 dB.");

    addAndMakeVisible (midToneSlider);
    midToneSlider.attach (apvts, ParamID::midTone);
    midToneSlider.getSlider().setTooltip ("Sweeps the mid band from body (350 Hz) to bite (5 kHz).");

    addAndMakeVisible (highKnob);
    highKnob.attach (apvts, ParamID::highGain);
    highKnob.getSlider().setTooltip ("High band gain, +/- 12 dB.");

    setupCombo (highModeBox, Choices::highMode);
    highModeBox.setTooltip ("AIR: 11 kHz shelf. BRIGHT: 6.5 kHz shelf. PRESENCE: 3.8 kHz bell.");
    addAndMakeVisible (highModeBox);
    highModeAttach = std::make_unique<ComboAttach> (apvts, ParamID::highMode, highModeBox);

    addAndMakeVisible (deEssKnob);
    deEssKnob.attach (apvts, ParamID::deEss);
    deEssKnob.getSlider().setTooltip ("Split-band sibilance control. Threshold and ratio move together.");

    addAndMakeVisible (deEssMeter);

    addAndMakeVisible (deEssFreqSlider);
    deEssFreqSlider.attach (apvts, ParamID::deEssFreq);
    deEssFreqSlider.getSlider().setTooltip ("De-esser crossover frequency.");

    // --- dynamics row -------------------------------------------------------
    addAndMakeVisible (controlKnob);
    controlKnob.attach (apvts, ParamID::control);
    controlKnob.getSlider().setTooltip ("Main compressor. Turn up for more consistency.");

    addAndMakeVisible (controlRange);
    controlRange.attach (apvts, ParamID::controlRange);

    setupCombo (controlModeBox, Choices::controlMode);
    controlModeBox.setTooltip ("AGGRO: fast and obvious. SMOOTH: opto-style. VOCAL: balanced.");
    addAndMakeVisible (controlModeBox);
    controlModeAttach = std::make_unique<ComboAttach> (apvts, ParamID::controlMode, controlModeBox);

    addAndMakeVisible (pushKnob);
    pushKnob.attach (apvts, ParamID::push);
    pushKnob.getSlider().setTooltip ("Parallel density. Raises the floor without flattening transients.");

    addAndMakeVisible (pushRange);
    pushRange.attach (apvts, ParamID::pushRange);

    setupCombo (pushModeBox, Choices::pushMode);
    pushModeBox.setTooltip ("PUNCH: keeps consonants. FAT: maximum sustain. TIGHT: aggressive.");
    addAndMakeVisible (pushModeBox);
    pushModeAttach = std::make_unique<ComboAttach> (apvts, ParamID::pushMode, pushModeBox);

    addAndMakeVisible (saturateKnob);
    saturateKnob.attach (apvts, ParamID::saturate);
    saturateKnob.getSlider().setTooltip ("Harmonic saturation, running inside the oversampled section.");

    setupCombo (satModeBox, Choices::satMode);
    satModeBox.setTooltip ("RICH, WARM, TAPE and TUBE each generate a different harmonic mix.");
    addAndMakeVisible (satModeBox);
    satModeAttach = std::make_unique<ComboAttach> (apvts, ParamID::satMode, satModeBox);

    addAndMakeVisible (outputKnob);
    outputKnob.attach (apvts, ParamID::output);
    outputKnob.getSlider().setTooltip ("Output trim, applied before the limiter.");

    addAndMakeVisible (mixSlider);
    mixSlider.attach (apvts, ParamID::mix);
    mixSlider.getSlider().setTooltip ("Dry/wet blend across the whole strip.");
}

void ZinoxVocalsEditor::buildRightColumn()
{
    auto& apvts = processor.getAPVTS();

    addAndMakeVisible (outMeter);
    addAndMakeVisible (limitMeter);

    limitCeiling.setTooltip ("Limiter ceiling in dBFS.");
    addAndMakeVisible (limitCeiling);
    limitCeilingAttach = std::make_unique<SliderAttach> (apvts, ParamID::limit, limitCeiling);

    limitOnButton.setClickingTogglesState (true);
    limitOnButton.setTooltip ("Enable the look-ahead brickwall limiter.");
    addAndMakeVisible (limitOnButton);
    limitOnAttach = std::make_unique<ButtonAttach> (apvts, ParamID::limitOn, limitOnButton);

    oversampleBox.addItemList (Choices::oversample, 1);
    oversampleBox.setJustificationType (juce::Justification::centred);
    oversampleBox.setTooltip ("Oversampling for the Push and Saturate stages. Higher is cleaner but costs CPU.");
    addAndMakeVisible (oversampleBox);
    oversampleAttach = std::make_unique<ComboAttach> (apvts, ParamID::oversample, oversampleBox);
}

// ===========================================================================
//  Presets
// ===========================================================================

void ZinoxVocalsEditor::refreshPresetList()
{
    auto& pm = processor.getPresetManager();

    presetBox.clear (juce::dontSendNotification);

    const auto names = pm.getAllPresetNames();
    int id = 1;

    for (const auto& n : names)
        presetBox.addItem (n, id++);

    const auto idx = pm.getCurrentIndex();

    if (idx >= 0)
        presetBox.setSelectedItemIndex (idx, juce::dontSendNotification);
    else
        presetBox.setText (pm.getCurrentPresetName(), juce::dontSendNotification);
}

// ===========================================================================
//  Standalone file import/export
// ===========================================================================

void ZinoxVocalsEditor::setImportedFile (const juce::File& file)
{
    const bool loaded = file.existsAsFile() && processor.loadFileForPlayback (file);

    importedFile = loaded ? file : juce::File();

    if (fileDropZone != nullptr)
        fileDropZone->setLoadedFile (importedFile);

    if (exportButton != nullptr)
        exportButton->setEnabled (loaded);

    if (playButton != nullptr)
    {
        playButton->setEnabled (loaded);
        playButton->setToggleState (false, juce::dontSendNotification);
        updatePlayButton();
    }

    if (file.existsAsFile() && ! loaded)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon, "Couldn't Load File",
            "\"" + file.getFileName() + "\" doesn't look like a supported audio file.");
    }
}

void ZinoxVocalsEditor::togglePlayback()
{
    if (playButton == nullptr)
        return;

    const bool shouldPlay = playButton->getToggleState();
    processor.setFilePlaying (shouldPlay);
    updatePlayButton();
}

void ZinoxVocalsEditor::updatePlayButton()
{
    if (playButton == nullptr)
        return;

    playButton->setButtonText (processor.isFilePlaying() ? "STOP" : "PLAY");
}

void ZinoxVocalsEditor::chooseImportFile()
{
    activeFileChooser = std::make_unique<juce::FileChooser> (
        "Choose an audio file to import", juce::File(),
        "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg;*.m4a;*.wma");

    activeFileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();

            if (f.existsAsFile())
                setImportedFile (f);
        });
}

void ZinoxVocalsEditor::chooseExportFile()
{
    if (! importedFile.existsAsFile())
        return;

    if (processor.getTrialStatus().expired && ! processor.getLicenseInfo().valid)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon, "Trial Ended",
            "Your free trial has ended, so export is disabled. Enter a license key to "
            "continue - click the badge in the top right.");
        return;
    }

    const auto suggested = importedFile.getSiblingFile (
        importedFile.getFileNameWithoutExtension() + " (Zinox Vocals).wav");

    activeFileChooser = std::make_unique<juce::FileChooser> (
        "Export processed audio as...", suggested, "*.wav");

    activeFileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();

            if (f != juce::File())
                runExport (f);
        });
}

void ZinoxVocalsEditor::runExport (const juce::File& destFile)
{
    juce::MemoryBlock state;
    processor.getStateInformation (state);

    // Reports the result and deletes itself once rendering finishes -
    // nothing to hold onto here.
    auto* renderer = new zx::OfflineRenderer (importedFile, destFile, state);
    renderer->launchThread();
}

void ZinoxVocalsEditor::showSaveDialog()
{
    if (saveWindow != nullptr)
        return;

    saveWindow = std::make_unique<juce::AlertWindow> ("SAVE PRESET",
                                                      "Name this preset:",
                                                      juce::MessageBoxIconType::NoIcon,
                                                      this);

    saveWindow->addTextEditor ("name", processor.getPresetManager().getCurrentPresetName(), "Name:");
    saveWindow->addButton ("SAVE",   1, juce::KeyPress (juce::KeyPress::returnKey));
    saveWindow->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    saveWindow->centreAroundComponent (this, saveWindow->getWidth(), saveWindow->getHeight());
    saveWindow->setVisible (true);

    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this] (int result)
        {
            if (saveWindow != nullptr && result == 1)
            {
                const auto name = saveWindow->getTextEditorContents ("name").trim();

                if (name.isNotEmpty())
                    processor.getPresetManager().savePreset (name);
            }

            // Deleting from inside the callback is safe — the modal loop has
            // already finished by the time this runs.
            saveWindow.reset();
        }), false);
}

void ZinoxVocalsEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetList();
}

// ===========================================================================
//  Licensing
// ===========================================================================

void ZinoxVocalsEditor::updateLicenseBadge()
{
    const auto& info  = processor.getLicenseInfo();
    const auto& trial = processor.getTrialStatus();

    if (info.valid)
    {
        licenseBadge.setButtonText (info.hasExpiry
            ? "LICENSED \xC2\xB7 UNTIL " + info.expiresAt.formatted ("%d %b %Y").toUpperCase()
            : juce::String ("LICENSED"));
        licenseBadge.setColour (juce::TextButton::textColourOffId, textDim);
    }
    else if (info.expired)
    {
        licenseBadge.setButtonText ("SUBSCRIPTION EXPIRED");
        licenseBadge.setColour (juce::TextButton::textColourOffId, meterRed);
    }
    else if (trial.expired)
    {
        licenseBadge.setButtonText ("TRIAL EXPIRED");
        licenseBadge.setColour (juce::TextButton::textColourOffId, meterRed);
    }
    else
    {
        licenseBadge.setButtonText ("TRIAL: " + juce::String (trial.daysRemaining) + "D LEFT");
        licenseBadge.setColour (juce::TextButton::textColourOffId, gold);
    }
}

void ZinoxVocalsEditor::showLicenseDialog()
{
    if (licenseWindow != nullptr)
        return;

    const auto& info  = processor.getLicenseInfo();
    const auto& trial = processor.getTrialStatus();

    juce::String message;

    if (info.valid)
    {
        message = "Licensed to " + info.name + " <" + info.email + ">"
                  + (info.hasExpiry ? " until " + info.expiresAt.formatted ("%d %b %Y") + "." : juce::String ("."))
                  + "\n\nPaste a different key below to replace it.";
    }
    else if (info.expired)
    {
        message = "The subscription for " + info.name + " <" + info.email + "> expired "
                  + info.expiresAt.formatted ("%d %b %Y") + ", and audio processing is disabled.\n\n"
                  "Paste a renewed license key below to keep using Zinox Vocals.";
    }
    else if (trial.expired)
    {
        message = "Your 7-day free trial has ended and audio processing is disabled.\n\n"
                  "Paste your license key below to keep using Zinox Vocals. If you were "
                  "sent a machine-locked key, it must match the Machine ID shown below.";
    }
    else
    {
        message = "Trial mode - " + juce::String (trial.daysRemaining) + " day"
                  + juce::String (trial.daysRemaining == 1 ? "" : "s") + " remaining, full quality.\n\n"
                  "Paste a license key below at any time to keep using it after the trial "
                  "ends. If you were sent a machine-locked key, it must match the Machine "
                  "ID shown below.";
    }

    licenseWindow = std::make_unique<juce::AlertWindow> ("ZINOX VOCALS LICENSE", message,
                                                         juce::MessageBoxIconType::NoIcon, this);

    licenseWindow->addTextEditor ("machineId", zx::License::getMachineId(), "This machine's ID:");
    licenseWindow->addTextEditor ("key", "", "License key:");
    licenseWindow->addButton ("ACTIVATE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    licenseWindow->addButton ("CLOSE", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    licenseWindow->centreAroundComponent (this, licenseWindow->getWidth(), licenseWindow->getHeight());
    licenseWindow->setVisible (true);

    licenseWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this] (int result)
        {
            if (licenseWindow != nullptr && result == 1)
            {
                const auto key = licenseWindow->getTextEditorContents ("key").trim();

                if (key.isNotEmpty())
                {
                    const auto newInfo = processor.activateLicense (key);
                    updateLicenseBadge();

                    const auto title = newInfo.valid ? "Activated"
                                      : newInfo.expired ? "Key Expired"
                                                        : "Invalid Key";
                    const auto body = newInfo.valid
                        ? "Thanks, " + newInfo.name + " - Zinox Vocals is now licensed on this machine."
                        : newInfo.expired
                            ? "That key was valid but expired " + newInfo.expiresAt.formatted ("%d %b %Y")
                              + ". Ask for a renewed one."
                            : juce::String ("That key didn't verify. Double-check it was copied in full "
                                            "and that it matches this machine's ID.");

                    juce::AlertWindow::showMessageBoxAsync (
                        newInfo.valid ? juce::MessageBoxIconType::InfoIcon
                                     : juce::MessageBoxIconType::WarningIcon,
                        title, body);
                }
            }

            licenseWindow.reset();
        }), false);
}

// ===========================================================================
//  Timer — pull the meter values across from the audio thread
// ===========================================================================

void ZinoxVocalsEditor::timerCallback()
{
    outMeter.setLevels (processor.meters.outLeft.load(), processor.meters.outRight.load());
    limitMeter.setGainReduction (processor.meters.limitGr.load());
    deEssMeter.setGainReduction (processor.meters.deEssGr.load());

    // The trial check touches disk, so it runs on this UI timer rather than
    // every audio block - every 5 seconds (at 30 Hz) is plenty responsive
    // for a boundary measured in days.
    if (++trialCheckCounter >= 150)
    {
        trialCheckCounter = 0;
        const bool wasTrialExpired   = processor.getTrialStatus().expired;
        const bool wasLicenseExpired = processor.getLicenseInfo().expired;

        processor.refreshTrialStatus();
        processor.refreshLicenseStatus();
        updateLicenseBadge();

        const auto& license = processor.getLicenseInfo();

        if (! wasLicenseExpired && license.expired)
        {
            repaint();
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Subscription Expired",
                "Your Zinox Vocals subscription has just expired, and audio "
                "processing is now disabled.\n\nRenew and enter a new license key "
                "to keep using it - click the badge in the top right.");
        }
        else if (! wasTrialExpired && processor.getTrialStatus().expired && ! license.valid)
        {
            repaint();
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Trial Ended",
                "Your 7-day free trial of Zinox Vocals has just ended, and audio "
                "processing is now disabled.\n\nEnter a license key at any time to "
                "keep using it - click the badge in the top right.");
        }
    }
}

// ===========================================================================
//  Layout
// ===========================================================================

void ZinoxVocalsEditor::resized()
{
    // Everything is laid out at the design size and then scaled, so the panel
    // keeps its proportions at any window size.
    const auto scale = (float) getWidth() / (float) kDefaultWidth;

    auto full = juce::Rectangle<int> (0, 0, kDefaultWidth, designHeight);

    auto applyScale = [scale] (juce::Component& c, juce::Rectangle<int> r)
    {
        c.setBounds ((int) std::round (r.getX() * scale),
                     (int) std::round (r.getY() * scale),
                     (int) std::round (r.getWidth() * scale),
                     (int) std::round (r.getHeight() * scale));
    };

    auto body = full.reduced (16, 14);

    // --- file import/export bar (standalone only) ----------------------------
    if (isStandaloneApp)
    {
        fileBarArea = body.removeFromTop (kFileBarHeight);
        body.removeFromTop (10);

        auto bar = fileBarArea.reduced (0, 6);

        applyScale (*importButton, bar.removeFromLeft (86).reduced (0, 3));
        bar.removeFromLeft (8);
        applyScale (*exportButton, bar.removeFromRight (86).reduced (0, 3));
        bar.removeFromRight (8);
        applyScale (*playButton, bar.removeFromRight (70).reduced (0, 3));
        bar.removeFromRight (8);
        applyScale (*fileDropZone, bar);
    }

    // --- header -------------------------------------------------------------
    headerArea = body.removeFromTop (52);
    {
        auto h = headerArea;

        applyScale (logo, h.removeFromLeft (170).withSizeKeepingCentre (170, 34));

        applyScale (bypassButton, h.removeFromRight (84).withSizeKeepingCentre (78, 26));
        h.removeFromRight (6);
        applyScale (licenseBadge, h.removeFromRight (100).withSizeKeepingCentre (94, 24));
        h.removeFromRight (10);
        titleArea = h.removeFromRight (150);
        h.removeFromRight (10);

        // Preset bar sits in the middle of whatever is left.
        auto bar = h.withSizeKeepingCentre (juce::jmin (320, h.getWidth()), 30);

        applyScale (prevPresetButton, bar.removeFromLeft (30).reduced (1));
        bar.removeFromLeft (4);
        applyScale (savePresetButton, bar.removeFromRight (54).reduced (1));
        bar.removeFromRight (4);
        applyScale (nextPresetButton, bar.removeFromRight (30).reduced (1));
        bar.removeFromRight (4);
        applyScale (presetBox, bar);
    }

    body.removeFromTop (10);

    // --- footer -------------------------------------------------------------
    footerArea = body.removeFromBottom (76);
    body.removeFromBottom (10);

    // --- three columns ------------------------------------------------------
    leftPanelArea  = body.removeFromLeft (108);
    body.removeFromLeft (10);
    rightPanelArea = body.removeFromRight (156);
    body.removeFromRight (10);
    centrePanelArea = body;

    // --- left column --------------------------------------------------------
    {
        auto col = leftPanelArea.reduced (8, 16);
        const auto cellH = col.getHeight() / 4;

        applyScale (inputKnob,   col.removeFromTop (cellH).reduced (4, 2));
        applyScale (denoiseKnob, col.removeFromTop (cellH).reduced (4, 2));
        applyScale (lowCutKnob,  col.removeFromTop (cellH).reduced (4, 2));
        applyScale (highCutKnob, col.removeFromTop (cellH).reduced (4, 2));
    }

    // --- centre -------------------------------------------------------------
    {
        auto inner = centrePanelArea.reduced (12, 14);

        toneRowArea = inner.removeFromTop (inner.getHeight() / 2);
        inner.removeFromTop (8);
        dynRowArea = inner;

        const auto colW = toneRowArea.getWidth() / 4;

        // ---- tone row ------------------------------------------------------
        {
            auto row = toneRowArea;

            // LOW: lever switch to the left of the knob, frequency underneath.
            {
                auto cell = row.removeFromLeft (colW).reduced (4, 0);
                auto sliderRow = cell.removeFromBottom (30);
                applyScale (lowFreqSlider, sliderRow.reduced (10, 2));

                auto lever = cell.removeFromLeft (34).withSizeKeepingCentre (34, 72);
                lever.setY (cell.getY() + 30);
                applyScale (lowShapeSwitch, lever);
                applyScale (lowKnob, cell);
            }

            // MID
            {
                auto cell = row.removeFromLeft (colW).reduced (4, 0);
                auto sliderRow = cell.removeFromBottom (30);
                applyScale (midToneSlider, sliderRow.reduced (10, 2));
                applyScale (midKnob, cell);
            }

            // HIGH
            {
                auto cell = row.removeFromLeft (colW).reduced (4, 0);
                auto comboRow = cell.removeFromBottom (30);
                applyScale (highModeBox, comboRow.withSizeKeepingCentre (86, 22));
                applyScale (highKnob, cell);
            }

            // DE-ESS: knob with its gain-reduction meter stood up beside it.
            {
                auto cell = row.reduced (4, 0);
                auto sliderRow = cell.removeFromBottom (30);
                applyScale (deEssFreqSlider, sliderRow.reduced (10, 2));

                auto meterCol = cell.removeFromRight (18);
                meterCol = meterCol.withSizeKeepingCentre (9, 88);
                meterCol.setY (cell.getY() + 26);
                applyScale (deEssMeter, meterCol);

                applyScale (deEssKnob, cell);
            }
        }

        // ---- dynamics row --------------------------------------------------
        {
            auto row = dynRowArea;

            auto placeWithRange = [&] (ZinoxKnob& knob, RangeSwitch& range, juce::ComboBox& combo,
                                       juce::Rectangle<int> cell)
            {
                auto comboRow = cell.removeFromBottom (30);
                applyScale (combo, comboRow.withSizeKeepingCentre (86, 22));

                auto rangeCol = cell.removeFromRight (34);
                rangeCol = rangeCol.withSizeKeepingCentre (34, 82);
                rangeCol.setY (cell.getY() + 26);
                applyScale (range, rangeCol);

                applyScale (knob, cell);
            };

            placeWithRange (controlKnob, controlRange, controlModeBox,
                            row.removeFromLeft (colW).reduced (4, 0));

            placeWithRange (pushKnob, pushRange, pushModeBox,
                            row.removeFromLeft (colW).reduced (4, 0));

            {
                auto cell = row.removeFromLeft (colW).reduced (4, 0);
                auto comboRow = cell.removeFromBottom (30);
                applyScale (satModeBox, comboRow.withSizeKeepingCentre (86, 22));
                applyScale (saturateKnob, cell);
            }

            {
                auto cell = row.reduced (4, 0);
                auto sliderRow = cell.removeFromBottom (30);
                applyScale (mixSlider, sliderRow.reduced (10, 2));
                applyScale (outputKnob, cell);
            }
        }
    }

    // --- right column -------------------------------------------------------
    {
        auto col = rightPanelArea.reduced (12, 14);

        col.removeFromTop (22);                       // room for the OUT / LIMIT captions
        auto controls = col.removeFromBottom (74);

        auto meterRow = col.reduced (4, 4);

        auto outArea = meterRow.removeFromLeft (juce::roundToInt (meterRow.getWidth() * 0.52f));
        applyScale (outMeter, outArea.reduced (6, 0));

        meterRow.removeFromLeft (6);

        applyScale (limitMeter, meterRow.removeFromLeft (12));
        meterRow.removeFromLeft (6);
        applyScale (limitCeiling, meterRow.reduced (2, 0));

        applyScale (limitOnButton, controls.removeFromTop (24).reduced (6, 1));
        controls.removeFromTop (18);                  // room for the OVERSAMPLING caption
        applyScale (oversampleBox, controls.removeFromTop (24).reduced (14, 1));
    }
}

// ===========================================================================
//  Painting
// ===========================================================================

void ZinoxVocalsEditor::paintBackground (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    juce::ColourGradient bg (backgroundTop, r.getCentreX(), 0.0f,
                             backgroundBottom, r.getCentreX(), r.getBottom(), false);
    bg.addColour (0.45, backgroundTop.interpolatedWith (backgroundBottom, 0.35f));
    g.setGradientFill (bg);
    g.fillAll();

    // A wide, very soft gold wash behind the centre lifts the whole panel and
    // keeps the darkest corners from looking flat.
    juce::ColourGradient wash (gold.withAlpha (0.055f), r.getCentreX(), r.getHeight() * 0.32f,
                               juce::Colours::transparentBlack, r.getCentreX(), r.getHeight() * 0.95f,
                               true);
    g.setGradientFill (wash);
    g.fillAll();
}

void ZinoxVocalsEditor::paintFooter (juce::Graphics& g)
{
    const auto scale = (float) getWidth() / (float) kDefaultWidth;
    auto r = (footerArea.toFloat() * scale);

    g.setColour (panelOutline.withAlpha (0.45f));
    g.drawLine (r.getX(), r.getY(), r.getRight(), r.getY(), 1.0f);

    r.removeFromTop (10.0f * scale);

    auto markArea = r.removeFromLeft (150.0f * scale);

    // Small wordmark on the left of the footer.
    g.setFont (labelFont (15.0f * scale));
    g.setColour (text);
    drawTracked (g, "ZINOX", markArea.removeFromTop (r.getHeight() * 0.5f).toNearestInt(),
                 juce::Justification::left, 3.0f);
    g.setFont (labelFont (10.0f * scale, false));
    g.setColour (gold);
    drawTracked (g, "AUDIO", markArea.toNearestInt(), juce::Justification::left, 4.0f);

    r.removeFromLeft (10.0f * scale);
    g.setColour (panelOutline.withAlpha (0.5f));
    g.drawLine (r.getX(), r.getY() + 4.0f * scale, r.getX(), r.getBottom() - 10.0f * scale, 1.0f);
    r.removeFromLeft (14.0f * scale);

    const std::array<std::pair<const char*, const char*>, 4> features
    {{
        { "CLEAN &",   "NATURAL"   },
        { "POWERFUL",  "CONTROL"   },
        { "STUDIO",    "QUALITY"   },
        { "FAST &",    "EFFICIENT" },
    }};

    const auto cellW = r.getWidth() / (float) features.size();

    for (size_t i = 0; i < features.size(); ++i)
    {
        auto cell = r.removeFromLeft (cellW);
        auto icon = cell.removeFromLeft (30.0f * scale).withSizeKeepingCentre (24.0f * scale, 24.0f * scale);

        // Each feature gets a simple geometric glyph drawn in the accent colour.
        g.setColour (gold);
        const auto c = icon.getCentre();
        const auto s = icon.getWidth() * 0.5f;

        switch (i)
        {
            case 0:   // microphone
                g.fillRoundedRectangle (c.x - s * 0.3f, c.y - s * 0.85f, s * 0.6f, s * 1.0f, s * 0.3f);
                g.drawEllipse (c.x - s * 0.55f, c.y - s * 0.35f, s * 1.1f, s * 0.9f, 1.6f);
                g.drawLine (c.x, c.y + s * 0.55f, c.x, c.y + s * 0.9f, 1.6f);
                break;

            case 1:   // waveform
            {
                juce::Path wave;
                const int n = 7;
                for (int k = 0; k < n; ++k)
                {
                    const auto x = c.x - s * 0.8f + (float) k * (s * 1.6f / (float) (n - 1));
                    const auto h = s * (0.25f + 0.75f * std::abs (std::sin ((float) k * 1.1f)));
                    wave.addRoundedRectangle (x - 1.2f, c.y - h * 0.5f, 2.4f, h, 1.2f);
                }
                g.fillPath (wave);
                break;
            }

            case 2:   // bar chart
                for (int k = 0; k < 3; ++k)
                {
                    const auto h = s * (0.5f + 0.25f * (float) k);
                    g.fillRoundedRectangle (c.x - s * 0.8f + (float) k * s * 0.6f,
                                            c.y + s * 0.7f - h, s * 0.36f, h, 1.5f);
                }
                break;

            default:  // lightning bolt
            {
                juce::Path bolt;
                bolt.startNewSubPath (c.x + s * 0.25f, c.y - s * 0.9f);
                bolt.lineTo (c.x - s * 0.5f, c.y + s * 0.15f);
                bolt.lineTo (c.x - s * 0.02f, c.y + s * 0.15f);
                bolt.lineTo (c.x - s * 0.25f, c.y + s * 0.9f);
                bolt.lineTo (c.x + s * 0.5f, c.y - s * 0.15f);
                bolt.lineTo (c.x + s * 0.02f, c.y - s * 0.15f);
                bolt.closeSubPath();
                g.fillPath (bolt);
                break;
            }
        }

        cell.removeFromLeft (6.0f * scale);
        g.setFont (labelFont (9.5f * scale));
        g.setColour (text);
        drawTracked (g, features[i].first,
                     cell.removeFromTop (cell.getHeight() * 0.5f).toNearestInt(),
                     juce::Justification::left, 1.2f);
        g.setColour (textDim);
        drawTracked (g, features[i].second, cell.toNearestInt(), juce::Justification::left, 1.2f);
    }
}

void ZinoxVocalsEditor::paint (juce::Graphics& g)
{
    const auto scale = (float) getWidth() / (float) kDefaultWidth;

    paintBackground (g);

    // --- product name on the right ------------------------------------------
    {
        auto t = (titleArea.toFloat() * scale);
        auto lower = t.removeFromBottom (t.getHeight() * 0.42f);

        g.setFont (labelFont (15.0f * scale));
        g.setColour (goldBright);
        drawTracked (g, "ZINOX VOCALS", t.toNearestInt(), juce::Justification::right, 2.2f);

        g.setFont (labelFont (8.5f * scale, false));
        g.setColour (textFaint);
        drawTracked (g, "VOCAL CHANNEL STRIP", lower.toNearestInt(),
                     juce::Justification::right, 1.6f);
    }

    // --- panels -------------------------------------------------------------
    if (isStandaloneApp)
        drawPanel (g, fileBarArea.toFloat() * scale, 10.0f * scale);

    drawPanel (g, leftPanelArea.toFloat() * scale, 12.0f * scale);
    drawPanel (g, centrePanelArea.toFloat() * scale, 12.0f * scale);
    drawPanel (g, rightPanelArea.toFloat() * scale, 12.0f * scale);

    // Divider between the tone row and the dynamics row.
    {
        auto t = (toneRowArea.toFloat() * scale);
        auto d = (dynRowArea.toFloat() * scale);
        const auto y = (t.getBottom() + d.getY()) * 0.5f;

        juce::ColourGradient line (juce::Colours::transparentBlack, t.getX(), y,
                                   juce::Colours::transparentBlack, t.getRight(), y, false);
        line.addColour (0.5, gold.withAlpha (0.35f));
        g.setGradientFill (line);
        g.fillRect (juce::Rectangle<float> (t.getX(), y, t.getWidth(), 1.0f));
    }

    // --- right panel captions -----------------------------------------------
    {
        auto col = (rightPanelArea.toFloat() * scale).reduced (12.0f * scale, 14.0f * scale);

        auto caps = col.removeFromTop (22.0f * scale);
        g.setFont (labelFont (10.5f * scale));

        g.setColour (text);
        drawTracked (g, "OUT", caps.removeFromLeft (caps.getWidth() * 0.52f).toNearestInt(),
                     juce::Justification::centred, 1.6f);
        g.setColour (gold);
        drawTracked (g, "LIMIT", caps.toNearestInt(), juce::Justification::centred, 1.6f);

        auto controls = col.removeFromBottom (74.0f * scale);
        controls.removeFromTop (24.0f * scale);
        g.setFont (labelFont (8.5f * scale));
        g.setColour (textDim);
        drawTracked (g, "OVERSAMPLING", controls.removeFromTop (18.0f * scale).toNearestInt(),
                     juce::Justification::centred, 1.4f);
    }

    paintFooter (g);

    // A dimming veil makes it unmistakable when the plugin is bypassed.
    if (processor.getAPVTS().getRawParameterValue (ParamID::bypass)->load() > 0.5f)
    {
        g.setColour (backgroundBottom.withAlpha (0.55f));
        g.fillAll();
    }

    // Same treatment, stronger message, when the trial has run out or a
    // subscription key has lapsed.
    const auto& licenseInfo = processor.getLicenseInfo();
    if ((processor.getTrialStatus().expired || licenseInfo.expired) && ! licenseInfo.valid)
    {
        auto r = getLocalBounds().toFloat();

        g.setColour (backgroundBottom.withAlpha (0.82f));
        g.fillAll();

        g.setFont (labelFont (22.0f * scale, true));
        g.setColour (goldBright);
        drawTracked (g, licenseInfo.expired ? "SUBSCRIPTION EXPIRED" : "FREE TRIAL ENDED",
                     r.withHeight (r.getHeight() * 0.5f).toNearestInt(),
                     juce::Justification::centred, 2.0f);

        g.setFont (labelFont (13.0f * scale, false));
        g.setColour (text);
        drawTracked (g, licenseInfo.expired ? "RENEW YOUR SUBSCRIPTION TO KEEP USING ZINOX VOCALS"
                                             : "ENTER A LICENSE KEY TO KEEP USING ZINOX VOCALS",
                     r.withTop (r.getCentreY()).toNearestInt(),
                     juce::Justification::centred, 1.4f);
    }
}
