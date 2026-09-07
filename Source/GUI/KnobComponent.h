#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Dsp/ParamLayout.h"
#include "PluginLookAndFeel.h"

namespace argentum
{

/** Rotary knob + short label + formatted value text, stacked vertically
    like the web version's <Knob/> component. Double-click resets to default,
    matching the original's onDoubleClick behaviour. */
class KnobComponent : public juce::Component
{
public:
    KnobComponent (juce::AudioProcessorValueTreeState& apvts, const ParamSpec& specIn, juce::Colour accentIn)
        : spec (specIn), accent (accentIn)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * -0.75f,
                                     juce::MathConstants<float>::pi * 0.75f, true);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setDoubleClickReturnValue (true, spec.def);
        slider.setTooltip (juce::String (spec.label) + " - " + spec.tip);
        addAndMakeVisible (slider);

        shortLabel.setText (spec.shortLbl, juce::dontSendNotification);
        shortLabel.setJustificationType (juce::Justification::centred);
        shortLabel.setColour (juce::Label::textColourId, Colours::textDim);
        shortLabel.setFont (juce::Font (juce::FontOptions (9.5f)));
        addAndMakeVisible (shortLabel);

        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setColour (juce::Label::textColourId, Colours::text);
        valueLabel.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::plain)));
        addAndMakeVisible (valueLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, spec.id, slider);
        slider.onValueChange = [this] { updateValueLabel(); };
        updateValueLabel();
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto labels = b.removeFromBottom (28);
        slider.setBounds (b);
        shortLabel.setBounds (labels.removeFromTop (13));
        valueLabel.setBounds (labels);
    }

private:
    void updateValueLabel()
    {
        const float v = (float) slider.getValue();
        juce::String text;
        // Formatting rules ported from PARAMS[id].fmt() in params.ts
        const juce::String id (spec.id);
        if (id == "predelay")      text = juce::String (v, 1) + " ms";
        else if (id == "size" || id == "diffusion" || id == "modDepth" || id == "width" || id == "mix")
            text = juce::String (juce::roundToInt (v * 100.0f)) + " %";
        else if (id == "decay")   text = (v >= 10.0f ? juce::String (v, 1) : juce::String (v, 2)) + " s";
        else if (id == "damp" || id == "crossover" || id == "lowCut" || id == "highCut")
        {
            text = v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + "k Hz"
                                 : juce::String (juce::roundToInt (v)) + " Hz";
        }
        else if (id == "bassMul") text = juce::String (v, 2) + " x";
        else if (id == "modRate") text = juce::String (v, 2) + " Hz";
        else if (id == "duck")    text = v <= 0.0001f ? "OFF" : juce::String (juce::roundToInt (v * 100.0f)) + " %";
        else                       text = juce::String (v);
        valueLabel.setText (text, juce::dontSendNotification);
    }

    const ParamSpec& spec;
    juce::Colour accent;
    juce::Slider slider;
    juce::Label shortLabel, valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

} // namespace argentum
