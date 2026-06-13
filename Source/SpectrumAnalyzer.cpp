#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(fftOrder)
    , window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    for (auto& m : rawMags)     m.store(0.0f);
    for (auto& d : displayMags) d.store(0.0f);
    for (auto& p : prevDisplayMags) p = 0.0f;
}

void SpectrumAnalyzer::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
}

void SpectrumAnalyzer::reset()
{
    for (auto& m : rawMags)     m.store(0.0f);
    for (auto& d : displayMags) d.store(0.0f);
    for (auto& p : prevDisplayMags) p = 0.0f;
    maxMag.store(1e-6f);
}

void SpectrumAnalyzer::process(const float* audioData, int numSamples)
{
    std::vector<float> fftComplex(static_cast<size_t>(fftSize) * 2, 0.0f);
    const int copyLen = juce::jmin(numSamples, fftSize);

    std::copy_n(audioData, copyLen, fftComplex.begin());
    window.multiplyWithWindowingTable(fftComplex.data(), static_cast<size_t>(copyLen));
    fft.performFrequencyOnlyForwardTransform(fftComplex.data());

    float localMax = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        const float mag = fftComplex[static_cast<size_t>(i)];
        rawMags[i].store(mag, std::memory_order_relaxed);
        if (mag > localMax) localMax = mag;
    }

    const float oldMax = maxMag.load(std::memory_order_relaxed);
    const float trackAlpha = 0.05f;
    maxMag.store(oldMax * (1.0f - trackAlpha) + localMax * trackAlpha, std::memory_order_relaxed);

    computeSmoothedDisplayFrame();
}

void SpectrumAnalyzer::computeSmoothedDisplayFrame()
{
    const float invMax = 1.0f / juce::jmax(maxMag.load(std::memory_order_relaxed), 1e-6f);

    for (int i = 0; i < displayBins; ++i)
    {
        const float srcPos = static_cast<float>(i)
            * static_cast<float>(fftSize)
            / static_cast<float>(displayBins);
        const int   idxLo = static_cast<int>(srcPos);
        const int   idxHi = juce::jmin(idxLo + 1, fftSize - 1);
        const float frac = srcPos - static_cast<float>(idxLo);

        const float lo = rawMags[idxLo].load(std::memory_order_relaxed);
        const float hi = rawMags[idxHi].load(std::memory_order_relaxed);
        const float interp = lo + (hi - lo) * frac;
        const float normed = interp * invMax;

        const float smoothed = prevDisplayMags[i] * (1.0f - smoothAlpha)
            + normed * smoothAlpha;
        prevDisplayMags[i] = smoothed;
        displayMags[i].store(smoothed, std::memory_order_relaxed);
    }
}

float SpectrumAnalyzer::getSmoothedMag(int displayBin) const
{
    if (displayBin < 0 || displayBin >= displayBins) return 0.0f;
    return displayMags[displayBin].load(std::memory_order_relaxed);
}

float SpectrumAnalyzer::getRawMag(int fftBin) const
{
    if (fftBin < 0 || fftBin >= fftSize) return 0.0f;
    return rawMags[fftBin].load(std::memory_order_relaxed);
}
