#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrogramAndLowPassAudioProcessorEditor::SpectrogramAndLowPassAudioProcessorEditor(
    SpectrogramAndLowPassAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , waterfall(p.getAnalyzer(), p.getSampleRatePtr())
{
    // 标题
    titleLabel.setText("Spectrogram & Low Pass", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // 瀑布图
    addAndMakeVisible(waterfall);
    waterfall.startAnimation();  // 启动 60fps

    // 截止频率旋钮
    cutoffSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    cutoffSlider.setRange(20.0, 20000.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    cutoffSlider.setTextValueSuffix(" Hz");
    cutoffSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00bcd4));
    cutoffSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1a30));
    cutoffSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(cutoffSlider);

    cutoffLabel.setText("Low-Pass Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    cutoffLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(cutoffLabel);

    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getState(), "cutoff", cutoffSlider);

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

    auto controlsArea = area.removeFromBottom(105);

    auto knobRect = controlsArea.removeFromLeft(130);
    cutoffLabel.setBounds(knobRect.removeFromBottom(20));
    cutoffSlider.setBounds(knobRect);

    waterfall.setBounds(area.reduced(0, 6));
}
