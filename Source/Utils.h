#pragma once

#include <JuceHeader.h>

class LabelMeasure
{
public:
	LabelMeasure()
	{
	}

	static int getTextWidth(juce::Label* label)
	{
		return label->getFont().getStringWidth(label->getText());
	}

	static float getTextWidthFloat(juce::Label* label)
	{
		return label->getFont().getStringWidthFloat(label->getText());
	}
};