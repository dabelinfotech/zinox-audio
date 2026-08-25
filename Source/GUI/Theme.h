#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zx::theme
{

// ---------------------------------------------------------------------------
//  Palette.  Deliberately brighter and higher-contrast than a typical black
//  strip: the panels are lifted off the background, and the accent is a warm
//  saturated gold that stays legible on every display.
// ---------------------------------------------------------------------------
inline const juce::Colour backgroundTop    { 0xff2A2A36 };
inline const juce::Colour backgroundBottom { 0xff16161E };

inline const juce::Colour panelTop         { 0xff383846 };
inline const juce::Colour panelBottom      { 0xff262632 };
inline const juce::Colour panelOutline     { 0xff4A4A5C };
inline const juce::Colour panelInner       { 0xff1E1E28 };

inline const juce::Colour gold             { 0xffFFC72C };
inline const juce::Colour goldBright       { 0xffFFE27A };
inline const juce::Colour goldDeep         { 0xffE0A310 };

inline const juce::Colour text             { 0xffF4F4F8 };
inline const juce::Colour textDim          { 0xffA6A6B8 };
inline const juce::Colour textFaint        { 0xff70707F };

inline const juce::Colour knobFaceTop      { 0xff3E3E4C };
inline const juce::Colour knobFaceBottom   { 0xff1C1C26 };
inline const juce::Colour knobRim          { 0xff56566A };
inline const juce::Colour track            { 0xff14141C };

inline const juce::Colour meterGreen       { 0xff45E08A };
inline const juce::Colour meterYellow      { 0xffFFD84D };
inline const juce::Colour meterRed         { 0xffFF5C5C };

// ---------------------------------------------------------------------------
//  Type
// ---------------------------------------------------------------------------
inline juce::Font labelFont (float height, bool bold = true)
{
    auto opts = juce::FontOptions().withHeight (height);

    if (bold)
        opts = opts.withStyle ("Bold");

    return juce::Font (opts);
}

// Section headings are letter-spaced small caps, which is what gives the panel
// its "hardware" feel without needing a custom typeface.
inline void drawTracked (juce::Graphics& g, const juce::String& textToDraw,
                         juce::Rectangle<int> area, juce::Justification just,
                         float tracking = 1.6f)
{
    const auto upper = textToDraw.toUpperCase();

    if (upper.isEmpty())
        return;

    const auto font = g.getCurrentFont();
    const auto widthOf = [&font] (const juce::String& s)
    {
        return juce::GlyphArrangement::getStringWidth (font, s);
    };

    float total = 0.0f;

    for (int i = 0; i < upper.length(); ++i)
        total += widthOf (upper.substring (i, i + 1)) + tracking;

    total -= tracking;

    float x = (float) area.getX();

    if (just.testFlags (juce::Justification::horizontallyCentred))
        x = (float) area.getCentreX() - total * 0.5f;
    else if (just.testFlags (juce::Justification::right))
        x = (float) area.getRight() - total;

    const auto baseline = (float) area.getCentreY() + font.getAscent() * 0.5f - font.getDescent() * 0.4f;

    for (int i = 0; i < upper.length(); ++i)
    {
        const auto ch = upper.substring (i, i + 1);
        g.drawSingleLineText (ch, juce::roundToInt (x), juce::roundToInt (baseline));
        x += widthOf (ch) + tracking;
    }
}

/** Recessed inner panel with a soft top-edge highlight. */
inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner = 10.0f,
                       bool raised = true)
{
    juce::ColourGradient grad (raised ? panelTop : panelInner, r.getCentreX(), r.getY(),
                               raised ? panelBottom : panelInner.darker (0.3f), r.getCentreX(), r.getBottom(),
                               false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, corner);

    g.setColour (panelOutline.withAlpha (0.65f));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);

    // Top highlight — a single bright line reads as a bevel.
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawLine (r.getX() + corner, r.getY() + 1.0f, r.getRight() - corner, r.getY() + 1.0f, 1.0f);
}

/** Soft outer glow, used behind the accent arcs and the logo. */
inline void drawGlow (juce::Graphics& g, juce::Path path, juce::Colour colour,
                      float thickness, int layers = 4)
{
    for (int i = layers; i >= 1; --i)
    {
        const auto t = thickness + (float) i * 1.6f;
        g.setColour (colour.withAlpha (0.055f * (float) (layers - i + 1) / (float) layers));
        g.strokePath (path, juce::PathStrokeType (t, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
}

} // namespace zx::theme
