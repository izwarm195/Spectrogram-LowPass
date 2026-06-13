#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrogramAndLowPassAudioProcessorEditor::SpectrogramAndLowPassAudioProcessorEditor(
    SpectrogramAndLowPassAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , waterfall(p.getAnalyzer(), p.getSampleRatePtr())
{
    addAndMakeVisible(waterfall);
    // waterfall.startAnimation();  ← 删掉，timer 已在构造函数启动
    setResizable(true, true);
    setSize(800, 400);
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
