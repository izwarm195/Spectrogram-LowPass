#include "SpectrumWaterfall.h"
#include "SpectrumAnalyzer.h"

SpectrumWaterfall::SpectrumWaterfall(SpectrumAnalyzer& an, float* srPtr)
    : analyzer(an)
    , currentSampleRate(srPtr)
{
    for (auto& x : logXCoords) x = 0.0f;
    waterfallImage = juce::Image(juce::Image::RGB, 1, 1, false);
    startTimerHz(60);  // ★ 直接在构造函数启动 timer，不等外部调用
}

SpectrumWaterfall::~SpectrumWaterfall() { stopTimer(); }

// startAnimation / stopAnimation 保留但不再依赖它们

// ============ ensureImageSize: 修复点 1 ============
void SpectrumWaterfall::ensureImageSize(int w, int h)
{
    if (waterfallImage.getWidth() != w || waterfallImage.getHeight() != h)
    {
        // 用旧的 Image 做 resize 过渡，而不是直接丢弃
        juce::Image newImg(juce::Image::RGB, juce::jmax(1, w), juce::jmax(1, h), false);
        juce::Graphics ng(newImg);
        ng.fillAll(juce::Colour(0xff070710));

        // 把旧内容 blit 到新 Image 的底部（保留已绘制的历史行）
        if (waterfallImage.isValid() && waterfallImage.getWidth() > 0 && waterfallImage.getHeight() > 0)
        {
            const int oldH = waterfallImage.getHeight();
            const int copyH = juce::jmin(oldH, h);
            ng.drawImage(waterfallImage,
                0, h - copyH, w, copyH,
                0, juce::jmax(0, oldH - copyH), waterfallImage.getWidth(), copyH,
                false);
        }

        waterfallImage = newImg;
        currentRow = 0;  // 新行从顶部开始写，旧内容在下方保留
        rebuildXCoords(static_cast<float>(w),
            (currentSampleRate) ? *currentSampleRate : 44100.0f);
    }
}

// ============ timerCallback: 修复点 2 —— 加 idle 可视化 ============
void SpectrumWaterfall::timerCallback()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    ensureImageSize(getWidth(), getHeight());
    const int imgW = waterfallImage.getWidth();
    const int imgH = waterfallImage.getHeight();
    juce::Graphics ig(waterfallImage);

    const float sr = (currentSampleRate) ? *currentSampleRate : 44100.0f;
    if (std::abs(static_cast<float>(imgW) - lastWidth) > 0.5f ||
        std::abs(sr - lastSampleRate) > 1.0f)
        rebuildXCoords(static_cast<float>(imgW), sr);

    // 先检查是否有有效信号
    bool hasSignal = false;

    for (int i = 0; i < displayBins - 1; ++i)
    {
        const float mag = analyzer.getSmoothedMag(i);
        if (mag > 0.001f) hasSignal = true;

        juce::Colour col;
        if (mag < 0.01f)
            col = hasSignal
            ? juce::Colour(0xff101030)
            : juce::Colour(0xff080818);  // idle 时比背景稍亮，制造微弱的噪底
        else if (mag < 0.05f)  col = juce::Colour::fromHSV(0.62f, 0.95f, mag * 6.0f, 1.0f);
        else if (mag < 0.15f)  col = juce::Colour::fromHSV(0.55f, 0.90f, mag * 2.5f, 1.0f);
        else if (mag < 0.40f)  col = juce::Colour::fromHSV(0.35f, 0.75f, mag * 1.6f, 1.0f);
        else if (mag < 0.70f)  col = juce::Colour::fromHSV(0.18f, 0.65f, mag * 1.2f, 1.0f);
        else                   col = juce::Colour::fromHSV(0.13f, 0.95f, 1.0f, 1.0f);

        ig.setColour(col);
        const float x1 = logXCoords[static_cast<int>(i)];
        const float x2 = logXCoords[static_cast<int>(i + 1)];
        const float bw = juce::jmax(1.0f, x2 - x1);
        ig.fillRect(x1, static_cast<float>(currentRow), bw, 1.0f);
    }

    // 滚动
    currentRow = (currentRow + 1) % imgH;
    ig.setColour(juce::Colour(0xff070710));
    ig.fillRect(0.0f, static_cast<float>(currentRow), static_cast<float>(imgW), 1.0f);

    repaint();
}

// ============ paint: 修复点 3 —— cent 网格密度降低 ============
void SpectrumWaterfall::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    g.fillAll(juce::Colour(0xff070710));

    if (waterfallImage.isValid())
    {
        const int imgW = waterfallImage.getWidth();
        const int imgH = waterfallImage.getHeight();
        const int split = (currentRow + 1) % imgH;
        const int topH = imgH - split;
        const int botH = split;

        if (topH > 0)
            g.drawImage(waterfallImage,
                0, 0, imgW, topH,
                0, split, imgW, topH, false);
        if (botH > 0)
            g.drawImage(waterfallImage,
                0, static_cast<float>(topH), imgW, botH,
                0, 0, imgW, botH, false);
    }

    // ---- cent 频段竖线网格：每 100 cent 一根（~120 条） ----
    g.setColour(juce::Colours::grey.withAlpha(0.12f));

    static const float minFreq = 20.0f;
    static const float maxFreq = 20000.0f;
    static const float A4 = 440.0f;
    static const float centMin = 1200.0f * std::log2(minFreq / A4);
    static const float centMax = 1200.0f * std::log2(maxFreq / A4);
    static const float centSpan = centMax - centMin;

    const int step = 100;  // 每 100 cent 一条 → ~120 条线，性能无压力
    const int cStart = static_cast<int>(std::ceil(centMin / step)) * step;
    for (int c = cStart; c <= static_cast<int>(centMax); c += step)
    {
        const float nx = (static_cast<float>(c) - centMin) / centSpan;
        const float x = nx * w;
        g.drawVerticalLine(static_cast<int>(x), 0.0f, h);
    }
}
