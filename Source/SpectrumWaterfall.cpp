#include "SpectrumWaterfall.h"
#include "SpectrumAnalyzer.h"

SpectrumWaterfall::SpectrumWaterfall(SpectrumAnalyzer& an, float* srPtr)
    : analyzer(an)
    , currentSampleRate(srPtr)
{
    for (auto& frame : spectrumHistory)
        for (auto& v : frame)
            v = 0.0f;

    for (auto& x : logXCoords) x = 0.0f;
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

// ===== 预计算对数 X 坐标（cent 差值正比于频距，只 resize 时重建） =====
void SpectrumWaterfall::rebuildXCoords(float width, float sr)
{
    // FIX: constexpr → const，MSVC 的 std::log2 不是 constexpr
    static const float minFreq = 20.0f;
    static const float maxFreq = 20000.0f;
    static const float A4 = 440.0f;
    static const float centMin = 1200.0f * std::log2(minFreq / A4);
    static const float centMax = 1200.0f * std::log2(maxFreq / A4);

    const int fftN = analyzer.getFFTSize();

    for (int i = 0; i < displayBins; ++i)
    {
        const float rawBin = static_cast<float>(i)
            * static_cast<float>(fftN)
            / static_cast<float>(displayBins);
        const float freq = rawBin * sr / static_cast<float>(fftN);
        const float clamped = juce::jlimit(minFreq, maxFreq, freq);
        const float cent = 1200.0f * std::log2(clamped / A4);
        const float t = (cent - centMin) / (centMax - centMin);

        logXCoords[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, t) * width;
    }

    lastWidth = width;
    lastSampleRate = sr;
}

// ===== 60Hz 回调：快照一帧到环形缓冲 =====
void SpectrumWaterfall::timerCallback()
{
    for (int i = 0; i < displayBins; ++i)
        spectrumHistory[writeIdx][i] = analyzer.getSmoothedMag(i);

    writeIdx = (writeIdx + 1) % historyFrames;
    if (validFrames < historyFrames)
        ++validFrames;

    repaint();
}

// ===== 绘制 =====
void SpectrumWaterfall::paint(juce::Graphics& g)
{
    // FIX: bounds 改为非 const，因为 removeFromTop 是 non-const 成员函数
    juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // 背景
    g.fillAll(juce::Colour(0xff070710));

    // ---- 需要时重建 X 坐标 ----
    const float sr = (currentSampleRate) ? *currentSampleRate : 44100.0f;
    if (std::abs(w - lastWidth) > 0.5f || std::abs(sr - lastSampleRate) > 1.0f)
        rebuildXCoords(w, sr);

    if (validFrames == 0)
        return;

    const float rowHeight = h / static_cast<float>(historyFrames);

    // ---- 逐帧绘制（顶部 = 最旧，底部 = 最新） ----
    for (int f = 0; f < validFrames; ++f)
    {
        const int   idx = (writeIdx - validFrames + f + historyFrames) % historyFrames;
        const float y = static_cast<float>(f) * rowHeight;

        for (int i = 0; i < displayBins - 1; ++i)
        {
            const float mag = spectrumHistory[idx][i];

            juce::Colour col;
            if (mag < 0.08f)
                col = juce::Colour(0xff040410);
            else if (mag < 0.25f)
                col = juce::Colour::fromHSV(0.60f, 0.90f, mag * 1.5f, 1.0f);
            else if (mag < 0.55f)
                col = juce::Colour::fromHSV(0.40f, 0.75f, mag * 1.3f, 1.0f);
            else if (mag < 0.80f)
                col = juce::Colour::fromHSV(0.22f, 0.60f, mag * 1.1f, 1.0f);
            else
                col = juce::Colour::fromHSV(0.13f, 0.95f, 1.0f, 1.0f);

            g.setColour(col);

            const float x1 = logXCoords[static_cast<size_t>(i)];
            const float x2 = logXCoords[static_cast<size_t>(i + 1)];
            const float bw = juce::jmax(1.0f, x2 - x1);
            g.fillRect(x1, y, bw, rowHeight + 1.0f);
        }
    }

    // ---- 频率轴标注 ----
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.setFont(juce::FontOptions(10.0f));

    const float markers[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* labels[] = { "50","100","200","500","1k","2k","5k","10k","20k" };

    // FIX: constexpr → static const
    static const float mMin = 20.0f;
    static const float mA4 = 440.0f;
    static const float cMin = 1200.0f * std::log2(mMin / mA4);
    static const float cMax = 1200.0f * std::log2(20000.0f / mA4);

    for (int m = 0; m < 9; ++m)
    {
        const float c = 1200.0f * std::log2(markers[m] / mA4);
        const float nx = (c - cMin) / (cMax - cMin);
        const float x = nx * w;
        g.drawLine(x, h - 16.0f, x, h - 8.0f);
        g.drawText(juce::String(labels[m]) + "Hz",
            x - 24.0f, h - 20.0f, 48.0f, 13.0f,
            juce::Justification::centred);
    }

    // 小标题
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("log-scaled  ·  cent-spaced  ·  60fps",
        bounds.removeFromTop(14.0f).reduced(6.0f, 0.0f),
        juce::Justification::centredLeft);
}
