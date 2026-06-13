#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrogramAndLowPassAudioProcessorEditor::SpectrogramAndLowPassAudioProcessorEditor(
    SpectrogramAndLowPassAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , waterfall(p.getAnalyzer(), p.getSampleRatePtr())
{
    titleLabel.setText("Spectrogram", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

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
    g.fillAll(juce::Colour(0xff0e0e1a));
}

void SpectrogramAndLowPassAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(28));
    waterfall.setBounds(area.reduced(0, 6));
}
