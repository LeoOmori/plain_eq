#include "UI/UiStyle.h"

namespace PlainEq::UiStyle
{
const juce::Colour paperColour { 0xfff6f3ec };
const juce::Colour inkColour { 0xff242424 };
const juce::Colour lightInkColour { 0xff88847c };
const juce::Colour gridColour { 0x553a3a3a };

void drawSketchLine (juce::Graphics& g, juce::Line<float> line, float thickness)
{
    g.drawLine (line, thickness);
    g.drawLine ({ line.getStart().translated (0.7f, -0.45f), line.getEnd().translated (0.7f, -0.45f) }, thickness * 0.45f);
}

void drawSketchRect (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float thickness)
{
    g.drawRoundedRectangle (bounds, corner, thickness);
    g.drawRoundedRectangle (bounds.reduced (1.0f).translated (-0.6f, 0.4f), corner, thickness * 0.45f);
}
} // namespace PlainEq::UiStyle
