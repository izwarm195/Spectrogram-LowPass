#pragma once
#include <JuceHeader.h>

class SpectrumAnalyzer;

// ============================================================
// 频谱瀑布图组件
// 内部维护一张环形 Image 缓冲区，timer 逐行写入频谱颜色，
// paint() 只做 blit，保证 30fps 下流畅运行
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

private:
    SpectrumAnalyzer& analyzer;
    float* currentSampleRate;

    static constexpr int displayBins = 512;
    static constexpr int defaultHeight = 360;

    // ---- 瀑布 Image ----
    juce::Image waterfallImage;
    int        currentRow = 0;   // 下一帧写入的目标行（环形）

    // ---- 对数刻度 X 坐标缓存 ----
    std::array<float, displayBins> logXCoords{};
    float lastWidth = 0.0f;
    float lastSampleRate = 0.0f;

    void rebuildXCoords(float width, float sr);
    void ensureImageSize(int w, int h);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumWaterfall)
};
