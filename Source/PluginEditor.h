/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "GroupBox.h"
#include "CustomLookAndFeel.h"

using namespace juce;

//==============================================================================
/**
*/
class HandySynthAudioProcessorEditor  : 
    public juce::AudioProcessorEditor,
    private juce::FilenameComponentListener, 
    private Slider::Listener, 
    private Button::Listener
{
public:
    HandySynthAudioProcessorEditor (HandySynthAudioProcessor&, AudioProcessorValueTreeState&);
    ~HandySynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void initUiParameters();
    void sliderValueChanged(Slider* slider);
    void buttonClicked(Button* button);
    void filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged) override;
    void setupTreeView();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    HandySynthAudioProcessor& audioProcessor;

    AudioProcessorValueTreeState& parameters;

    FilenameComponent sfChooser;
    TreeView treeView;
    GroupBox treeGroup, gainGroup, polyGroup, fxGroup;
    Slider gainSlider, polySlider;
    ToggleButton chorusBtn, reverbBtn;

    CustomLookAndFeel* laf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HandySynthAudioProcessorEditor)
};
