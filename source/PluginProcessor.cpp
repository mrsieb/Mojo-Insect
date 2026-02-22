#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MojoInsectsAudioProcessor::createParameterLayout() noexcept
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f),
        0.0f));

    return layout;
}

//==============================================================================
MojoInsectsAudioProcessor::MojoInsectsAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    inferenceEngine.setOutputFifo (&resultFifo, resultBuffer.data());
}

MojoInsectsAudioProcessor::~MojoInsectsAudioProcessor() = default;

//==============================================================================
const juce::String MojoInsectsAudioProcessor::getName() const { return JucePlugin_Name; }
bool MojoInsectsAudioProcessor::acceptsMidi()  const { return false; }
bool MojoInsectsAudioProcessor::producesMidi() const { return false; }
bool MojoInsectsAudioProcessor::isMidiEffect() const { return false; }
double MojoInsectsAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  MojoInsectsAudioProcessor::getNumPrograms()                              { return 1; }
int  MojoInsectsAudioProcessor::getCurrentProgram()                           { return 0; }
void MojoInsectsAudioProcessor::setCurrentProgram (int)                       {}
const juce::String MojoInsectsAudioProcessor::getProgramName (int)            { return {}; }
void MojoInsectsAudioProcessor::changeProgramName (int, const juce::String&)  {}

//==============================================================================
void MojoInsectsAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // Pre-allocate anything needed here — never in processBlock.
}

void MojoInsectsAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MojoInsectsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}
#endif

//==============================================================================
// REAL-TIME SAFE — no heap allocation, no locks.
void MojoInsectsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto* inputGainParam  = apvts.getRawParameterValue ("inputGain");
    auto* outputGainParam = apvts.getRawParameterValue ("outputGain");

    const float inputGainLinear  = juce::Decibels::decibelsToGain (inputGainParam->load());
    const float outputGainLinear = juce::Decibels::decibelsToGain (outputGainParam->load());

    buffer.applyGain (inputGainLinear);

    // Submit audio to the inference engine (lock-free write).
    if (inferenceEngine.isReady())
    {
        const int numSamples = buffer.getNumSamples();
        inferenceEngine.submitInput (buffer.getReadPointer (0), numSamples);

        // Read any available inference results back into the buffer.
        const int available = resultFifo.getNumReady();
        if (available >= numSamples)
        {
            const auto scope = resultFifo.read (numSamples);
            // Copy result into each channel (simple passthrough for now).
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* dst = buffer.getWritePointer (ch);
                for (int i = 0; i < scope.blockSize1; ++i)
                    dst[i] = resultBuffer[(size_t)(scope.startIndex1 + i)];
                for (int i = 0; i < scope.blockSize2; ++i)
                    dst[scope.blockSize1 + i] = resultBuffer[(size_t)(scope.startIndex2 + i)];
            }
        }
    }

    buffer.applyGain (outputGainLinear);
}

//==============================================================================
bool MojoInsectsAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MojoInsectsAudioProcessor::createEditor()
{
    return new MojoInsectsAudioProcessorEditor (*this);
}

//==============================================================================
void MojoInsectsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MojoInsectsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MojoInsectsAudioProcessor();
}
