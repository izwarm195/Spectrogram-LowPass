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

    // 1. scroll image up by 1 pixel
    ig.drawImage(waterfallImage, 0, -1, imgW, imgH,
        0, 0, imgW, imgH, false);

    // fill the exposed top row
    ig.setColour(juce::Colour(0xff070710));
    ig.fillRect(0, 0, imgW, 1);

    // 2. draw new spectrum row at bottom
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
        ig.fillRect(x1, static_cast<float>(imgH) - 1.0f, bw, 1.0f);
    }

    repaint();
}

void SpectrumWaterfall::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    g.fillAll(juce::Colour(0xff070710));

    if (waterfallImage.isValid())
        g.drawImageAt(waterfallImage, 0, 0, false);

    // frequency markers
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.setFont(juce::FontOptions(10.0f));

    const float markers[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* labels[] = { "50","100","200","500","1k","2k","5k","10k","20k" };

    static const float mA4 = 440.0f;
    static const float cMin = 1200.0f * std::log2(20.0f / mA4);
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

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("log-scaled  cent-spaced  60fps image-buffer",
        juce::Rectangle<float>(6.0f, 2.0f, w - 12.0f, 14.0f),
        juce::Justification::centredLeft);
}
