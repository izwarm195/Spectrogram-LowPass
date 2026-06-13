#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzer;

// ============================================================
// 频谱瀑布图 —— Image 帧缓冲实现，零卡顿
// 每帧：Image 上移 1px → 底部画新频谱行 → paint 只 blit
// ============================================================
class SpectrumWaterfall : public juce::Component,
    public juce::Timer
{
public:
    SpectrumWaterfall(SpectrumAnalyzer& analyzer, float* sampleRatePtr);
    ~SpectrumWaterfall() override;

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    void startAnimation();
    void stopAnimation();

private:
    SpectrumAnalyzer& analyzer;
    float* currentSampleRate;

    static constexpr int displayBins = 512;
    static constexpr int historyHeight = 360;      // 6 秒 @60fps，或 12 秒 @30fps

    // 帧缓冲
    juce::Image waterfallImage;
    int        currentRow = 0;   // 当前写入行（0 = 底部，向上增长）

    // 对数 X 坐标（只 resize 时重建）
    std::array<float, displayBins> logXCoords{};
    float lastWidth = 0.0f;
    float lastSampleRate = 0.0f;

    void rebuildXCoords(float width, float sr);
    void ensureImageSize(int w, int h);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumWaterfall)
};
