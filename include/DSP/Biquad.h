#pragma once

#include <JuceHeader.h>

namespace PlainEq::DSP
{
class Biquad
{
public:
    void setPeakFilter (double sampleRate, double frequencyHz, double gainDb, double q);
    void setLowPassFilter (double sampleRate, double frequencyHz, double q);
    void setHighPassFilter (double sampleRate, double frequencyHz, double q);
    float process (float input);
    void reset();

private:
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;

    double z1 = 0.0;
    double z2 = 0.0;
};
} // namespace PlainEq::DSP
