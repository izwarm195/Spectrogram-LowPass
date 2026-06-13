#include "SpectrumWaterfall.h"
#include "SpectrumAnalyzer.h"

SpectrumWaterfall::SpectrumWaterfall(SpectrumAnalyzer& an, float* srPtr)
    : analyzer(an)
    , currentSampleRate(srPtr)
{
    for (auto& x : logXCoords) x = 0.0f;
    waterfallImage = juce::Image(juce::Image::RGB, 1, 1, false);
}

SpectrumWaterfall::~SpectrumWaterfall()
{
    stopTimer();
}

void SpectrumWaterfall::startAnimation()
{
    startTimerHz(60);
}

void SpectrumWaterfall::stopAnimation()
{
    stopTimer();
}

void SpectrumWaterfall::resized()
{
    ensureImageSize(getWidth(), getHeight());
}

void SpectrumWaterfall::ensureImageSize(int w, int h)
{
    if (waterfallImage.getWidth() != w || waterfallImage.getHeight() != h)
    {
        waterfallImage = juce::Image(juce::Image::RGB, juce::jmax(1, w), juce::jmax(1, h), false);
        juce::Graphics ig(waterfallImage);
        ig.fillAll(juce::Colour(0xff070710));
        currentRow = 0;
    }
}

void SpectrumWaterfall::rebuildXCoords(float width, float sr)
{
    static const float minFreq = 20.0f;
    static const float maxFreq = 20000.0f;
    static const float A4 = 440.0f;
    static const float centMin = 1200.0f * std::log2(minFreq / A4);
    static const float centMax = 1200.0f * std::log2(maxFreq / A4);

    const int fftN = analyzer.getFFTSize();
    for (int i = 0; i < displayBins; ++i)
    {
        const float rawBin = static_cast<float>(i) * static_cast<float>(fftN) / static_cast<float>(displayBins);
        const float freq = rawBin * sr / static_cast<float>(fftN);
        const float clamped = juce::jlimit(minFreq, maxFreq, freq);
        const float cent = 1200.0f * std::log2(clamped / A4);
        const float t = (cent - centMin) / (centMax - centMin);
        logXCoords[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, t) * width;
    }
    lastWidth = width;
    lastSampleRate = sr;
}

void SpectrumWaterfall::timerCallback()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    ensureImageSize(getWidth(), getHeight());

    const int imgW = waterfallImage.getWidth();
    const int imgH = waterfallImage.getHeight();

    juce::Graphics ig(waterfallImage);

    // ---- 1. 在当前行绘制新频谱 ----
    const float sr = (currentSampleRate) ? *currentSampleRate : 44100.0f;
    if (std::abs(static_cast<float>(imgW) - lastWidth) > 0.5f
        || std::abs(sr - lastSampleRate) > 1.0f)
        rebuildXCoords(static_cast<float>(imgW), sr);

    for (int i = 0; i < displayBins - 1; ++i)
    {
        const float mag = analyzer.getSmoothedMag(i);
        juce::Colour col;

        if (mag < 0.01f)
            col = juce::Colour(0xff101030);
        else if (mag < 0.05f)
            col = juce::Colour::fromHSV(0.62f, 0.95f, mag * 6.0f, 1.0f);
        else if (mag < 0.15f)
            col = juce::Colour::fromHSV(0.55f, 0.90f, mag * 2.5f, 1.0f);
        else if (mag < 0.40f)
            col = juce::Colour::fromHSV(0.35f, 0.75f, mag * 1.6f, 1.0f);
        else if (mag < 0.70f)
            col = juce::Colour::fromHSV(0.18f, 0.65f, mag * 1.2f, 1.0f);
        else
            col = juce::Colour::fromHSV(0.13f, 0.95f, 1.0f, 1.0f);

        ig.setColour(col);

        const float x1 = logXCoords[static_cast<size_t>(i)];
        const float x2 = logXCoords[static_cast<size_t>(i + 1)];
        const float bw = juce::jmax(1.0f, x2 - x1);
        ig.fillRect(x1, static_cast<float>(currentRow), bw, 1.0f);
    }

    // ---- 2. 推进 currentRow（环形），下一行填入背景色 ----
    currentRow = (currentRow + 1) % imgH;
    ig.setColour(juce::Colour(0xff070710));
    ig.fillRect(0.0f, static_cast<float>(currentRow), static_cast<float>(imgW), 1.0f);

    repaint();
}

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

    // ---- cent 频段竖线网格（无文字） ----
    g.setColour(juce::Colours::grey.withAlpha(0.15f));

    static const float minFreq = 20.0f;
    static const float maxFreq = 20000.0f;
    static const float A4 = 440.0f;
    static const float centMin = 1200.0f * std::log2(minFreq / A4);
    static const float centMax = 1200.0f * std::log2(maxFreq / A4);
    static const float centSpan = centMax - centMin;

    // 每 1 cent 一条线，约 12000 条，渲染开销可控
    for (int c = static_cast<int>(std::ceil(centMin)); c <= static_cast<int>(centMax); ++c)
    {
        const float nx = (static_cast<float>(c) - centMin) / centSpan;
        const float x = nx * w;
        g.drawVerticalLine(static_cast<int>(x), 0.0f, h);
    }
}
