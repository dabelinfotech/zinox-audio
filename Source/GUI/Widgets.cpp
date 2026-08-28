#include "Widgets.h"

namespace zx
{

using namespace theme;

// ===========================================================================
//  ZinoxKnob
// ===========================================================================

ZinoxKnob::ZinoxKnob (const juce::String& captionIn, bool bipolar, float captionHeight)
    : caption (captionIn), captionH (captionHeight)
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setVelocityBasedMode (false);
    slider.setDoubleClickReturnValue (true, 0.0);
    slider.getProperties().set (ZinoxLookAndFeel::bipolarProperty, bipolar);
    addAndMakeVisible (slider);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (labelFont (11.0f, false));
    valueLabel.setColour (juce::Label::textColourId, textDim);
    valueLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (valueLabel);

    slider.onValueChange = [this] { refreshValueText(); };
}

void ZinoxKnob::attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);

    // Double-click resets to the parameter's own default rather than zero,
    // which matters for Low Cut / High Cut where zero is out of range.
    if (auto* param = apvts.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));

    refreshValueText();
}

void ZinoxKnob::refreshValueText()
{
    if (attachment == nullptr)
        return;

    valueLabel.setText (slider.getTextFromValue (slider.getValue()),
                        juce::dontSendNotification);
}

void ZinoxKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop ((int) captionH + 3);
    valueLabel.setBounds (r.removeFromBottom (14));

    // Keep the dial square and centred — cells in this layout are taller than
    // they are wide, and a rotary stretched into one looks unanchored.
    const auto size = juce::jmin (r.getWidth(), r.getHeight()) - 4;
    slider.setBounds (r.withSizeKeepingCentre (size, size));
}

void ZinoxKnob::paint (juce::Graphics& g)
{
    g.setFont (labelFont (captionH));
    g.setColour (text);
    drawTracked (g, caption, getLocalBounds().removeFromTop ((int) captionH + 3),
                 juce::Justification::centred, 1.8f);
}

// ===========================================================================
//  ZinoxMiniSlider
// ===========================================================================

ZinoxMiniSlider::ZinoxMiniSlider (const juce::String& captionIn, bool bipolar)
    : caption (captionIn)
{
    slider.getProperties().set (ZinoxLookAndFeel::bipolarProperty, bipolar);
    slider.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (slider);
}

void ZinoxMiniSlider::attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);

    if (auto* param = apvts.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
}

void ZinoxMiniSlider::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (12);
    slider.setBounds (r);
}

void ZinoxMiniSlider::paint (juce::Graphics& g)
{
    g.setFont (labelFont (9.5f));
    g.setColour (textFaint);
    drawTracked (g, caption, getLocalBounds().removeFromBottom (12),
                 juce::Justification::centred, 1.4f);
}

// ===========================================================================
//  RangeSwitch
// ===========================================================================

RangeSwitch::RangeSwitch (const juce::StringArray& labelsIn)
    : labels (labelsIn)
{
    slider.setRange (0.0, (double) juce::jmax (1, labels.size() - 1), 1.0);
    slider.getProperties().set (ZinoxLookAndFeel::stepTicksProperty, labels.size());
    addAndMakeVisible (slider);
}

void RangeSwitch::attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);
}

void RangeSwitch::resized()
{
    // Leave room on the right for the numeric legend.
    slider.setBounds (getLocalBounds().removeFromLeft (getWidth() - 20));
}

void RangeSwitch::paint (juce::Graphics& g)
{
    const auto legend = getLocalBounds().removeFromRight (20).toFloat();
    const auto trackArea = getLocalBounds().removeFromLeft (getWidth() - 20).toFloat();

    g.setFont (labelFont (9.0f, false));

    const auto selected = (int) std::round (slider.getValue());

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto t = (float) i / (float) juce::jmax (1, labels.size() - 1);
        const auto y = juce::jmap (t, trackArea.getBottom() - 6.0f, trackArea.getY() + 6.0f);

        g.setColour (i == selected ? gold : textFaint);
        g.drawText (labels[i],
                    juce::Rectangle<float> (legend.getX(), y - 6.0f, legend.getWidth(), 12.0f),
                    juce::Justification::centredLeft, false);
    }
}

// ===========================================================================
//  LeverSwitch
// ===========================================================================

LeverSwitch::LeverSwitch (const juce::String& top, const juce::String& bottom)
    : topText (top), bottomText (bottom)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

LeverSwitch::~LeverSwitch()
{
    stateValue.removeListener (this);
}

void LeverSwitch::attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    state = &apvts;
    param = paramID;

    stateValue.referTo (apvts.getParameterAsValue (paramID));
    stateValue.addListener (this);
    valueChanged (stateValue);
}

void LeverSwitch::valueChanged (juce::Value&)
{
    // Index 0 is the top label, so a value of 0 shows the lever up.
    isTop = ((int) stateValue.getValue()) == 0;
    repaint();
}

void LeverSwitch::mouseDown (const juce::MouseEvent&)
{
    if (state == nullptr)
        return;

    if (auto* p = state->getParameter (param))
    {
        const auto next = isTop ? 1.0f : 0.0f;
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (next));
        p->endChangeGesture();
    }
}

void LeverSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    const auto labelH = 11;
    auto topArea = r.removeFromTop (labelH);
    auto bottomArea = r.removeFromBottom (labelH);
    auto body = r.reduced (0, 2).toFloat();

    // Keep the lever body narrow so it reads as a switch, not a fader.
    body = juce::Rectangle<float> (16.0f, body.getHeight()).withCentre (body.getCentre());

    g.setFont (labelFont (8.5f));
    g.setColour (isTop ? gold : textFaint);
    drawTracked (g, topText, topArea, juce::Justification::centred, 1.0f);
    g.setColour (isTop ? textFaint : gold);
    drawTracked (g, bottomText, bottomArea, juce::Justification::centred, 1.0f);

    // slot
    g.setColour (track);
    g.fillRoundedRectangle (body, 5.0f);
    g.setColour (panelOutline);
    g.drawRoundedRectangle (body.reduced (0.5f), 5.0f, 1.0f);

    // lever cap
    const auto capH = body.getHeight() * 0.46f;
    auto cap = juce::Rectangle<float> (body.getWidth() - 3.0f, capH)
                   .withCentre ({ body.getCentreX(),
                                  isTop ? body.getY() + capH * 0.5f + 1.5f
                                        : body.getBottom() - capH * 0.5f - 1.5f });

    juce::Path capPath;
    capPath.addRoundedRectangle (cap, 4.0f);
    drawGlow (g, capPath, gold, 1.5f, 3);

    juce::ColourGradient grad (goldBright, cap.getCentreX(), cap.getY(),
                               goldDeep, cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (cap, 4.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (cap.getX() + 3.0f, cap.getCentreY(), cap.getRight() - 3.0f, cap.getCentreY(), 1.0f);
}

// ===========================================================================
//  LevelMeter
// ===========================================================================

LevelMeter::LevelMeter()
{
    startTimerHz (30);
}

void LevelMeter::setLevels (float leftDb, float rightDb)
{
    targetL = leftDb;
    targetR = rightDb;
}

float LevelMeter::dbToProportion (float db)
{
    // -60 dB at the bottom, 0 dB at the top, with the useful range expanded.
    const auto clamped = juce::jlimit (-60.0f, 6.0f, db);
    return juce::jmap (clamped, -60.0f, 6.0f, 0.0f, 1.0f);
}

void LevelMeter::timerCallback()
{
    auto decay = [] (float& display, float target, float& peak, int& hold)
    {
        // Instant rise, smooth fall — the standard way to make a meter readable.
        display = target > display ? target : display * 0.72f + target * 0.28f;

        if (display >= peak)
        {
            peak = display;
            hold = 30;              // ~1 second at 30 Hz
        }
        else if (--hold <= 0)
        {
            peak -= 1.2f;
        }
    };

    decay (displayL, targetL, peakL, peakHoldL);
    decay (displayR, targetR, peakR, peakHoldR);

    repaint();
}

void LevelMeter::drawBar (juce::Graphics& g, juce::Rectangle<float> r, float levelDb, float peakDb)
{
    g.setColour (track);
    g.fillRoundedRectangle (r, 2.0f);

    const auto prop = dbToProportion (levelDb);

    if (prop > 0.001f)
    {
        const auto h = r.getHeight() * prop;
        auto fill = r.withTop (r.getBottom() - h);

        // Green up to -12, yellow to -3, red above.
        juce::ColourGradient grad (meterGreen, r.getCentreX(), r.getBottom(),
                                   meterRed, r.getCentreX(), r.getY(), false);
        grad.addColour (dbToProportion (-12.0f), meterGreen);
        grad.addColour (dbToProportion (-6.0f), meterYellow);
        grad.addColour (dbToProportion (-1.5f), meterRed);

        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 2.0f);
    }

    // peak-hold line
    const auto peakProp = dbToProportion (peakDb);
    if (peakProp > 0.001f)
    {
        const auto y = r.getBottom() - r.getHeight() * peakProp;
        g.setColour (peakDb > -0.5f ? meterRed : goldBright);
        g.fillRect (juce::Rectangle<float> (r.getX(), y - 1.0f, r.getWidth(), 2.0f));
    }

    g.setColour (panelOutline.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    const auto gap = 3.0f;
    const auto barW = (r.getWidth() - gap) * 0.5f;

    drawBar (g, r.removeFromLeft (barW), displayL, peakL);
    r.removeFromLeft (gap);
    drawBar (g, r, displayR, peakR);
}

// ===========================================================================
//  GainReductionMeter
// ===========================================================================

GainReductionMeter::GainReductionMeter (float maxDb, bool horizontal)
    : maxRange (maxDb), isHorizontal (horizontal)
{
    startTimerHz (30);
}

void GainReductionMeter::setGainReduction (float grDb)
{
    target = juce::jlimit (0.0f, maxRange, grDb);
}

void GainReductionMeter::timerCallback()
{
    display = target > display ? target : display * 0.75f + target * 0.25f;

    if (display < 0.01f)
        display = 0.0f;

    repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setColour (track);
    g.fillRoundedRectangle (r, 2.0f);

    const auto prop = juce::jlimit (0.0f, 1.0f, display / maxRange);

    if (prop > 0.002f)
    {
        // Gain reduction reads downward from the top — the direction the gain
        // is actually moving.
        auto fill = isHorizontal ? r.withWidth (r.getWidth() * prop)
                                 : r.withHeight (r.getHeight() * prop);

        juce::ColourGradient grad (gold, r.getX(), r.getY(),
                                   meterRed, isHorizontal ? r.getRight() : r.getX(),
                                   isHorizontal ? r.getY() : r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 2.0f);
    }

    g.setColour (panelOutline.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);
}

// ===========================================================================
//  LogoComponent
// ===========================================================================

LogoComponent::LogoComponent (bool withWordmark)
    : showWordmark (withWordmark)
{
}

void LogoComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto markArea = r.removeFromLeft (r.getHeight());

    // rounded square badge
    juce::ColourGradient badge (juce::Colour (0xff32323F), markArea.getCentreX(), markArea.getY(),
                                juce::Colour (0xff1A1A24), markArea.getCentreX(), markArea.getBottom(), false);
    g.setGradientFill (badge);
    g.fillRoundedRectangle (markArea, markArea.getWidth() * 0.24f);

    g.setColour (gold.withAlpha (0.55f));
    g.drawRoundedRectangle (markArea.reduced (0.5f), markArea.getWidth() * 0.24f, 1.0f);

    // The mark: a "Z" cut as a solid chevron, which stays legible when small.
    auto m = markArea.reduced (markArea.getWidth() * 0.26f);
    juce::Path z;
    z.startNewSubPath (m.getX(), m.getY());
    z.lineTo (m.getRight(), m.getY());
    z.lineTo (m.getRight(), m.getY() + m.getHeight() * 0.26f);
    z.lineTo (m.getX() + m.getWidth() * 0.34f, m.getBottom() - m.getHeight() * 0.26f);
    z.lineTo (m.getRight(), m.getBottom() - m.getHeight() * 0.26f);
    z.lineTo (m.getRight(), m.getBottom());
    z.lineTo (m.getX(), m.getBottom());
    z.lineTo (m.getX(), m.getBottom() - m.getHeight() * 0.26f);
    z.lineTo (m.getRight() - m.getWidth() * 0.34f, m.getY() + m.getHeight() * 0.26f);
    z.lineTo (m.getX(), m.getY() + m.getHeight() * 0.26f);
    z.closeSubPath();

    drawGlow (g, z, gold, 1.0f, 3);

    juce::ColourGradient zg (goldBright, m.getCentreX(), m.getY(),
                             goldDeep, m.getCentreX(), m.getBottom(), false);
    g.setGradientFill (zg);
    g.fillPath (z);

    if (showWordmark && r.getWidth() > 10.0f)
    {
        r.removeFromLeft (8.0f);

        auto upper = r.removeFromTop (r.getHeight() * 0.58f);
        g.setFont (labelFont (upper.getHeight() * 0.86f));
        g.setColour (text);
        drawTracked (g, "ZINOX", upper.toNearestInt(), juce::Justification::left, 2.4f);

        g.setFont (labelFont (r.getHeight() * 0.78f, false));
        g.setColour (gold);
        drawTracked (g, "AUDIO", r.toNearestInt(), juce::Justification::left, 3.4f);
    }
}

// ===========================================================================
//  FileDropZone
// ===========================================================================

FileDropZone::FileDropZone() = default;

bool FileDropZone::isSupportedAudioFile (const juce::File& f)
{
    return f.hasFileExtension ("wav;aiff;aif;mp3;flac;ogg;m4a;wma");
}

bool FileDropZone::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isSupportedAudioFile (juce::File (f)))
            return true;

    return false;
}

void FileDropZone::fileDragEnter (const juce::StringArray&, int, int)
{
    dragHighlight = true;
    repaint();
}

void FileDropZone::fileDragExit (const juce::StringArray&)
{
    dragHighlight = false;
    repaint();
}

void FileDropZone::filesDropped (const juce::StringArray& files, int, int)
{
    dragHighlight = false;

    for (const auto& f : files)
    {
        juce::File file (f);

        if (isSupportedAudioFile (file))
        {
            setLoadedFile (file);

            if (onFileDropped != nullptr)
                onFileDropped (file);

            break;
        }
    }

    repaint();
}

void FileDropZone::setLoadedFile (const juce::File& file)
{
    loadedFile = file;
    repaint();
}

void FileDropZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto corner = 6.0f;

    g.setColour (dragHighlight ? gold.withAlpha (0.18f) : panelInner.withAlpha (0.6f));
    g.fillRoundedRectangle (r, corner);

    g.setColour (dragHighlight ? goldBright : panelOutline);
    juce::Path dashed;
    dashed.addRoundedRectangle (r.reduced (1.0f), corner);
    const float dashLengths[] = { 5.0f, 4.0f };
    juce::PathStrokeType stroke (1.4f);
    juce::Path dashedStroke;
    stroke.createDashedStroke (dashedStroke, dashed, dashLengths, 2);
    g.fillPath (dashedStroke);

    g.setFont (labelFont (12.0f, ! loadedFile.existsAsFile()));

    if (loadedFile.existsAsFile())
    {
        g.setColour (text);
        drawTracked (g, loadedFile.getFileName(), r.reduced (10.0f, 0.0f).toNearestInt(),
                     juce::Justification::centred, 1.0f);
    }
    else
    {
        g.setColour (textFaint);
        drawTracked (g, "DROP AN AUDIO FILE HERE, OR CLICK IMPORT",
                     r.toNearestInt(), juce::Justification::centred, 1.2f);
    }
}

} // namespace zx
