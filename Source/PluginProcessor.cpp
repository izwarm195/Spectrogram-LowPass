#include "PluginProcessor.h"
#include "PluginEditor.h"

// ===== 参数布局 =====
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

// ===== 构造函数 =====
SpectrogramAndLowPassAudioProcessor::SpectrogramAndLowPassAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , analyzer()
{
    cutoffParam = parameters.getRawParameterValue("cutoff");
}

SpectrogramAndLowPassAudioProcessor::~SpectrogramAndLowPassAudioProcessor() = default;

// ===== 基础信息 =====
const juce::String SpectrogramAndLowPassAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double SpectrogramAndLowPassAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectrogramAndLowPassAudioProcessor::getNumPrograms()
{
    return 1;
}

int SpectrogramAndLowPassAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectrogramAndLowPassAudioProcessor::setCurrentProgram(int) {}

const juce::String SpectrogramAndLowPassAudioProcessor::getProgramName(int)
{
    return {};
}

void SpectrogramAndLowPassAudioProcessor::changeProgramName(int, const juce::String&) {}

// ==================== 生命周期 ====================

void SpectrogramAndLowPassAudioProcessor::prepareToPlay(double sr, int maxBlock)
{
    currentSampleRate = static_cast<float>(sr);

    analyzer.prepare(sr, maxBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lowPassFilter.reset();
    lowPassFilter.prepare(spec);
}

void SpectrogramAndLowPassAudioProcessor::releaseResources()
{
    analyzer.reset();
}

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

    const auto totalIn = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();

    for (auto ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // ---- 1. 先低通滤波 ----
    const float cutoff = cutoffParam->load();
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        currentSampleRate, cutoff, 0.7071f);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    lowPassFilter.process(ctx);

    // ---- 2. 用滤波后的数据做频谱分析 ----
    if (buffer.getNumChannels() > 0)
        analyzer.process(buffer.getReadPointer(0), buffer.getNumSamples());
}

// ==================== Editor ====================

bool SpectrogramAndLowPassAudioProcessor::hasEditor() const
{
    return true;
}

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

// ==================== 工厂函数 ====================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrogramAndLowPassAudioProcessor();
}
