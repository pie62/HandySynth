#include "PresetViewItem.h"

PresetViewItem::PresetViewItem(const String& text, bool hasSubItem)
{
	this->text = text;
	this->hasSubItem = hasSubItem;
}

bool PresetViewItem::mightContainSubItems()
{
	return hasSubItem;
}

void PresetViewItem::paintItem(Graphics& g, int width, int height)
{
    // if this item is selected, fill it with a background colour..
    if (isSelected()) {
        g.fillAll(Colours::black.withAlpha(0.2f));
    }

    // use a "colour" attribute in the xml tag for this node to set the text colour..
    g.setColour(Colours::white);

    g.setFont(height * 0.7f);

    g.drawText(
        text,
        4, 0, width - 4, height,
        Justification::centredLeft, true
    );
}

String PresetViewItem::getText()
{
	return text;
}

void PresetViewItem::setText(const String& text)
{
	this->text = text;
	repaintItem();
}
