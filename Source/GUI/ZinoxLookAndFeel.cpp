#include "ZinoxLookAndFeel.h"

namespace zx
{

using namespace theme;

ZinoxLookAndFeel::ZinoxLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, backgroundBottom);
    setColour (juce::Label::textColourId,                 text);
    setColour (juce::Slider::textBoxTextColourId,         text);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId,        panelInner);
    setColour (juce::ComboBox::textColourId,              gold);
    setColour (juce::ComboBox::outlineColourId,           panelOutline);
    setColour (juce::ComboBox::arrowColourId,             gold);
    setColour (juce::PopupMenu::backgroundColourId,       panelInner);
    setColour (juce::PopupMenu::textColourId,             text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, gold);
    setColour (juce::PopupMenu::highlightedTextColourId,  juce::Colour (0xff14141C));
    setColour (juce::TextButton::buttonColourId,          panelInner);
    setColour (juce::TextButton::textColourOffId,         textDim);
    setColour (juce::TextButton::textColourOnId,          gold);
    setColour (juce::TooltipWindow::backgroundColourId,   panelInner);
    setColour (juce::TooltipWindow::textColourId,         text);
    setColour (juce::TooltipWindow::outlineColourId,      gold.withAlpha (0.5f));
    setColour (juce::AlertWindow::backgroundColourId,     backgroundTop);
    setColour (juce::AlertWindow::textColourId,           text);
    setColour (juce::AlertWindow::outlineColourId,        gold.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId,      panelInner);
    setColour (juce::TextEditor::textColourId,            text);
    setColour (juce::TextEditor::highlightColourId,       gold.withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId,         panelOutline);
    setColour (juce::TextEditor::focusedOutlineColourId,  gold);
    setColour (juce::CaretComponent::caretColourId,       gold);
}

// ---------------------------------------------------------------------------

void ZinoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = radius - 3.0f;
    const auto arcThickness = juce::jmax (3.0f, radius * 0.13f);

    const bool enabled = slider.isEnabled();
    const auto accent = enabled ? (slider.isMouseOverOrDragging() ? goldBright : gold)
                                : gold.withAlpha (0.3f);

    // --- tick marks around the outside --------------------------------------
    {
        const auto numTicks = 11;
        g.setColour (textFaint.withAlpha (0.55f));

        for (int i = 0; i < numTicks; ++i)
        {
            const auto t = (float) i / (float) (numTicks - 1);
            const auto a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const auto inner = radius + 1.0f;
            const auto outer = radius + (i == 0 || i == numTicks - 1 || i == numTicks / 2 ? 5.0f : 3.0f);

            g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                        centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer,
                        i == numTicks / 2 ? 1.6f : 1.0f);
        }
    }

    // --- background track ---------------------------------------------------
    juce::Path backTrack;
    backTrack.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (track);
    g.strokePath (backTrack, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    // --- value arc ----------------------------------------------------------
    const bool bipolar = (bool) slider.getProperties().getWithDefault (bipolarProperty, false);
    const auto centreAngle = rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle);
    const auto fromAngle = bipolar ? centreAngle : rotaryStartAngle;

    if (std::abs (angle - fromAngle) > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);

        if (enabled)
            drawGlow (g, valueArc, accent, arcThickness);

        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // --- knob body ----------------------------------------------------------
    const auto bodyRadius = arcRadius - arcThickness * 0.5f - 4.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (body.translated (0.0f, 2.0f).expanded (1.0f));

    juce::ColourGradient bodyGrad (knobFaceTop, body.getCentreX(), body.getY(),
                                   knobFaceBottom, body.getCentreX(), body.getBottom(), false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (body);

    // rim
    g.setColour (knobRim.withAlpha (enabled ? 0.9f : 0.4f));
    g.drawEllipse (body.reduced (0.5f), 1.2f);

    // specular highlight across the top of the cap
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.14f), body.getCentreX(), body.getY(),
                               juce::Colours::transparentWhite, body.getCentreX(), body.getCentreY(), false);
    g.setGradientFill (spec);
    g.fillEllipse (body.reduced (1.5f));

    // --- pointer ------------------------------------------------------------
    {
        juce::Path pointer;
        const auto pw = juce::jmax (2.2f, bodyRadius * 0.11f);
        pointer.addRoundedRectangle (-pw * 0.5f, -bodyRadius + 3.0f, pw, bodyRadius * 0.52f, pw * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

        if (enabled)
            drawGlow (g, pointer, accent, pw, 3);

        g.setColour (enabled ? goldBright : textFaint);
        g.fillPath (pointer);
    }

    // --- centre dot ---------------------------------------------------------
    g.setColour (accent.withAlpha (enabled ? 0.85f : 0.3f));
    g.fillEllipse (juce::Rectangle<float> (3.5f, 3.5f).withCentre (centre));
}

// ---------------------------------------------------------------------------

void ZinoxLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const bool vertical = slider.isVertical();
    const bool enabled = slider.isEnabled();
    const auto accent = enabled ? (slider.isMouseOverOrDragging() ? goldBright : gold)
                                : gold.withAlpha (0.35f);

    const auto steps = (int) slider.getProperties().getWithDefault (stepTicksProperty, 0);

    if (vertical)
    {
        const auto trackRect = juce::Rectangle<float> (5.0f, bounds.getHeight())
                                   .withCentre ({ bounds.getCentreX(), bounds.getCentreY() });

        g.setColour (track);
        g.fillRoundedRectangle (trackRect, 2.5f);
        g.setColour (panelOutline.withAlpha (0.6f));
        g.drawRoundedRectangle (trackRect, 2.5f, 1.0f);

        // Detent ticks for the -3 / -6 / -9 style switches.
        if (steps > 1)
        {
            g.setColour (textFaint);
            for (int i = 0; i < steps; ++i)
            {
                const auto t = (float) i / (float) (steps - 1);
                const auto ty = juce::jmap (t, bounds.getBottom() - 6.0f, bounds.getY() + 6.0f);
                g.drawLine (bounds.getCentreX() - 8.0f, ty, bounds.getCentreX() - 4.0f, ty, 1.2f);
                g.drawLine (bounds.getCentreX() + 4.0f, ty, bounds.getCentreX() + 8.0f, ty, 1.2f);
            }
        }

        // thumb
        const auto thumbH = 12.0f;
        const auto thumb = juce::Rectangle<float> (bounds.getWidth() * 0.8f, thumbH)
                               .withCentre ({ bounds.getCentreX(), juce::jlimit (bounds.getY() + thumbH * 0.5f,
                                                                                 bounds.getBottom() - thumbH * 0.5f,
                                                                                 sliderPos) });

        juce::Path thumbPath;
        thumbPath.addRoundedRectangle (thumb, 3.0f);

        if (enabled)
            drawGlow (g, thumbPath, accent, 2.0f, 3);

        juce::ColourGradient tg (accent.brighter (0.25f), thumb.getCentreX(), thumb.getY(),
                                 accent.darker (0.35f), thumb.getCentreX(), thumb.getBottom(), false);
        g.setGradientFill (tg);
        g.fillRoundedRectangle (thumb, 3.0f);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (thumb.getX() + 2.0f, thumb.getCentreY(), thumb.getRight() - 2.0f, thumb.getCentreY(), 1.0f);
    }
    else
    {
        const auto trackRect = juce::Rectangle<float> (bounds.getWidth(), 5.0f)
                                   .withCentre ({ bounds.getCentreX(), bounds.getCentreY() });

        g.setColour (track);
        g.fillRoundedRectangle (trackRect, 2.5f);
        g.setColour (panelOutline.withAlpha (0.6f));
        g.drawRoundedRectangle (trackRect, 2.5f, 1.0f);

        // filled portion, drawn from the centre when the control is bipolar
        const bool bipolar = (bool) slider.getProperties().getWithDefault (bipolarProperty, false);
        const auto from = bipolar ? bounds.getCentreX() : bounds.getX();
        const auto lo = juce::jmin (from, sliderPos);
        const auto hi = juce::jmax (from, sliderPos);

        if (hi - lo > 0.5f && enabled)
        {
            g.setColour (accent.withAlpha (0.9f));
            g.fillRoundedRectangle ({ lo, trackRect.getY(), hi - lo, trackRect.getHeight() }, 2.5f);
        }

        const auto thumbW = 12.0f;
        const auto thumb = juce::Rectangle<float> (thumbW, bounds.getHeight() * 0.8f)
                               .withCentre ({ juce::jlimit (bounds.getX() + thumbW * 0.5f,
                                                            bounds.getRight() - thumbW * 0.5f,
                                                            sliderPos),
                                              bounds.getCentreY() });

        juce::Path thumbPath;
        thumbPath.addRoundedRectangle (thumb, 3.0f);

        if (enabled)
            drawGlow (g, thumbPath, accent, 2.0f, 3);

        juce::ColourGradient tg (accent.brighter (0.25f), thumb.getCentreX(), thumb.getY(),
                                 accent.darker (0.35f), thumb.getCentreX(), thumb.getBottom(), false);
        g.setGradientFill (tg);
        g.fillRoundedRectangle (thumb, 3.0f);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (thumb.getCentreX(), thumb.getY() + 2.0f, thumb.getCentreX(), thumb.getBottom() - 2.0f, 1.0f);
    }

    juce::ignoreUnused (style);
}

juce::Slider::SliderLayout ZinoxLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    juce::Slider::SliderLayout layout;
    layout.sliderBounds = slider.getLocalBounds();
    layout.textBoxBounds = {};
    return layout;
}

// ---------------------------------------------------------------------------

void ZinoxLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    const auto corner = 4.0f;
    const bool hover = box.isMouseOver() || isButtonDown;

    juce::ColourGradient grad (panelInner.brighter (hover ? 0.16f : 0.06f), 0.0f, 0.0f,
                               panelInner.darker (0.25f), 0.0f, (float) height, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (hover ? gold.withAlpha (0.85f) : panelOutline);
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    // arrow
    const auto arrowArea = juce::Rectangle<float> ((float) width - 16.0f, 0.0f, 12.0f, (float) height);
    juce::Path arrow;
    arrow.startNewSubPath (arrowArea.getCentreX() - 3.5f, arrowArea.getCentreY() - 1.8f);
    arrow.lineTo (arrowArea.getCentreX(), arrowArea.getCentreY() + 2.4f);
    arrow.lineTo (arrowArea.getCentreX() + 3.5f, arrowArea.getCentreY() - 1.8f);

    g.setColour (hover ? goldBright : gold);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void ZinoxLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 0, box.getWidth() - 22, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font ZinoxLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (11.5f);
}

juce::Font ZinoxLookAndFeel::getPopupMenuFont()
{
    return labelFont (13.0f, false);
}

juce::Font ZinoxLookAndFeel::getLabelFont (juce::Label& label)
{
    return label.getFont();
}

void ZinoxLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (panelInner);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (gold.withAlpha (0.4f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

void ZinoxLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& itemText,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused (hasSubMenu, shortcutKeyText, icon, textColour);

    if (isSeparator)
    {
        g.setColour (panelOutline.withAlpha (0.7f));
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    auto r = area.reduced (3, 1);

    if (isHighlighted && isActive)
    {
        g.setColour (gold.withAlpha (0.9f));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff14141C));
    }
    else
    {
        g.setColour (isActive ? text : textFaint);
    }

    if (isTicked)
    {
        auto tickArea = r.removeFromLeft (18).reduced (5);

        if (! (isHighlighted && isActive))
            g.setColour (gold);

        g.fillEllipse (tickArea.toFloat().withSizeKeepingCentre (6.0f, 6.0f));

        if (isHighlighted && isActive)
            g.setColour (juce::Colour (0xff14141C));
        else
            g.setColour (text);
    }
    else
    {
        r.removeFromLeft (18);
    }

    g.setFont (labelFont (13.0f, false));
    g.drawText (itemText, r, juce::Justification::centredLeft, true);
}

// ---------------------------------------------------------------------------

void ZinoxLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto corner = 4.0f;
    const bool on = button.getToggleState();

    juce::Colour fill = on ? gold.withAlpha (0.9f) : panelInner;

    if (shouldDrawButtonAsDown)
        fill = fill.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.15f);

    juce::ColourGradient grad (fill.brighter (0.1f), 0.0f, bounds.getY(),
                               fill.darker (0.22f), 0.0f, bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, corner);

    if (on)
    {
        juce::Path p;
        p.addRoundedRectangle (bounds, corner);
        drawGlow (g, p, gold, 1.5f, 3);
    }

    g.setColour (on ? goldBright : (shouldDrawButtonAsHighlighted ? gold.withAlpha (0.7f) : panelOutline));
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void ZinoxLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted, bool)
{
    const bool on = button.getToggleState();

    g.setFont (labelFont ((float) juce::jmin (13, button.getHeight() - 6)));
    g.setColour (on ? juce::Colour (0xff14141C)
                    : (shouldDrawButtonAsHighlighted ? goldBright : textDim));

    drawTracked (g, button.getButtonText(), button.getLocalBounds(),
                 juce::Justification::centred, 1.2f);
}

void ZinoxLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    drawButtonBackground (g, button, {}, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    const bool on = button.getToggleState();
    g.setFont (labelFont ((float) juce::jmin (12, button.getHeight() - 6)));
    g.setColour (on ? juce::Colour (0xff14141C) : textDim);
    drawTracked (g, button.getButtonText(), button.getLocalBounds(),
                 juce::Justification::centred, 1.2f);
}

} // namespace zx
