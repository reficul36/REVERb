#include "PluginEditor.h"
#include "Dsp/Presets.h"

using namespace argentum;

namespace
{
    struct GroupDef { const char* title; juce::Colour accent; std::vector<int> paramIndices; };
}

static const GroupDef kGroups[4] =
{
    { "SPACE",  Colours::cyanBright, { 0, 1, 2, 3 } },        // predelay, size, decay, diffusion
    { "TONE",   Colours::green,      { 4, 5, 6, 9, 10 } },    // damp, bassMul, crossover, lowCut, highCut
    { "MOTION", Colours::purple,     { 7, 8, 11 } },          // modDepth, modRate, width
    { "OUTPUT", Colours::amber,      { 12, 13 } },            // duck, mix
};

ArgentumPlateEditor::ArgentumPlateEditor (ArgentumPlateProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      meter (p.dsp.lastInRms, p.dsp.lastWetRms, p.dsp.lastDuck)
{
    setLookAndFeel (&laf);
    setResizable (true, true);
    setResizeLimits (860, 640, 1600, 1200);
    setSize (1120, 800);

    title1.setText ("ARGENTUM", juce::dontSendNotification);
    title1.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    title1.setColour (juce::Label::textColourId, juce::Colour (0xfff4f4f5));
    addAndMakeVisible (title1);

    title2.setText ("PLATE", juce::dontSendNotification);
    title2.setFont (juce::Font (juce::FontOptions (16.0f)));
    title2.setColour (juce::Label::textColourId, Colours::cyan);
    addAndMakeVisible (title2);

    vstTag.setText ("VST3 . x64", juce::dontSendNotification);
    vstTag.setFont (juce::Font (juce::FontOptions (9.0f)));
    vstTag.setColour (juce::Label::textColourId, Colours::textDimmer);
    addAndMakeVisible (vstTag);

    // preset combo, grouped by category to mirror the <optgroup> layout
    int idx = 1;
    for (const char* cat : { "VOCAL", "DRUMS", "SYNTH", "GLUE", "FX" })
    {
        presetBox.addSectionHeading (cat);
        for (auto& preset : factoryPresets())
            if (preset.category == cat)
                presetBox.addItem (preset.name, idx++);
    }
    presetBox.onChange = [this]
    {
        const int sel = presetBox.getSelectedItemIndex();
        if (sel >= 0) proc.loadPreset (sel);
    };
    addAndMakeVisible (presetBox);

    initBtn.onClick = [this] { proc.resetToDefaults(); };
    initBtn.setTooltip ("INIT - reset every parameter to the reference 140 state.");
    addAndMakeVisible (initBtn);

    freezeBtn.setClickingTogglesState (true);
    freezeBtn.setColour (juce::TextButton::buttonColourId, Colours::amber);
    freezeBtn.setTooltip ("FREEZE - mutes the tank input and pins feedback near unity. Infinite sustain with no runaway gain.");
    addAndMakeVisible (freezeBtn);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kFreezeId, freezeBtn);

    bypassBtn.setClickingTogglesState (true);
    bypassBtn.setColour (juce::TextButton::buttonColourId, Colours::rose);
    bypassBtn.setTooltip ("BYPASS - true bypass of the wet path, dry passes through unchanged.");
    addAndMakeVisible (bypassBtn);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kBypassId, bypassBtn);

    trueStBtn.setClickingTogglesState (true);
    trueStBtn.setColour (juce::TextButton::buttonColourId, Colours::cyan);
    trueStBtn.setTooltip ("TRUE STEREO - independent diffuser chains per channel. Disable for authentic vintage mono-summed behaviour.");
    addAndMakeVisible (trueStBtn);
    trueStAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kTrueStereoId, trueStBtn);

    alloyLabel.setText ("ALLOY", juce::dontSendNotification);
    alloyLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    alloyLabel.setColour (juce::Label::textColourId, Colours::textDimmer);
    addAndMakeVisible (alloyLabel);

    auto* modeParam = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (kModeId));
    for (int i = 0; i < 5; ++i)
    {
        auto& b = modeButtons[i];
        b.setButtonText (kModeNames[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (9001, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonColourId, Colours::cyan);
        b.setTooltip (juce::String (kModeNames[i]) + " - " + kModeBlurbs[i]);
        b.setToggleState (i == 0, juce::dontSendNotification);
        b.onClick = [this, i, modeParam]
        {
            if (modeParam != nullptr)
                modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) i));
        };
        addAndMakeVisible (b);
    }

    modeNameLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    modeNameLabel.setColour (juce::Label::textColourId, Colours::textDimmer);
    modeNameLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (modeNameLabel);

    addAndMakeVisible (decayCurve);
    addAndMakeVisible (meter);

    for (int g = 0; g < 4; ++g)
    {
        groupTitles[g].setText (kGroups[g].title, juce::dontSendNotification);
        groupTitles[g].setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        groupTitles[g].setColour (juce::Label::textColourId, kGroups[g].accent);
        addAndMakeVisible (groupTitles[g]);

        for (int paramIdx : kGroups[g].paramIndices)
        {
            auto knob = std::make_unique<KnobComponent> (proc.apvts, kParams[paramIdx], kGroups[g].accent);
            addAndMakeVisible (*knob);
            knobs.push_back (std::move (knob));
        }
    }

    footerLabel.setText ("ARGENTUM PLATE - DATTORRO FIGURE-OF-EIGHT TANK - PORTED FROM THE REFERENCE WEB MODEL",
                          juce::dontSendNotification);
    footerLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    footerLabel.setColour (juce::Label::textColourId, juce::Colour (0xff3f3f46));
    footerLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (footerLabel);

    startTimerHz (15);
}

ArgentumPlateEditor::~ArgentumPlateEditor()
{
    setLookAndFeel (nullptr);
}

void ArgentumPlateEditor::timerCallback()
{
    DecayCurveInputs in;
    in.size      = proc.apvts.getRawParameterValue ("size")->load();
    in.decay     = proc.apvts.getRawParameterValue ("decay")->load();
    in.damp      = proc.apvts.getRawParameterValue ("damp")->load();
    in.bassMul   = proc.apvts.getRawParameterValue ("bassMul")->load();
    in.crossover = proc.apvts.getRawParameterValue ("crossover")->load();
    const int mode = juce::jlimit (0, 4, (int) proc.apvts.getRawParameterValue (kModeId)->load());
    decayCurve.setValues (in, kModes[mode].hfMul);
    modeNameLabel.setText (kModeNames[mode], juce::dontSendNotification);

    // Keep the ALLOY buttons in sync with the parameter (e.g. after a preset load).
    for (int i = 0; i < 5; ++i)
        if (modeButtons[i].getToggleState() != (i == mode))
            modeButtons[i].setToggleState (i == mode, juce::dontSendNotification);
}

void ArgentumPlateEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::bg);
    auto panel = getLocalBounds().reduced (12).toFloat();
    g.setColour (Colours::panelBg);
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (Colours::border);
    g.drawRoundedRectangle (panel.reduced (0.5f), 12.0f, 1.0f);
}

void ArgentumPlateEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    // header
    auto header = area.removeFromTop (40);
    title1.setBounds (header.removeFromLeft (90));
    title2.setBounds (header.removeFromLeft (60));
    vstTag.setBounds (header.removeFromLeft (80));
    header.removeFromLeft (12);
    bypassBtn.setBounds (header.removeFromRight (72).reduced (2));
    freezeBtn.setBounds (header.removeFromRight (72).reduced (2));
    initBtn.setBounds (header.removeFromRight (56).reduced (2));
    presetBox.setBounds (header.removeFromRight (240).reduced (2));

    area.removeFromTop (10);

    // alloy row
    auto alloyRow = area.removeFromTop (30);
    alloyLabel.setBounds (alloyRow.removeFromLeft (46));
    for (auto& b : modeButtons)
        b.setBounds (alloyRow.removeFromLeft (92).reduced (2));
    modeNameLabel.setBounds (alloyRow.removeFromRight (110));
    trueStBtn.setBounds (alloyRow.removeFromRight (70).reduced (2));

    area.removeFromTop (12);

    // displays: decay curve (left, big) + meter (right, narrow)
    auto displays = area.removeFromTop (220);
    meter.setBounds (displays.removeFromRight (90).reduced (4));
    displays.removeFromRight (8);
    decayCurve.setBounds (displays);

    area.removeFromTop (14);

    // knob groups, 4 across
    auto knobArea = area.removeFromTop (area.getHeight() - 60);
    const int groupW = knobArea.getWidth() / 4;
    int knobCursor = 0;
    for (int g = 0; g < 4; ++g)
    {
        auto gArea = knobArea.removeFromLeft (groupW).reduced (6);
        groupTitles[g].setBounds (gArea.removeFromTop (18));
        gArea.removeFromTop (4);

        const int n = (int) kGroups[g].paramIndices.size();
        const int cols = n > 4 ? 3 : 2;
        const int rows = (n + cols - 1) / cols;
        const int cellW = gArea.getWidth() / cols;
        const int cellH = juce::jmin (110, gArea.getHeight() / juce::jmax (1, rows));
        for (int i = 0; i < n; ++i)
        {
            const int col = i % cols, row = i / cols;
            auto cell = juce::Rectangle<int> (gArea.getX() + col * cellW, gArea.getY() + row * cellH, cellW, cellH);
            knobs[(size_t) knobCursor++]->setBounds (cell.reduced (4));
        }
    }

    area.removeFromTop (8);
    footerLabel.setBounds (area);
}
