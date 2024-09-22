/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HandySynthAudioProcessor::HandySynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    
#endif
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
    , settings(nullptr)
    , synth(nullptr)
    , sfId(-1)
    , chorusOn(false)
    , reverbOn(false)
{
    settings = new_fluid_settings();
    synth = new_fluid_synth(settings);

#if JUCE_DEBUG
    fluid_settings_setint(settings, "synth.verbose", 1);
#endif

    fluid_synth_set_gain(synth, 0.6f);
    fluid_synth_set_polyphony(synth, 128);

    fluid_settings_setint(settings, "synth.chorus.active", 0);
    fluid_settings_setint(settings, "synth.reverb.active", 0);

    fluid_synth_set_interp_method(synth, -1, FLUID_INTERP_HIGHEST);

    parameters.state.addListener(this);

    parameters.addParameterListener("gain", this);
    parameters.addParameterListener("polyphony", this);
    parameters.addParameterListener("chorus", this);
    parameters.addParameterListener("reverb", this);
}

HandySynthAudioProcessor::~HandySynthAudioProcessor()
{
    fluid_synth_sfunload(synth, sfId, 1);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
}

//==============================================================================
const juce::String HandySynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HandySynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HandySynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HandySynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HandySynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HandySynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HandySynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HandySynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HandySynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void HandySynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HandySynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    fluid_synth_set_sample_rate(synth, sampleRate);
}

void HandySynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HandySynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HandySynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }

    int time;
    juce::MidiMessage m;

    for (juce::MidiBuffer::Iterator i{ midiMessages }; i.getNextEvent(m, time);)
    {
        juce::uint8 status = m.getRawData()[0];
        int channel = m.getChannel() - 1;

        switch (status & 0xF0)
        {
        case 0x80:
            fluid_synth_noteoff(synth, channel, m.getNoteNumber());
            break;
        case 0x90:
            fluid_synth_noteon(synth, channel, m.getNoteNumber(), m.getVelocity());
            break;
        case 0xA0:
            fluid_synth_key_pressure(synth, channel, m.getNoteNumber(), m.getAfterTouchValue());
            break;
        case 0xB0:
            fluid_synth_cc(synth, channel, m.getControllerNumber(), m.getControllerValue());
            break;
        case 0xC0:
            fluid_synth_program_change(synth, channel, m.getProgramChangeNumber());
            break;
        case 0xD0:
            fluid_synth_channel_pressure(synth, channel, m.getChannelPressureValue());
            break;
        case 0xE0:
            fluid_synth_pitch_bend(synth, channel, m.getPitchWheelValue());
            break;
        case 0xF0:
            switch (status)
            {
            case 0xF7: // SysEx
                break;
            case 0xFF: // Meta
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

   fluid_synth_process(
        synth,
        buffer.getNumSamples(),
        0,
        nullptr,
        buffer.getNumChannels(),
        const_cast<float**>(buffer.getArrayOfWritePointers()) 
    );
}

//==============================================================================
bool HandySynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* HandySynthAudioProcessor::createEditor()
{
    return new HandySynthAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void HandySynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HandySynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

    setSoundfont(parameters.state.getChildWithName("SOUNDFONT"));

    auto gain = (AudioParameterFloat*)parameters.getParameter("gain");
    auto polyphony = (AudioParameterInt*)(parameters.getParameter("polyphony"));
    auto chorus = (AudioParameterBool*)parameters.getParameter("chorus");
    auto reverb = (AudioParameterBool*)parameters.getParameter("reverb");

    fluid_synth_set_gain(synth, gain->get());
    fluid_synth_set_polyphony(synth, polyphony->get());
    fluid_settings_setint(settings, "synth.chorus.active", chorus->get());
    fluid_settings_setint(settings, "synth.reverb.active", reverb->get());
}

fluid_synth_t* HandySynthAudioProcessor::getFluidSynth()
{
    return synth;
}

fluid_sfont_t* HandySynthAudioProcessor::getFluidSoundfont()
{
    if (sfId == -1)
        return nullptr;
    else
        return fluid_synth_get_sfont_by_id(synth, sfId);
}

bool HandySynthAudioProcessor::getChorusOn()
{
    return chorusOn;
}

bool HandySynthAudioProcessor::getReverbOn()
{
    return reverbOn;
}

AudioProcessorValueTreeState::ParameterLayout HandySynthAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout params;

    params.add(std::make_unique<AudioParameterFloat>("gain", "Gain", 0.0, 1.0, 0.6));
    params.add(std::make_unique<AudioParameterInt>("polyphony", "Polyphony", 32, 1024, 128));
    params.add(std::make_unique<AudioParameterBool>("chorus", "Chorus", false));
    params.add(std::make_unique<AudioParameterBool>("reverb", "Reverb", false));

    return params;
}

void HandySynthAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == "gain") 
    {
        fluid_synth_set_gain(synth, newValue);
    }
    else if (parameterID == "polyphony")
    {
        fluid_synth_set_polyphony(synth, static_cast<int>(newValue));
    }
    else if (parameterID == "chorus")
    {
        auto value = (bool)newValue;
        chorusOn = value;
        fluid_settings_setint(settings, "synth.chorus.active", value);
        
    }
    else if (parameterID == "reverb")
    {
        auto value = (bool)newValue;
        reverbOn = value;
        fluid_settings_setint(settings, "synth.reverb.active", value);
    }
    else 
    {}
}

void HandySynthAudioProcessor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (treeWhosePropertyHasChanged.getType() == StringRef("SOUNDFONT"))
        if (property == StringRef("path"))
            setSoundfont(treeWhosePropertyHasChanged);
}

void HandySynthAudioProcessor::setSoundfont(const ValueTree& sfValueTree)
{
    if (!sfValueTree.isValid())
        return;

    String path = sfValueTree.getProperty("path").toString();

    if (path.isEmpty())
        return;

    fluid_synth_sfunload(synth, sfId, 1);
    sfId = fluid_synth_sfload(synth, path.toRawUTF8(), 1);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HandySynthAudioProcessor();
}
