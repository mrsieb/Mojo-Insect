#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MojoInsectsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MojoInsectsAudioProcessorEditor (MojoInsectsAudioProcessor&);
    ~MojoInsectsAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MojoInsectsAudioProcessor& processorRef;

    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Label  inputGainLabel;
    juce::Label  outputGainLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment inputGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MojoInsectsAudioProcessorEditor)
};
