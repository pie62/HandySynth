#pragma once
#include <JuceHeader.h>

using namespace juce;

class PresetViewItem : public TreeViewItem
{
public:
	PresetViewItem(const String& text, bool hasSubItem);

	bool mightContainSubItems() override;
	void paintItem(Graphics& g, int width, int height) override;

	String getText();
	void setText(const String& text);

private:
	String text;
	bool hasSubItem;
};

