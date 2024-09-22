/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils.h"
#include "PresetViewItem.h"

//==============================================================================
HandySynthAudioProcessorEditor::HandySynthAudioProcessorEditor (HandySynthAudioProcessor& p, AudioProcessorValueTreeState& params)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
    , parameters (params)
    , sfChooser(
        "File", File(), false, false, false, 
        "*.sf2;*.SF2;*.sf3;*.SF3;*.sfz;*.SFZ", 
        String(), "Choose a Soundfont file..")
    , treeView()
    , treeGroup()
    , gainGroup()
    , polyGroup()
    , fxGroup()
    , gainSlider(
        Slider::SliderStyle::RotaryHorizontalVerticalDrag, 
        Slider::TextEntryBoxPosition::TextBoxBelow)
    , polySlider(
        Slider::SliderStyle::IncDecButtons,
        Slider::TextEntryBoxPosition::TextBoxAbove
    )
    , chorusBtn("Enable chorus")
    , reverbBtn("Enable reverb")
    , laf(nullptr)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 400);

    laf = new CustomLookAndFeel();
    juce::LookAndFeel::setDefaultLookAndFeel(laf);

    addAndMakeVisible(sfChooser);
    addAndMakeVisible(treeGroup);
    addAndMakeVisible(gainGroup);
    addAndMakeVisible(polyGroup);
    addAndMakeVisible(fxGroup);

    treeGroup.setText("Presets list");
    treeGroup.addAndMakeVisible(treeView);

    gainGroup.setText("Gain");
    gainGroup.addAndMakeVisible(gainSlider);

    polyGroup.setText("Polyphony");
    polyGroup.addAndMakeVisible(polySlider);

    fxGroup.setText("Effects");
    fxGroup.addAndMakeVisible(chorusBtn);
    fxGroup.addAndMakeVisible(reverbBtn);

    auto v = parameters.state.getOrCreateChildWithName("SOUNDFONT", nullptr);
    String path = v.getProperty("path").toString();

    sfChooser.setCurrentFile(File(path), false, juce::dontSendNotification);
    sfChooser.addListener(this);

    gainSlider.setRange(0.0, 1.0);
    gainSlider.setNumDecimalPlacesToDisplay(2);
    gainSlider.addListener(this);

    polySlider.setRange(32, 1024, 1);
    polySlider.setNumDecimalPlacesToDisplay(0);
    polySlider.addListener(this);

    chorusBtn.addListener(this);
    reverbBtn.addListener(this);

    initUiParameters();
    setupTreeView();
}

HandySynthAudioProcessorEditor::~HandySynthAudioProcessorEditor()
{
    treeView.deleteRootItem();

    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    delete laf;
}

//==============================================================================
void HandySynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void HandySynthAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    const int padding = 8;
    const int fileChooserHeight = 28;
    const int bottomHeight = 120;

    auto bounds = getLocalBounds().reduced(padding);
    auto bottomBounds = bounds.removeFromBottom(bottomHeight);

    //sfChooser.setBounds(bounds.removeFromTop(fileChooserHeight + padding).reduced(padding, 0).withTrimmedTop(padding));

    FlexBox rootFlex;
    rootFlex.flexDirection = FlexBox::Direction::column;

    //FlexItem item(sfChooser);
    //item.set
    rootFlex.items.add(FlexItem(sfChooser).withHeight(fileChooserHeight));

    FlexItem::Margin margin;
    margin.top = 2;
    margin.bottom = 2;

    rootFlex.items.add(FlexItem(treeGroup).withFlex(1.0).withMargin(margin));

    rootFlex.performLayout(bounds);

    bounds = treeGroup.getLocalBounds().reduced(padding);
    bounds.removeFromTop(10);
    treeView.setBounds(bounds);


    // Bottom ---------------------------------------

    FlexBox bottomFlex;
    bottomFlex.flexDirection = FlexBox::Direction::row;
    bottomFlex.alignItems = FlexBox::AlignItems::stretch;

    bottomFlex.items.add(FlexItem(gainGroup).withFlex(1));
    bottomFlex.items.add(FlexItem(polyGroup).withFlex(1));
    bottomFlex.items.add(FlexItem(fxGroup).withFlex(1));

    bottomFlex.performLayout(bottomBounds);

    gainSlider.setBounds(gainGroup.getLocalBounds().reduced(16));
    polySlider.setBounds(polyGroup.getLocalBounds().reduced(32));

    bounds = fxGroup.getLocalBounds();
    bounds = bounds.withTrimmedLeft(16).withHeight(40);
    bounds.setY(20);
    chorusBtn.setBounds(bounds);
    bounds.setY(50);
    reverbBtn.setBounds(bounds);
}

void HandySynthAudioProcessorEditor::initUiParameters()
{
    auto synth = audioProcessor.getFluidSynth();

    gainSlider.setValue(fluid_synth_get_gain(synth), juce::dontSendNotification);
    polySlider.setValue(fluid_synth_get_polyphony(synth), juce::dontSendNotification);
    chorusBtn.setToggleState(audioProcessor.getChorusOn(), juce::dontSendNotification);
    reverbBtn.setToggleState(audioProcessor.getReverbOn(), juce::dontSendNotification);

}

void HandySynthAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
    if (slider == &gainSlider) {
        auto param = parameters.getParameter("gain");
        param->setValueNotifyingHost(slider->getValue());
    }

    if (slider == &polySlider) {

        auto polyphony = (AudioParameterInt*)(parameters.getParameter("polyphony"));
        *polyphony = (float)slider->getValue();
    }
}

void HandySynthAudioProcessorEditor::buttonClicked(Button* button)
{
    if (button == &chorusBtn) {
        auto param = parameters.getParameter("chorus");
        param->setValueNotifyingHost(button->getToggleState());
    }

    if (button == &reverbBtn) {
        auto param = parameters.getParameter("reverb");
        param->setValueNotifyingHost(button->getToggleState());
    }
}

void HandySynthAudioProcessorEditor::filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged)
{
    auto path = fileComponentThatHasChanged->getCurrentFile().getFullPathName();
    auto v = parameters.state.getOrCreateChildWithName("SOUNDFONT", nullptr);
    v.setProperty("path", path, nullptr);

    setupTreeView();
}

void HandySynthAudioProcessorEditor::setupTreeView()
{
    fluid_sfont_t* sfont = audioProcessor.getFluidSoundfont();

    treeView.deleteRootItem();

    if (sfont == nullptr)
        return;


    auto* rootItem = new PresetViewItem("", true);
    rootItem->setOpen(true);
    treeView.setRootItem(rootItem);
    treeView.setRootItemVisible(false);

    HashMap<int, Array<fluid_preset_t*>> map;

    fluid_sfont_iteration_start(sfont);
    while (auto preset = fluid_sfont_iteration_next(sfont))
    {
        int bank = fluid_preset_get_banknum(preset);
        auto list = map.getReference(bank);
        list.add(preset);
        map.set(bank, list);
    }

    for (HashMap<int, Array<fluid_preset_t*>>::Iterator bank(map); bank.next();)
    {
        String bankText = "Bank # " + String(bank.getKey());
        PresetViewItem* bankItem = new PresetViewItem(bankText, true);
        bankItem->setOpen(true);
        rootItem->addSubItem(bankItem);

        Array<fluid_preset_t*> bankList = bank.getValue();

        for (int i=0; i<bankList.size(); i++)
        {
            int num = fluid_preset_get_num(bankList[i]);
            String name = String(CharPointer_ASCII(fluid_preset_get_name(bankList[i])));
            String text = String(num) + "   " + name;
            bankItem->addSubItem(new PresetViewItem(text, false));
        }
    }
}
