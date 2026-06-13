#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==================== SpectrumWaterfall ====================

SpectrogramAndLowPassAudioProcessorEditor::SpectrumWaterfall::SpectrumWaterfall(
    SpectrogramAndLowPassAudioProcessor& p)
    : proc(p)
{
    for (auto& frame : spectrumHistory)
        std::fill(std::begin(frame), std::end(frame), 0.0f);

    startTimerHz(60);
}

void SpectrogramAndLowPassAudioProcessorEditor::SpectrumWaterfall::timerCallback()
{
    const int fftN = proc.getFFTSize();

    for (int i = 0; i < displayBins; ++i)
        spectrumHistory[writeIdx][i] = (i < fftN) ? proc.getFFTMagnitude(i) : 0.0f;

    writeIdx = (writeIdx + 1) % historyFrames;
    if (validFrames < historyFrames) ++validFrames;

    repaint();
}

float SpectrogramAndLowPassAudioProcessorEditor::SpectrumWaterfall::freqToNormX(
    float bin, float fftN, float sr)
{
    static const float minFreq = 20.0f;
    static const float maxFreq = 20000.0f;
    static const float A4 = 440.0f;

    float freq = bin * sr / fftN;
    freq = juce::jlimit(minFreq, maxFreq, freq);

    // 用 const 代替 constexpr，因为 MSVC 的 std::log2 不是 constexpr
    static const float centMin = 1200.0f * std::log2(minFreq / A4);
    static const float centMax = 1200.0f * std::log2(maxFreq / A4);

    float cent = 1200.0f * std::log2(freq / A4);
    float t = (cent - centMin) / (centMax - centMin);
    return juce::jlimit(0.0f, 1.0f, t);
}

void SpectrogramAndLowPassAudioProcessorEditor::SpectrumWaterfall::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    g.fillAll(juce::Colour(0xff080812));

    const int   fftN = proc.getFFTSize();
    const float sr = proc.getSampleRate();
    float maxMag = proc.getMaxMagnitude();
    if (maxMag < 1e-4f) maxMag = 1.0f;

    std::array<float, displayBins> xPos{};
    for (int i = 0; i < displayBins; ++i)
        xPos[static_cast<size_t>(i)] =
        freqToNormX(static_cast<float>(i), static_cast<float>(fftN), sr) * w;

    const float rowH = h / static_cast<float>(historyFrames);

    for (int f = 0; f < validFrames; ++f)
    {
        const int   rowIdx = (writeIdx - validFrames + f + historyFrames) % historyFrames;
        const float yTop = static_cast<float>(f) * rowH;
        const float yBottom = yTop + rowH + 1.0f;

        for (int i = 0; i < displayBins - 1; ++i)
        {
            const float mag = juce::jlimit(0.0f, 1.0f,
                spectrumHistory[rowIdx][i] / maxMag);

            juce::Colour col;
            if (mag < 0.12f)
                col = juce::Colour(0xff060618);
            else if (mag < 0.35f)
                col = juce::Colour::fromHSV(0.60f, 0.85f, mag * 1.6f, 1.0f);
            else if (mag < 0.60f)
                col = juce::Colour::fromHSV(0.45f, 0.75f, mag * 1.4f, 1.0f);
            else if (mag < 0.85f)
                col = juce::Colour::fromHSV(0.25f, 0.65f, mag * 1.2f, 1.0f);
            else
                col = juce::Colour::fromHSV(0.14f, 0.95f, 1.0f, 1.0f);

            g.setColour(col);
            const float barW = juce::jmax(1.0f, xPos[static_cast<size_t>(i + 1)]
                - xPos[static_cast<size_t>(i)]);
            g.fillRect(xPos[static_cast<size_t>(i)], yTop, barW, yBottom - yTop);
        }
    }

    // ---- 频率轴 ----
    g.setColour(juce::Colours::grey.withAlpha(0.45f));
    g.setFont(juce::FontOptions(10.0f));

    const float markerFreqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const char* const markerLabels[] = {
        "50Hz", "100Hz", "200Hz", "500Hz", "1k", "2k", "5k", "10k", "20k" };

    for (int m = 0; m < 9; ++m)
    {
        const float nx = freqToNormX(markerFreqs[m], 1.0f, 1.0f);
        const float x = nx * w;
        g.drawLine(x, h - 16.0f, x, h - 8.0f);
        g.drawText(juce::String(markerLabels[m]),
            x - 20.0f, h - 20.0f, 40.0f, 12.0f,
            juce::Justification::centred);
    }

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Spectrum  log scale  cent-spaced",
        bounds.reduced(8.0f, 2.0f).removeFromTop(16.0f),
        juce::Justification::centredLeft);
}

// ==================== Editor ====================

SpectrogramAndLowPassAudioProcessorEditor::SpectrogramAndLowPassAudioProcessorEditor(
    SpectrogramAndLowPassAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , waterfall(p)
{
    titleLabel.setText("Spectrogram & Low Pass", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(waterfall);

    cutoffSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    cutoffSlider.setRange(20.0, 20000.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    cutoffSlider.setTextValueSuffix(" Hz");
    cutoffSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00bcd4));
    cutoffSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1a30));
    cutoffSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(cutoffSlider);

    cutoffLabel.setText("Low-Pass Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    cutoffLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(cutoffLabel);

    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getState(), "cutoff", cutoffSlider);

    setSize(740, 520);
}

SpectrogramAndLowPassAudioProcessorEditor::~SpectrogramAndLowPassAudioProcessorEditor() = default;

void SpectrogramAndLowPassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0e0e1a));
}

void SpectrogramAndLowPassAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    titleLabel.setBounds(area.removeFromTop(28));

    auto controlsArea = area.removeFromBottom(105);

    auto knobRect = controlsArea.removeFromLeft(130);
    cutoffLabel.setBounds(knobRect.removeFromBottom(20));
    cutoffSlider.setBounds(knobRect);

    waterfall.setBounds(area.reduced(0, 6));
}
