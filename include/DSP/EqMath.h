#pragma once

#include <JuceHeader.h>

namespace PlainEq::DSP
{
double getPeakFilterMagnitudeDb (double sampleRate,
                                 double frequencyHz,
                                 double gainDb,
                                 double q,
                                 double analysisFrequencyHz);
} // namespace PlainEq::DSP
