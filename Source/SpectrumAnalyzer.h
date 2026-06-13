#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    void prepare(double sampleRate, int maxBlockSize);
    void process(const float* audioData, int numSamples);
    void reset();

    int   getFFTSize()     const { return fftSize; }
    int   getDisplayBins() const { return displayBins; }
    float getSmoothedMag(int displayBin) const;
    float getRawMag(int fftBin)         const;

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int displayBins = 512;
    static constexpr float smoothAlpha = 0.25f;

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    double sampleRate = 44100.0;

    std::atomic<float> rawMags[fftSize];
    std::atomic<float> displayMags[displayBins];
    float prevDisplayMags[displayBins] = {};
    std::atomic<float> maxMag{ 1e-6f };

    void computeSmoothedDisplayFrame();
};
