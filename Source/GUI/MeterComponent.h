#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginLookAndFeel.h"

namespace argentum
{

class MeterComponent : public juce::Component, private juce::Timer
{
public:
    MeterComponent (std::atomic<float>& inLevel, std::atomic<float>& wetLevel, std::atomic<float>& duckGain)
        : in (inLevel), wet (wetLevel), duck (duckGain)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float w = b.getWidth() / 3.0f;
        drawBar (g, b.removeFromLeft (w).reduced (3.0f), scale (in.load()),
                 juce::Colour (0xff94a3b8), "IN");
        drawBar (g, b.removeFromLeft (w).reduced (3.0f), scale (wet.load()),
                 Colours::cyanBright, "WET");
        drawBar (g, b.reduced (3.0f), juce::jlimit (0.0f, 1.0f, (1.0f - duck.load())),
                 Colours::amber, "GR");
    }

private:
    static float scale (float v) { return juce::jlimit (0.0f, 1.0f, std::pow (juce::jmin (1.0f, v * 3.2f), 0.55f)); }

    void drawBar (juce::Graphics& g, juce::Rectangle<float> cell, float level01, juce::Colour c, const char* label)
    {
        auto barArea = cell.removeFromTop (cell.getHeight() - 12.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (barArea, 2.0f);
        g.setColour (Colours::border);
        g.drawRoundedRectangle (barArea, 2.0f, 1.0f);

        auto filled = barArea.removeFromBottom (barArea.getHeight() * level01);
        g.setColour (c);
        g.fillRoundedRectangle (filled, 2.0f);

        g.setColour (Colours::textDimmer);
        g.setFont (juce::Font (juce::FontOptions (8.0f)));
        g.drawText (label, cell, juce::Justification::centred);
    }

    void timerCallback() override { repaint(); }

    std::atomic<float>& in;
    std::atomic<float>& wet;
    std::atomic<float>& duck;
};

} // namespace argentum
