#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/PluginLookAndFeel.h"
#include "GUI/KnobComponent.h"
#include "GUI/DecayCurveComponent.h"
#include "GUI/MeterComponent.h"

class ArgentumPlateEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ArgentumPlateEditor (ArgentumPlateProcessor&);
    ~ArgentumPlateEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ArgentumPlateProcessor& proc;
    argentum::PluginLookAndFeel laf;

    juce::Label title1, title2, vstTag;
    juce::ComboBox presetBox;
    juce::TextButton initBtn { "INIT" }, freezeBtn { "FREEZE" }, bypassBtn { "BYPASS" }, trueStBtn { "TRUE ST" };
    juce::TextButton modeButtons[5];
    juce::Label alloyLabel, modeNameLabel, footerLabel;

    argentum::DecayCurveComponent decayCurve;
    argentum::MeterComponent meter;

    juce::Label groupTitles[4];
    std::vector<std::unique_ptr<argentum::KnobComponent>> knobs;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach, bypassAttach, trueStAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArgentumPlateEditor)
};
