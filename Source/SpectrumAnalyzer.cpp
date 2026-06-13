#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(fftOrder)
    , window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    for (auto& m : rawMags)  m.store(0.0f);
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
    for (auto& m : rawMags)  m.store(0.0f);
    for (auto& d : displayMags) d.store(0.0f);
    for (auto& p : prevDisplayMags) p = 0.0f;
    maxMag.store(1e-6f);
}

float SpectrumAnalyzer::binToFreq(int bin, double sr, int n)
{
    return static_cast<float>(bin) * static_cast<float>(sr) / static_cast<float>(n);
}

// ===== 核心：每帧音频进来做 FFT =====
void SpectrumAnalyzer::process(const float* audioData, int numSamples)
{
    // ---- 1. 填充 FFT 复数数组（实部为音频，虚部为 0） ----
    std::vector<float> fftComplex(static_cast<size_t>(fftSize) * 2, 0.0f);
    const int copyLen = juce::jmin(numSamples, fftSize);

    std::copy_n(audioData, copyLen, fftComplex.begin());

    // 加 Hann 窗
    window.multiplyWithWindowingTable(fftComplex.data(), static_cast<size_t>(copyLen));

    // 执行 FFT（幅值存回前半）
    fft.performFrequencyOnlyForwardTransform(fftComplex.data());

    // ---- 2. 写入 rawMags，同时求最大幅度 ----
    float localMax = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        const float mag = fftComplex[static_cast<size_t>(i)];
        rawMags[i].store(mag, std::memory_order_relaxed);
        if (mag > localMax) localMax = mag;
    }

    // ---- 3. 自适应：缓慢跟踪峰值用于归一化 ----
    const float oldMax = maxMag.load(std::memory_order_relaxed);
    const float trackAlpha = 0.05f;  // 缓慢跟踪
    maxMag.store(oldMax * (1.0f - trackAlpha) + localMax * trackAlpha, std::memory_order_relaxed);

    // ---- 4. 计算平滑插值的显示帧 ----
    computeSmoothedDisplayFrame();
}

// ===== 4096 raw bins → 512 display bins + 指数平滑 =====
void SpectrumAnalyzer::computeSmoothedDisplayFrame()
{
    const float invMax = 1.0f / juce::jmax(maxMag.load(std::memory_order_relaxed), 1e-6f);

    for (int i = 0; i < displayBins; ++i)
    {
        // 浮点映射：display bin → raw bin 位置
        const float srcPos = static_cast<float>(i)
            * static_cast<float>(fftSize)
            / static_cast<float>(displayBins);
        const int   idxLo = static_cast<int>(srcPos);
        const int   idxHi = juce::jmin(idxLo + 1, fftSize - 1);
        const float frac = srcPos - static_cast<float>(idxLo);

        // 在 raw bins 间线性插值
        const float lo = rawMags[idxLo].load(std::memory_order_relaxed);
        const float hi = rawMags[idxHi].load(std::memory_order_relaxed);
        const float interp = lo + (hi - lo) * frac;

        // 归一化
        const float normed = interp * invMax;

        // 指数平滑
        const float smoothed = prevDisplayMags[i] * (1.0f - smoothAlpha)
            + normed * smoothAlpha;
        prevDisplayMags[i] = smoothed;

        // 写入 atomic（GUI 线程安全读取）
        displayMags[i].store(smoothed, std::memory_order_relaxed);
    }
}

// ===== GUI 查询接口 =====
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
