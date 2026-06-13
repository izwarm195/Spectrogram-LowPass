#pragma once

#include <JuceHeader.h>

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

    float getFFTMagnitude(int index) const;
    int   getFFTSize() const;
    float getMaxMagnitude() const;
    float getSampleRate() const;

    juce::AudioProcessorValueTreeState& getState() { return parameters; }

private:
    // 注意：parameters 在初始化列表中构造（见 .cpp）
    juce::AudioProcessorValueTreeState parameters;

    using Filter = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorDuplicator<Filter, juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    std::atomic<float> fftMagnitudes[fftSize];
    std::atomic<float> maxMagnitude{ 0.0f };

    std::atomic<float>* cutoffParam = nullptr;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramAndLowPassAudioProcessor)
};
