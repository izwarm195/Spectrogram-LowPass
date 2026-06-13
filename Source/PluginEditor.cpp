#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrogramAndLowPassAudioProcessorEditor::SpectrogramAndLowPassAudioProcessorEditor(
    SpectrogramAndLowPassAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , waterfall(p.getAnalyzer(), p.getSampleRatePtr())
{
    addAndMakeVisible(waterfall);
    waterfall.startAnimation();

    setSize(740, 520);
}

SpectrogramAndLowPassAudioProcessorEditor::~SpectrogramAndLowPassAudioProcessorEditor()
{
    waterfall.stopAnimation();
}

void SpectrogramAndLowPassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff070710));
}

void SpectrogramAndLowPassAudioProcessorEditor::resized()
{
    waterfall.setBounds(getLocalBounds());
}
