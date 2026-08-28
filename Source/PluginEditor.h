#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "GUI/ZinoxLookAndFeel.h"
#include "GUI/Widgets.h"

class ZinoxVocalsEditor : public juce::AudioProcessorEditor,
                          private juce::Timer,
                          private juce::ChangeListener
{
public:
    explicit ZinoxVocalsEditor (ZinoxVocalsProcessor&);
    ~ZinoxVocalsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void buildHeader();
    void buildLeftColumn();
    void buildCentre();
    void buildRightColumn();

    void refreshPresetList();
    void showSaveDialog();
    void showLicenseDialog();
    void updateLicenseBadge();

    void paintBackground (juce::Graphics&);
    void paintFooter (juce::Graphics&);

    ZinoxVocalsProcessor& processor;
    zx::ZinoxLookAndFeel lnf;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // --- header -------------------------------------------------------------
    zx::LogoComponent logo { true };
    juce::ComboBox presetBox;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton bypassButton { "BYPASS" };
    std::unique_ptr<ButtonAttach> bypassAttach;
    std::unique_ptr<juce::AlertWindow> saveWindow;

    juce::TextButton licenseBadge { "UNLICENSED" };
    std::unique_ptr<juce::AlertWindow> licenseWindow;

    // --- left column --------------------------------------------------------
    zx::ZinoxKnob inputKnob   { "INPUT",    true };
    zx::ZinoxKnob denoiseKnob { "DENOISE" };
    zx::ZinoxKnob lowCutKnob  { "LOW CUT" };
    zx::ZinoxKnob highCutKnob { "HIGH CUT" };

    // --- tone row -----------------------------------------------------------
    zx::ZinoxKnob       lowKnob  { "LOW", true };
    zx::LeverSwitch     lowShapeSwitch { "PEAK", "SHELF" };
    zx::ZinoxMiniSlider lowFreqSlider  { "LOW FREQ" };

    zx::ZinoxKnob       midKnob  { "MID", true };
    zx::ZinoxMiniSlider midToneSlider  { "TONE" };

    zx::ZinoxKnob highKnob { "HIGH", true };
    juce::ComboBox highModeBox;
    std::unique_ptr<ComboAttach> highModeAttach;

    zx::ZinoxKnob          deEssKnob { "DE-ESS" };
    zx::GainReductionMeter deEssMeter { 18.0f };
    zx::ZinoxMiniSlider    deEssFreqSlider { "FREQ" };

    // --- dynamics row -------------------------------------------------------
    zx::ZinoxKnob   controlKnob { "CONTROL" };
    zx::RangeSwitch controlRange { zx::Choices::range };
    juce::ComboBox  controlModeBox;
    std::unique_ptr<ComboAttach> controlModeAttach;

    zx::ZinoxKnob   pushKnob { "PUSH" };
    zx::RangeSwitch pushRange { zx::Choices::range };
    juce::ComboBox  pushModeBox;
    std::unique_ptr<ComboAttach> pushModeAttach;

    zx::ZinoxKnob  saturateKnob { "SATURATE" };
    juce::ComboBox satModeBox;
    std::unique_ptr<ComboAttach> satModeAttach;

    zx::ZinoxKnob       outputKnob { "OUTPUT", true };
    zx::ZinoxMiniSlider mixSlider  { "MIX" };

    // --- right column -------------------------------------------------------
    zx::LevelMeter         outMeter;
    zx::GainReductionMeter limitMeter { 12.0f };
    juce::Slider           limitCeiling { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    std::unique_ptr<SliderAttach> limitCeilingAttach;
    juce::TextButton       limitOnButton { "LIMIT" };
    std::unique_ptr<ButtonAttach> limitOnAttach;

    juce::ComboBox oversampleBox;
    std::unique_ptr<ComboAttach> oversampleAttach;

    juce::TooltipWindow tooltips { this, 500 };

    // Layout rectangles cached by resized() and reused by paint().
    juce::Rectangle<int> headerArea, leftPanelArea, centrePanelArea, rightPanelArea, footerArea;
    juce::Rectangle<int> toneRowArea, dynRowArea, titleArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZinoxVocalsEditor)
};
