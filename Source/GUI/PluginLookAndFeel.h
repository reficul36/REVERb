#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace argentum
{

// Palette lifted from the web build's Tailwind classes / index.css.
namespace Colours
{
    static const juce::Colour bg          { 0xff07090c };
    static const juce::Colour panelBg     { 0xff0b0e13 };
    static const juce::Colour groupBg     { 0xff0f1319 };
    static const juce::Colour groupBgLo   { 0xff0a0d12 };
    static const juce::Colour headerTop   { 0xff161b22 };
    static const juce::Colour border      { 0x1affffff };
    static const juce::Colour text        { 0xffd4d4d8 };
    static const juce::Colour textDim     { 0xff71717a };
    static const juce::Colour textDimmer  { 0xff52525b };
    static const juce::Colour cyan        { 0xff7fd4ff };
    static const juce::Colour cyanBright  { 0xff8ee3ff };
    static const juce::Colour green       { 0xff9ad9b0 };
    static const juce::Colour purple      { 0xffc8b4ff };
    static const juce::Colour amber       { 0xffffcf8e };
    static const juce::Colour rose        { 0xffff7d8c };
    static const juce::Colour knobFace    { 0xff20252e };
    static const juce::Colour knobFaceHi  { 0xff3a4250 };
    static const juce::Colour knobFaceLo  { 0xff12151b };
    static const juce::Colour knobTrack   { 0xff252b36 };
}

class PluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PluginLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, Colours::bg);
        setColour (juce::Slider::textBoxTextColourId, Colours::text);
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha (0.4f));
        setColour (juce::ComboBox::textColourId, Colours::text);
        setColour (juce::ComboBox::outlineColourId, Colours::border);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d1117));
        setColour (juce::PopupMenu::textColourId, Colours::text);
        setColour (juce::TextButton::buttonColourId, juce::Colours::white.withAlpha (0.03f));
        setColour (juce::TextButton::textColourOffId, Colours::textDim);
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (juce::FontOptions (11.0f)).withTypefaceStyle ("Regular");
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        const auto cx = bounds.getCentreX();
        const auto cy = bounds.getCentreY();
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // background track
        juce::Path track;
        track.addCentredArc (cx, cy, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Colours::knobTrack);
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc
        juce::Path valueArc;
        valueArc.addCentredArc (cx, cy, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // knob face
        const float faceR = radius - 5.0f;
        juce::ColourGradient grad (Colours::knobFaceHi, cx, cy - faceR * 0.35f,
                                    Colours::knobFaceLo, cx, cy + faceR, false);
        grad.addColour (0.6, Colours::knobFace);
        g.setGradientFill (grad);
        g.fillEllipse (cx - faceR, cy - faceR, faceR * 2.0f, faceR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawEllipse (cx - faceR, cy - faceR, faceR * 2.0f, faceR * 2.0f, 0.75f);

        // pointer
        juce::Path pointer;
        const float innerR = faceR * 0.42f, outerR = faceR - 4.0f;
        juce::Point<float> p0 (cx + innerR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                cy + innerR * std::sin (angle - juce::MathConstants<float>::halfPi));
        juce::Point<float> p1 (cx + outerR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                cy + outerR * std::sin (angle - juce::MathConstants<float>::halfPi));
        g.setColour (accent);
        g.drawLine ({ p0, p1 }, 2.0f);
        g.fillEllipse (p1.x - 1.6f, p1.y - 1.6f, 3.2f, 3.2f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                                bool isOver, bool isDown) override
    {
        auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (on ? backgroundColour.withAlpha (0.85f) : juce::Colours::white.withAlpha (isOver ? 0.05f : 0.03f));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (on ? backgroundColour : Colours::border);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
        juce::ignoreUnused (isDown);
    }
};

} // namespace argentum
