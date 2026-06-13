#pragma once
#include <JuceHeader.h>

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    void prepare(double sampleRate, int maxBlockSize);
    void process(const float* audioData, int numSamples);
    void reset();

    int  getFFTSize()      const { return fftSize; }
    int  getDisplayBins()  const { return displayBins; }
    float getSmoothedMag(int displayBin) const;

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;   // 4096
    static constexpr int displayBins = 512;
    static constexpr float smoothAlpha = 0.25f;

private:
    void computeSmoothedDisplayFrame();

    juce::dsp::FFT fft{ fftOrder };
    juce::dsp::WindowingFunction<float> window{ static_cast<size_t>(fftSize),
        juce::dsp::WindowingFunction<float>::hann };

    double sampleRate = 44100.0;

    std::atomic<float> rawMags[fftSize];
    std::atomic<float> displayMags[displayBins];
    float              prevDisplayMags[displayBins] = {};
    std::atomic<float> maxMag{ 1e-6f };
};
