#pragma once

#include <JuceHeader.h>

// ============================================================
// 频谱分析引擎 —— 独立于 Processor，只负责 FFT + 平滑 + 插值
// ============================================================
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    void prepare(double sampleRate, int maxBlockSize);
    void process(const float* audioData, int numSamples);   // 推入音频帧做 FFT
    void reset();

    // ---- 查询接口（供 GUI 线程安全读取） ----
    int   getFFTSize()       const { return fftSize; }
    int   getDisplayBins()   const { return displayBins; }
    float getSmoothedMag(int displayBin) const;   // 0..1 归一化
    float getRawMag(int fftBin)         const;

    static constexpr int fftOrder = 12;            // 4096
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int displayBins = 512;
    static constexpr float smoothAlpha = 0.25f;

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    double sampleRate = 44100.0;

    // 瞬时 FFT 幅度（音频线程写，GUI 线程读）
    std::atomic<float> rawMags[fftSize];

    // 平滑 + 插值后的幅度（音频线程写完，GUI 线程读）
    std::atomic<float> displayMags[displayBins];

    // 上一帧的 displayMags（用于指数平滑的旧值）
    float prevDisplayMags[displayBins] = {};

    // 当前显示帧的最大幅度（用于自适应归一化）
    std::atomic<float> maxMag{ 1e-6f };

    // ---- 内部方法 ----
    static float binToFreq(int bin, double sr, int n);
    void   computeSmoothedDisplayFrame();
};
