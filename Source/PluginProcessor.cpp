#include "PluginProcessor.h"
#include "PluginEditor.h"

// ===== 辅助：创建参数布局（在初始化列表中调用） =====
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "cutoff", 1 },
        "Cutoff Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        5000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v)) + " Hz"; },
        [](const juce::String& s) { return s.getFloatValue(); }));
    return layout;
}

// ===== 构造函数：parameters 在初始化列表中构造 =====
SpectrogramAndLowPassAudioProcessor::SpectrogramAndLowPassAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
    , parameters(*this, nullptr, "Parameters", createParameterLayout())  // ← 初始化列表
    , fft(fftOrder)
    , window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    for (int i = 0; i < fftSize; ++i)
        fftMagnitudes[i].store(0.0f);

    cutoffParam = parameters.getRawParameterValue("cutoff");
}

SpectrogramAndLowPassAudioProcessor::~SpectrogramAndLowPassAudioProcessor() = default;

const juce::String SpectrogramAndLowPassAudioProcessor::getName() const { return JucePlugin_Name; }

bool SpectrogramAndLowPassAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}
bool SpectrogramAndLowPassAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}
bool SpectrogramAndLowPassAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}
double SpectrogramAndLowPassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  SpectrogramAndLowPassAudioProcessor::getNumPrograms() { return 1; }
int  SpectrogramAndLowPassAudioProcessor::getCurrentProgram() { return 0; }
void SpectrogramAndLowPassAudioProcessor::setCurrentProgram(int) {}
const juce::String SpectrogramAndLowPassAudioProcessor::getProgramName(int) { return {}; }
void SpectrogramAndLowPassAudioProcessor::changeProgramName(int, const juce::String&) {}

// ==================== 生命周期 ====================

void SpectrogramAndLowPassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lowPassFilter.reset();
    lowPassFilter.prepare(spec);
}

void SpectrogramAndLowPassAudioProcessor::releaseResources() {}

bool SpectrogramAndLowPassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

// ==================== 音频处理 ====================

void SpectrogramAndLowPassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInput = getTotalNumInputChannels();
    const auto totalOutput = getTotalNumOutputChannels();

    for (auto ch = totalInput; ch < totalOutput; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // ---- FFT 频谱分析 ----
    if (buffer.getNumChannels() > 0)
    {
        std::vector<float> fftData(static_cast<size_t>(fftSize * 2), 0.0f);
        const float* readPtr = buffer.getReadPointer(0);
        const int   numCopy = juce::jmin(buffer.getNumSamples(), fftSize);

        std::copy_n(readPtr, numCopy, fftData.begin());

        window.multiplyWithWindowingTable(fftData.data(), static_cast<size_t>(numCopy));
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        float localMax = 0.0f;
        for (int i = 0; i < fftSize; ++i)
        {
            const float mag = fftData[static_cast<size_t>(i)];
            fftMagnitudes[i].store(mag);
            if (mag > localMax) localMax = mag;
        }
        maxMagnitude.store(localMax);
    }

    // ---- 低通滤波 ----
    const float cutoff = cutoffParam->load();
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        currentSampleRate, cutoff, 0.7071f);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);
}

// ==================== Editor ====================

bool SpectrogramAndLowPassAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SpectrogramAndLowPassAudioProcessor::createEditor()
{
    return new SpectrogramAndLowPassAudioProcessorEditor(*this);
}

// ==================== 状态持久化 ====================

void SpectrogramAndLowPassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectrogramAndLowPassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

// ==================== 公开读取 ====================

float SpectrogramAndLowPassAudioProcessor::getFFTMagnitude(int index) const
{
    return (index >= 0 && index < fftSize) ? fftMagnitudes[index].load() : 0.0f;
}

int SpectrogramAndLowPassAudioProcessor::getFFTSize() const { return fftSize; }

float SpectrogramAndLowPassAudioProcessor::getMaxMagnitude() const { return maxMagnitude.load(); }

float SpectrogramAndLowPassAudioProcessor::getSampleRate() const
{
    return static_cast<float>(currentSampleRate);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrogramAndLowPassAudioProcessor();
}
