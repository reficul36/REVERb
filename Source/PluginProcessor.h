#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp/PlateReverbDSP.h"
#include "Dsp/ParamLayout.h"

class ArgentumPlateProcessor : public juce::AudioProcessor
{
public:
    ArgentumPlateProcessor();
    ~ArgentumPlateProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Argentum Plate"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadPreset (int index);
    void resetToDefaults();

    juce::AudioProcessorValueTreeState apvts;
    argentum::PlateReverbDSP dsp;

private:
    argentum::PlateParams gatherParams();

    std::atomic<float>* floatParam[argentum::kNumContinuousParams] {};
    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* trueStereoParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    juce::AudioBuffer<float> scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArgentumPlateProcessor)
};
