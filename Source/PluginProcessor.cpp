#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrogramAndLowPassAudioProcessor::SpectrogramAndLowPassAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
{
}

SpectrogramAndLowPassAudioProcessor::~SpectrogramAndLowPassAudioProcessor() = default;

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

double SpectrogramAndLowPassAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  SpectrogramAndLowPassAudioProcessor::getNumPrograms() { return 1; }
int  SpectrogramAndLowPassAudioProcessor::getCurrentProgram() { return 0; }
void SpectrogramAndLowPassAudioProcessor::setCurrentProgram(int) {}
const juce::String SpectrogramAndLowPassAudioProcessor::getProgramName(int) { return {}; }
void SpectrogramAndLowPassAudioProcessor::changeProgramName(int, const juce::String&) {}

void SpectrogramAndLowPassAudioProcessor::prepareToPlay(double sr, int maxBlock)
{
    currentSampleRate = static_cast<float>(sr);
    analyzer.prepare(sr, maxBlock);
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

void SpectrogramAndLowPassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalIn = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();
    for (auto ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() > 0)
        analyzer.process(buffer.getReadPointer(0), buffer.getNumSamples());
}

bool SpectrogramAndLowPassAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SpectrogramAndLowPassAudioProcessor::createEditor()
{
    return new SpectrogramAndLowPassAudioProcessorEditor(*this);
}

void SpectrogramAndLowPassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void SpectrogramAndLowPassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrogramAndLowPassAudioProcessor();
}
