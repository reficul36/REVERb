#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Dsp/Presets.h"

ArgentumPlateProcessor::ArgentumPlateProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", argentum::createParameterLayout())
{
    for (int i = 0; i < argentum::kNumContinuousParams; ++i)
        floatParam[i] = apvts.getRawParameterValue (argentum::kParams[i].id);

    freezeParam     = apvts.getRawParameterValue (argentum::kFreezeId);
    bypassParam     = apvts.getRawParameterValue (argentum::kBypassId);
    trueStereoParam = apvts.getRawParameterValue (argentum::kTrueStereoId);
    modeParam       = apvts.getRawParameterValue (argentum::kModeId);
}

bool ArgentumPlateProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    auto in = layouts.getMainInputChannelSet();
    auto out = layouts.getMainOutputChannelSet();
    if (out != stereo) return false;
    if (in != stereo && in != mono) return false;
    return true;
}

void ArgentumPlateProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp.prepare (sampleRate);
    scratch.setSize (2, samplesPerBlock);
}

argentum::PlateParams ArgentumPlateProcessor::gatherParams()
{
    argentum::PlateParams p;
    p.predelay  = floatParam[0]->load();
    p.size      = floatParam[1]->load();
    p.decay     = floatParam[2]->load();
    p.diffusion = floatParam[3]->load();
    p.damp      = floatParam[4]->load();
    p.bassMul   = floatParam[5]->load();
    p.crossover = floatParam[6]->load();
    p.modDepth  = floatParam[7]->load();
    p.modRate   = floatParam[8]->load();
    p.lowCut    = floatParam[9]->load();
    p.highCut   = floatParam[10]->load();
    p.width     = floatParam[11]->load();
    p.duck      = floatParam[12]->load();
    p.mix       = floatParam[13]->load();
    p.freeze     = freezeParam->load() > 0.5f;
    p.bypass     = bypassParam->load() > 0.5f;
    p.trueStereo = trueStereoParam->load() > 0.5f;
    p.mode       = (int) modeParam->load();
    return p;
}

void ArgentumPlateProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    const float* inL = numIn > 0 ? buffer.getReadPointer (0) : nullptr;
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : inL;

    scratch.setSize (2, numSamples, false, false, true);
    float* outL = scratch.getWritePointer (0);
    float* outR = scratch.getWritePointer (1);

    dsp.processBlock (inL, inR, outL, outR, numSamples, gatherParams());

    if (numOut > 0) buffer.copyFrom (0, 0, outL, numSamples);
    if (numOut > 1) buffer.copyFrom (1, 0, outR, numSamples);
}

void ArgentumPlateProcessor::loadPreset (int index)
{
    const auto& presets = argentum::factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    const auto& preset = presets[index];

    for (auto& s : argentum::kParams)
    {
        float v = s.def;
        auto it = preset.values.find (s.id);
        if (it != preset.values.end()) v = it->second;
        if (auto* param = apvts.getParameter (s.id))
            param->setValueNotifyingHost (param->convertTo0to1 (v));
    }
    if (auto* param = apvts.getParameter (argentum::kModeId))
        param->setValueNotifyingHost (param->convertTo0to1 ((float) preset.mode));
}

void ArgentumPlateProcessor::resetToDefaults()
{
    for (auto& s : argentum::kParams)
        if (auto* param = apvts.getParameter (s.id))
            param->setValueNotifyingHost (param->convertTo0to1 (s.def));
    if (auto* param = apvts.getParameter (argentum::kModeId))
        param->setValueNotifyingHost (0.0f);
}

void ArgentumPlateProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ArgentumPlateProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ArgentumPlateProcessor::createEditor()
{
    return new ArgentumPlateEditor (*this);
}

// This creates the instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArgentumPlateProcessor();
}
