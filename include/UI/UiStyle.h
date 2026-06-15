#pragma once

#include <JuceHeader.h>

namespace PlainEq::UiStyle
{
extern const juce::Colour paperColour;
extern const juce::Colour inkColour;
extern const juce::Colour lightInkColour;
extern const juce::Colour gridColour;

void drawSketchLine (juce::Graphics& g, juce::Line<float> line, float thickness = 1.4f);

void drawSketchRect (juce::Graphics& g, juce::Rectangle<float> bounds, float corner = 5.0f, float thickness = 1.4f);
} // namespace PlainEq::UiStyle
