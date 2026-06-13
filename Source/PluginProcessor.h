#pragma once

#include <JuceHeader.h>
#include "SpectrumAnalyzer.h"

class SpectrogramAndLowPassAudioProcessor : public juce::AudioProcessor
{
public:
    SpectrogramAndLowPassAudioProcessor();
    ~SpectrogramAndLowPassAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // 公开接口
    SpectrumAnalyzer& getAnalyzer() { return analyzer; }
    float             getSampleRate() const { return static_cast<float>(currentSampleRate); }
    float* getSampleRatePtr() { return &currentSampleRate; }

    juce::AudioProcessorValueTreeState& getState() { return parameters; }

private:
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* cutoffParam = nullptr;

    // 滤波器
    using Filter = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorDuplicator<Filter, juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    // 频谱分析器（独立的引擎）
    SpectrumAnalyzer analyzer;

    float currentSampleRate = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramAndLowPassAudioProcessor)
};
