#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../Dsp/ParamLayout.h"
#include "PluginLookAndFeel.h"

namespace argentum
{

/** Plots RT60(f) across 20 Hz - 20 kHz using the same analytic model the
    DSP's feedback path implements (rt60At in ParamLayout.h). A static,
    read-only rendering of what DecayCurve.tsx shows in the web build
    (that version also lets you drag two nodes to solve DECAY/BASS MULT
    for a target RT60 at a chosen frequency; this port focuses on giving
    an accurate visual reference). */
class DecayCurveComponent : public juce::Component
{
public:
    void setValues (const DecayCurveInputs& v, double hfMulIn)
    {
        inputs = v;
        hfMul = hfMulIn;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (Colours::groupBgLo);
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (Colours::border);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        auto plot = b.reduced (36.0f, 16.0f);

        // Compute RT60 across the frequency range first, to know the y-scale.
        constexpr int kPoints = 200;
        std::vector<float> freqs (kPoints), rt60s (kPoints);
        const double fMin = 20.0, fMax = 20000.0;
        double maxRt = 1.0;
        for (int i = 0; i < kPoints; ++i)
        {
            const double t = (double) i / (double) (kPoints - 1);
            const double f = fMin * std::pow (fMax / fMin, t);
            const double rt = rt60At (f, inputs, hfMul);
            freqs[(size_t) i] = (float) f;
            rt60s[(size_t) i] = (float) rt;
            maxRt = std::max (maxRt, rt);
        }
        maxRt = std::min (maxRt, 32.0);

        // Grid: octave lines + a couple of RT60 gridlines.
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (double f = 100.0; f < fMax; f *= 10.0)
        {
            const float x = plot.getX() + (float) (std::log (f / fMin) / std::log (fMax / fMin)) * plot.getWidth();
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        }
        for (int i = 1; i <= 4; ++i)
        {
            const float y = plot.getBottom() - (plot.getHeight() * (float) i / 4.0f);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        }

        // Curve
        juce::Path p;
        for (int i = 0; i < kPoints; ++i)
        {
            const float x = plot.getX() + (float) (std::log (freqs[(size_t) i] / fMin) / std::log (fMax / fMin)) * plot.getWidth();
            const float norm = juce::jlimit (0.0f, 1.0f, rt60s[(size_t) i] / (float) maxRt);
            const float y = plot.getBottom() - norm * plot.getHeight();
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (Colours::cyanBright);
        g.strokePath (p, juce::PathStrokeType (2.0f));

        juce::Path fill (p);
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (Colours::cyanBright.withAlpha (0.08f));
        g.fillPath (fill);

        // Axis labels
        g.setColour (Colours::textDimmer);
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawText ("RT60 (s)", b.getX() + 4, b.getY() + 2, 80.0f, 12.0f, juce::Justification::left);
        g.drawText (juce::String (maxRt, 1) + "s", plot.getX() - 32, plot.getY() - 4, 30.0f, 12.0f, juce::Justification::right);
        g.drawText ("0s", plot.getX() - 32, plot.getBottom() - 8, 30.0f, 12.0f, juce::Justification::right);
        for (double f : { 100.0, 1000.0, 10000.0 })
        {
            const float x = plot.getX() + (float) (std::log (f / fMin) / std::log (fMax / fMin)) * plot.getWidth();
            juce::String lbl = f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "k" : juce::String ((int) f);
            g.drawText (lbl, x - 15, plot.getBottom() + 2, 30.0f, 12.0f, juce::Justification::centred);
        }
    }

private:
    DecayCurveInputs inputs { 1.0, 2.4, 6200.0, 1.35, 380.0 };
    double hfMul = 1.0;
};

} // namespace argentum
