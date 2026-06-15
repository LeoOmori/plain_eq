#include "UI/UiFormatters.h"

namespace PlainEq::UiFormatters
{
juce::String formatFrequency (double value)
{
    if (value >= 1000.0)
        return juce::String (value / 1000.0, 1) + " kHz";

    return juce::String (juce::roundToInt (value)) + " Hz";
}

juce::String formatDecibels (double value)
{
    return juce::String (value > 0.0 ? "+" : "") + juce::String (value, 1) + " dB";
}

juce::String formatQ (double value)
{
    return juce::String (value, 2);
}
} // namespace PlainEq::UiFormatters
