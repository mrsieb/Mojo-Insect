#include "PluginEditor.h"

//==============================================================================
MojoInsectsAudioProcessorEditor::MojoInsectsAudioProcessorEditor (MojoInsectsAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      inputGainAttachment  (p.apvts, "inputGain",  inputGainSlider),
      outputGainAttachment (p.apvts, "outputGain", outputGainSlider)
{
    inputGainLabel.setText ("Input Gain", juce::dontSendNotification);
    inputGainLabel.attachToComponent (&inputGainSlider, false);

    outputGainLabel.setText ("Output Gain", juce::dontSendNotification);
    outputGainLabel.attachToComponent (&outputGainSlider, false);

    inputGainSlider.setSliderStyle  (juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

    outputGainSlider.setSliderStyle  (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (inputGainLabel);
    addAndMakeVisible (outputGainLabel);

    setSize (400, 300);
}

MojoInsectsAudioProcessorEditor::~MojoInsectsAudioProcessorEditor() = default;

//==============================================================================
void MojoInsectsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawFittedText ("Mojo Insects", getLocalBounds().removeFromTop (50),
                      juce::Justification::centred, 1);
}

void MojoInsectsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // title

    const int knobSize = 120;
    auto row = area.removeFromTop (knobSize + 30);

    inputGainSlider.setBounds  (row.removeFromLeft (knobSize + 20).withTrimmedTop (30));
    outputGainSlider.setBounds (row.removeFromLeft (knobSize + 20).withTrimmedTop (30));
}
