#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "ZinoxLookAndFeel.h"

namespace zx
{

// ---------------------------------------------------------------------------
/** A labelled rotary control: caption above, knob, value readout below. */
class ZinoxKnob : public juce::Component
{
public:
    ZinoxKnob (const juce::String& caption, bool bipolar = false, float captionHeight = 13.0f);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider& getSlider() noexcept { return slider; }
    void setCaption (const juce::String& c) { caption = c; repaint(); }

    /** Attaches to the tree.  Call once, after construction. */
    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

private:
    void refreshValueText();

    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Label  valueLabel;
    juce::String caption;
    float captionH;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZinoxKnob)
};

// ---------------------------------------------------------------------------
/** Small horizontal slider with a caption underneath (LOW FREQ, TONE). */
class ZinoxMiniSlider : public juce::Component
{
public:
    ZinoxMiniSlider (const juce::String& caption, bool bipolar = false);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider& getSlider() noexcept { return slider; }
    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

private:
    juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZinoxMiniSlider)
};

// ---------------------------------------------------------------------------
/** Vertical detented switch used for the -3 / -6 / -9 range selectors. */
class RangeSwitch : public juce::Component
{
public:
    explicit RangeSwitch (const juce::StringArray& labels);

    void resized() override;
    void paint (juce::Graphics&) override;

    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

private:
    juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::StringArray labels;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangeSwitch)
};

// ---------------------------------------------------------------------------
/** Two-position vertical toggle, drawn like a hardware lever (PEAK / SHELF). */
class LeverSwitch : public juce::Component,
                    private juce::Value::Listener
{
public:
    LeverSwitch (const juce::String& topLabel, const juce::String& bottomLabel);
    ~LeverSwitch() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

private:
    void valueChanged (juce::Value&) override;

    juce::String topText, bottomText;
    juce::AudioProcessorValueTreeState* state = nullptr;
    juce::String param;
    juce::Value stateValue;
    bool isTop = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LeverSwitch)
};

// ---------------------------------------------------------------------------
/** Vertical stereo output meter with peak-hold. */
class LevelMeter : public juce::Component,
                   private juce::Timer
{
public:
    LevelMeter();

    void paint (juce::Graphics&) override;

    /** dBFS values. */
    void setLevels (float leftDb, float rightDb);

private:
    void timerCallback() override;
    void drawBar (juce::Graphics&, juce::Rectangle<float>, float levelDb, float peakDb);

    static float dbToProportion (float db);

    float targetL = -100.0f, targetR = -100.0f;
    float displayL = -100.0f, displayR = -100.0f;
    float peakL = -100.0f, peakR = -100.0f;
    int   peakHoldL = 0, peakHoldR = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

// ---------------------------------------------------------------------------
/** Downward gain-reduction bar, used by the limiter and the de-esser. */
class GainReductionMeter : public juce::Component,
                           private juce::Timer
{
public:
    explicit GainReductionMeter (float maxDb = 12.0f, bool horizontal = false);

    void paint (juce::Graphics&) override;
    void setGainReduction (float grDb);

private:
    void timerCallback() override;

    float target = 0.0f, display = 0.0f, maxRange;
    bool  isHorizontal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};

// ---------------------------------------------------------------------------
/** The Zinox mark: a bright gold chevron in a rounded square. */
class LogoComponent : public juce::Component
{
public:
    explicit LogoComponent (bool withWordmark = false);
    void paint (juce::Graphics&) override;

private:
    bool showWordmark;
};

// ---------------------------------------------------------------------------
/** Drag-and-drop target for a single audio file (standalone import/export). */
class FileDropZone : public juce::Component,
                     public juce::FileDragAndDropTarget
{
public:
    FileDropZone();

    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /** Updates the displayed filename without going through drag-and-drop -
        call this after a file is chosen via the Import button too. */
    void setLoadedFile (const juce::File& file);

    std::function<void (const juce::File&)> onFileDropped;

private:
    static bool isSupportedAudioFile (const juce::File& f);

    juce::File loadedFile;
    bool dragHighlight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileDropZone)
};

} // namespace zx
