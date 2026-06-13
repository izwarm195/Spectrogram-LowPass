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
    SpectrumWaterfall                   waterfall;
    juce::Label                         titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramAndLowPassAudioProcessorEditor)
};
