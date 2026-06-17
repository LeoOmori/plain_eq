#pragma once

#include <JuceHeader.h>

namespace PlainEq::DSP
{
double getPeakFilterMagnitudeDb (double sampleRate,
                                 double frequencyHz,
                                 double gainDb,
                                 double q,
                                 double analysisFrequencyHz);

double getLowPassFilterMagnitudeDb (double sampleRate,
                                    double frequencyHz,
                                    double q,
                                    double analysisFrequencyHz);

double getHighPassFilterMagnitudeDb (double sampleRate,
                                     double frequencyHz,
                                     double q,
                                     double analysisFrequencyHz);

double getLowShelfFilterMagnitudeDb (double sampleRate,
                                     double frequencyHz,
                                     double gainDb,
                                     double q,
                                     double analysisFrequencyHz);

double getHighShelfFilterMagnitudeDb (double sampleRate,
                                      double frequencyHz,
                                      double gainDb,
                                      double q,
                                      double analysisFrequencyHz);
} // namespace PlainEq::DSP
