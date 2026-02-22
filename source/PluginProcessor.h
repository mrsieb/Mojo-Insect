#pragma once

#include <JuceHeader.h>
#include "InferenceEngine.h"

class MojoInsectsAudioProcessor : public juce::AudioProcessor
{
public:
    MojoInsectsAudioProcessor();
    ~MojoInsectsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    // Real-time safe — no heap allocation, no locks.
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() noexcept;

    InferenceEngine inferenceEngine;

    // Lock-free FIFO for audio ↔ inference results exchange.
    static constexpr int kFifoSize = 512;
    juce::AbstractFifo resultFifo { kFifoSize };
    std::array<float, kFifoSize> resultBuffer {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MojoInsectsAudioProcessor)
};
