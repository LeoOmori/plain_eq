#include "DSP/Biquad.h"

#include <algorithm>
#include <cmath>

namespace PlainEq::DSP
{
void Biquad::setPeakFilter (double sampleRate, double frequencyHz, double gainDb, double q)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * pi * frequencyHz / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosW0 = std::cos (w0);

    const double rawB0 = 1.0 + alpha * A;
    const double rawB1 = -2.0 * cosW0;
    const double rawB2 = 1.0 - alpha * A;

    const double rawA0 = 1.0 + alpha / A;
    const double rawA1 = -2.0 * cosW0;
    const double rawA2 = 1.0 - alpha / A;

    b0 = rawB0 / rawA0;
    b1 = rawB1 / rawA0;
    b2 = rawB2 / rawA0;
    a1 = rawA1 / rawA0;
    a2 = rawA2 / rawA0;
}

void Biquad::setLowPassFilter (double sampleRate, double frequencyHz, double q)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const double w0 = 2.0 * pi * frequencyHz / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosW0 = std::cos (w0);

    const double rawB0 = (1.0 - cosW0) * 0.5;
    const double rawB1 = 1.0 - cosW0;
    const double rawB2 = (1.0 - cosW0) * 0.5;

    const double rawA0 = 1.0 + alpha;
    const double rawA1 = -2.0 * cosW0;
    const double rawA2 = 1.0 - alpha;

    b0 = rawB0 / rawA0;
    b1 = rawB1 / rawA0;
    b2 = rawB2 / rawA0;
    a1 = rawA1 / rawA0;
    a2 = rawA2 / rawA0;
}

void Biquad::setHighPassFilter (double sampleRate, double frequencyHz, double q)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const double w0 = 2.0 * pi * frequencyHz / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosW0 = std::cos (w0);

    const double rawB0 = (1.0 + cosW0) * 0.5;
    const double rawB1 = -(1.0 + cosW0);
    const double rawB2 = (1.0 + cosW0) * 0.5;

    const double rawA0 = 1.0 + alpha;
    const double rawA1 = -2.0 * cosW0;
    const double rawA2 = 1.0 - alpha;

    b0 = rawB0 / rawA0;
    b1 = rawB1 / rawA0;
    b2 = rawB2 / rawA0;
    a1 = rawA1 / rawA0;
    a2 = rawA2 / rawA0;
}

void Biquad::setLowShelfFilter (double sampleRate, double frequencyHz, double gainDb, double q)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * pi * frequencyHz / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosW0 = std::cos (w0);
    const double twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const double rawB0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha);
    const double rawB1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
    const double rawB2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha);

    const double rawA0 = (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
    const double rawA1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
    const double rawA2 = (A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha;

    b0 = rawB0 / rawA0;
    b1 = rawB1 / rawA0;
    b2 = rawB2 / rawA0;
    a1 = rawA1 / rawA0;
    a2 = rawA2 / rawA0;
}

void Biquad::setHighShelfFilter (double sampleRate, double frequencyHz, double gainDb, double q)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * pi * frequencyHz / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosW0 = std::cos (w0);
    const double twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const double rawB0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha);
    const double rawB1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
    const double rawB2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha);

    const double rawA0 = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
    const double rawA1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
    const double rawA2 = (A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha;

    b0 = rawB0 / rawA0;
    b1 = rawB1 / rawA0;
    b2 = rawB2 / rawA0;
    a1 = rawA1 / rawA0;
    a2 = rawA2 / rawA0;
}

float Biquad::process (float input)
{
    const double output = b0 * input + z1;

    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;

    return static_cast<float> (output);
}

void Biquad::reset()
{
    z1 = 0.0;
    z2 = 0.0;
}
} // namespace PlainEq::DSP
