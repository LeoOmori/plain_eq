#pragma once

#include <JuceHeader.h>

namespace PlainEq::UiFormatters
{
juce::String formatFrequency (double value);
juce::String formatDecibels (double value);
juce::String formatQ (double value);
} // namespace PlainEq::UiFormatters
