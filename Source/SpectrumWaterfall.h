#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzer;

// ============================================================
// 频谱瀑布图组件 —— 独立于 Editor，只管绘制
// 保证 60fps：用 OpenGL 兼容的 juce::Graphics 整帧重绘
// 对数 X 轴基于音乐 cent 差值
// ============================================================
class SpectrumWaterfall : public juce::Component,
    public juce::Timer
{
public:
    SpectrumWaterfall(SpectrumAnalyzer& analyzer, float* sampleRatePtr);
    ~SpectrumWaterfall() override;

    void timerCallback() override;
    void paint(juce::Graphics& g) override;

    void startAnimation();
    void stopAnimation();

private:
    SpectrumAnalyzer& analyzer;
    float* currentSampleRate;  // 指向 Processor 里的采样率

    static constexpr int historyFrames = 180;   // 3秒 @60fps
    static constexpr int displayBins = 512;

    // 紧凑预归一化历史缓冲（0..1 浮点）
    float spectrumHistory[historyFrames][displayBins] = {};
    int   writeIdx = 0;
    int   validFrames = 0;

    // 预计算对数 X 坐标（仅窗口 resize 时更新）
    std::array<float, displayBins> logXCoords{};
    float lastWidth = 0.0f;
    float lastSampleRate = 0.0f;

    void rebuildXCoords(float width, float sr);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumWaterfall)
};
