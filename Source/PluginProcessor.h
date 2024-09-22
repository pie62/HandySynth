/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <fluidsynth.h>

using namespace juce;

//==============================================================================
/**
*/
class HandySynthAudioProcessor  : 
    public juce::AudioProcessor, 
    private AudioProcessorValueTreeState::Listener, 
    private ValueTree::Listener
{
public:
    //==============================================================================
    HandySynthAudioProcessor();
    ~HandySynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

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

    fluid_synth_t* getFluidSynth();
    fluid_sfont_t* getFluidSoundfont();

    bool getChorusOn();
    bool getReverbOn();
private:
    //==============================================================================

    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const String& parameterID, float newValue) override;
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void setSoundfont(const ValueTree& sfValueTree);

    AudioProcessorValueTreeState parameters;

    fluid_settings_t* settings;
    fluid_synth_t* synth;
    int sfId;

    bool chorusOn, reverbOn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HandySynthAudioProcessor)
};
