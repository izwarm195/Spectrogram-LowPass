#include "SpectrumWaterfall.h"
#include "SpectrumAnalyzer.h"

// ============================================================
// 颜色映射：深空主题 —— 弱信号蓝紫，强信号暖橙
// ============================================================
static juce::Colour colourForMag(float mag, bool idle)
{
    if (mag < 0.005f)
    {
        // idle 噪底：微弱的暗蓝，与纯背景有可见差异
        return idle ? juce::Colour(0xff0c1220)
            : juce::Colour(0xff070710);  // 纯背景
    }
    if (mag < 0.02f)  return juce::Colour::fromHSV(0.65f, 0.90f, mag * 18.0f, 1.0f);
    if (mag < 0.08f)  return juce::Colour::fromHSV(0.58f, 0.85f, 0.22f + mag * 3.5f, 1.0f);
    if (mag < 0.20f)  return juce::Colour::fromHSV(0.45f, 0.75f, 0.35f + mag * 1.8f, 1.0f);
    if (mag < 0.45f)  return juce::Colour::fromHSV(0.25f, 0.70f, 0.50f + mag * 0.8f, 1.0f);
    if (mag < 0.75f)  return juce::Colour::fromHSV(0.13f, 0.75f, 0.65f + mag * 0.4f, 1.0f);
    return juce::Colour::fromHSV(0.10f, 0.85f, 1.0f, 1.0f);
}

// ============================================================
// 构造 / 析构
// ============================================================
SpectrumWaterfall::SpectrumWaterfall(SpectrumAnalyzer& an, float* srPtr)
    : analyzer(an)
    , currentSampleRate(srPtr)
{
    for (auto& x : logXCoords) x = 0.0f;
    waterfallImage = juce::Image(juce::Image::RGB, 1, 1, false);
    startTimerHz(30);   // 30fps，画质和性能的最优平衡点
}

SpectrumWaterfall::~SpectrumWaterfall()
{
    stopTimer();
}

// ============================================================
// resize
// ============================================================
void SpectrumWaterfall::resized()
{
    ensureImageSize(getWidth(), getHeight());
}

// ============================================================
// ensureImageSize —— resize 时保留旧内容
// ============================================================
void SpectrumWaterfall::ensureImageSize(int w, int h)
{
    const int iw = juce::jmax(1, w);
    const int ih = juce::jmax(1, h);

    if (waterfallImage.getWidth() == iw && waterfallImage.getHeight() == ih)
        return;

    juce::Image newImg(juce::Image::RGB, iw, ih, false);
    juce::Graphics ng(newImg);
    ng.fillAll(juce::Colour(0xff070710));

    // 把旧 Image 内容 blit 到新 Image 的底部
    if (waterfallImage.isValid() &&
        waterfallImage.getWidth() > 0 &&
        waterfallImage.getHeight() > 0)
    {
        const int oldH = waterfallImage.getHeight();
        const int copyH = juce::jmin(oldH, ih);
        ng.drawImage(waterfallImage,
            0, ih - copyH, iw, copyH,           // dest
            0, juce::jmax(0, oldH - copyH),     // src x, y
            waterfallImage.getWidth(), copyH,
            false);
    }

    waterfallImage = newImg;
    currentRow = 0;

    const float sr = (currentSampleRate) ? *currentSampleRate : 44100.0f;
    rebuildXCoords(static_cast<float>(iw), sr);
}

// ============================================================
// rebuildXCoords —— 对数频率 → 像素 X 映射
// ============================================================
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

// ============================================================
// timerCallback —— 核心渲染循环
// ============================================================
void SpectrumWaterfall::timerCallback()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    ensureImageSize(getWidth(), getHeight());

    const int imgW = waterfallImage.getWidth();
    const int imgH = waterfallImage.getHeight();

    juce::Graphics ig(waterfallImage);

    // ---- 检查 X 坐标是否需要更新 ----
    const float sr = (currentSampleRate) ? *currentSampleRate : 44100.0f;
    if (std::abs(static_cast<float>(imgW) - lastWidth) > 0.5f ||
        std::abs(sr - lastSampleRate) > 1.0f)
    {
        rebuildXCoords(static_cast<float>(imgW), sr);
    }

    // ---- 判定是否有信号输入 ----
    bool hasSignal = false;
    for (int i = 0; i < displayBins; ++i)
    {
        if (analyzer.getSmoothedMag(i) > 0.002f)
        {
            hasSignal = true;
            break;
        }
    }

    // ---- 逐 bin 绘制当前行 ----
    for (int i = 0; i < displayBins - 1; ++i)
    {
        const float mag = analyzer.getSmoothedMag(i);
        const juce::Colour col = colourForMag(mag, !hasSignal);

        ig.setColour(col);

        const float x1 = logXCoords[static_cast<size_t>(i)];
        const float x2 = logXCoords[static_cast<size_t>(i + 1)];
        const float bw = juce::jmax(1.0f, x2 - x1);

        ig.fillRect(x1, static_cast<float>(currentRow), bw, 1.0f);
    }

    // ---- 环形推进：清掉下一帧将覆盖的行 ----
    currentRow = (currentRow + 1) % imgH;
    ig.setColour(juce::Colour(0xff070710));
    ig.fillRect(0.0f, static_cast<float>(currentRow),
        static_cast<float>(imgW), 1.0f);

    repaint();
}

// ============================================================
// paint —— 只做环形 blit，不画网格不画线条
// ============================================================
void SpectrumWaterfall::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff070710));

    if (!waterfallImage.isValid()) return;

    const int imgW = waterfallImage.getWidth();
    const int imgH = waterfallImage.getHeight();
    if (imgW <= 0 || imgH <= 0) return;

    // 环形缓冲区 blit
    // currentRow 是最新写入位置
    // split 把 Image 切为两段：下方是较新数据，上方是较旧数据
    const int split = (currentRow + 1) % imgH;
    const int topH = imgH - split;   // 较旧部分的高度
    const int botH = split;          // 较新部分的高度

    // 先画较旧部分（Image 中 split..imgH-1 → 屏幕顶部）
    if (topH > 0)
        g.drawImage(waterfallImage,
            0, 0, imgW, topH,           // dest
            0, split, imgW, topH,       // src
            false);

    // 再画较新部分（Image 中 0..split-1 → 屏幕底部）
    if (botH > 0)
        g.drawImage(waterfallImage,
            0, static_cast<float>(topH), imgW, botH,   // dest
            0, 0, imgW, botH,                          // src
            false);
}
