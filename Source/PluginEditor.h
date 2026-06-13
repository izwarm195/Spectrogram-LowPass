#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrogramAndLowPassAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit SpectrogramAndLowPassAudioProcessorEditor(SpectrogramAndLowPassAudioProcessor&);
    ~SpectrogramAndLowPassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // ========== 频谱瀑布图子组件 ==========
    class SpectrumWaterfall : public juce::Component,
        public juce::Timer
    {
    public:
        explicit SpectrumWaterfall(SpectrogramAndLowPassAudioProcessor&);
        void timerCallback() override;
        void paint(juce::Graphics&) override;

    private:
        SpectrogramAndLowPassAudioProcessor& proc;

        static constexpr int historyFrames = 240;
        static constexpr int displayBins = 512;

        float spectrumHistory[historyFrames][displayBins] = {};
        int   writeIdx = 0;
        int   validFrames = 0;

        // 对数映射：频率 → 归一化 X（基于 cent 差值）
        static float freqToNormX(float bin, float fftN, float sr);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumWaterfall)
    };

    // ========== 成员 ==========
    SpectrogramAndLowPassAudioProcessor& audioProcessor;
    SpectrumWaterfall                   waterfall;

    juce::Slider cutoffSlider;
    juce::Label  cutoffLabel;
    juce::Label  titleLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramAndLowPassAudioProcessorEditor)
};
