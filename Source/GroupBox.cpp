#include "GroupBox.h"

void GroupBox::paint(Graphics& g)
{
	g.setColour(Colours::black);
	g.setOpacity(0.3);
	g.fillRoundedRectangle(3, 9, getWidth() - 6, getHeight() - 12, 6);

	GroupComponent::paint(g);
}
