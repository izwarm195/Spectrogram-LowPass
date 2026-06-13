#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumWaterfall.h"

class SpectrogramAndLowPassAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit SpectrogramAndLowPassAudioProcessorEditor(SpectrogramAndLowPassAudioProcessor&);
    ~SpectrogramAndLowPassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SpectrogramAndLowPassAudioProcessor& audioProcessor;

    // 瀑布图（独立组件，运行在 60fps Timer）
    SpectrumWaterfall waterfall;

    // 旋钮 + 标签
    juce::Slider cutoffSlider;
    juce::Label  cutoffLabel;
    juce::Label  titleLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramAndLowPassAudioProcessorEditor)
};
