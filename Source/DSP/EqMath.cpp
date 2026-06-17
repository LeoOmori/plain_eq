#include "DSP/EqMath.h"

#include <algorithm>
#include <cmath>

namespace PlainEq::DSP
{
namespace
{
double getBiquadMagnitudeDb (double b0,
                             double b1,
                             double b2,
                             double a1,
                             double a2,
                             double sampleRate,
                             double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    const auto w = 2.0 * pi * analysisFrequencyHz / sampleRate;
    const auto numeratorReal = b0 + b1 * std::cos (w) + b2 * std::cos (2.0 * w);
    const auto numeratorImag = -b1 * std::sin (w) - b2 * std::sin (2.0 * w);
    const auto denominatorReal = 1.0 + a1 * std::cos (w) + a2 * std::cos (2.0 * w);
    const auto denominatorImag = -a1 * std::sin (w) - a2 * std::sin (2.0 * w);

    const auto numeratorMagnitude = std::sqrt (numeratorReal * numeratorReal + numeratorImag * numeratorImag);
    const auto denominatorMagnitude = std::sqrt (denominatorReal * denominatorReal + denominatorImag * denominatorImag);
    const auto magnitude = numeratorMagnitude / juce::jmax (denominatorMagnitude, 1.0e-9);

    return 20.0 * std::log10 (juce::jmax (magnitude, 1.0e-9));
}
} // namespace

double getPeakFilterMagnitudeDb (double sampleRate,
                                 double frequencyHz,
                                 double gainDb,
                                 double q,
                                 double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const auto A = std::pow (10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequencyHz / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto cosW0 = std::cos (w0);

    const auto rawB0 = 1.0 + alpha * A;
    const auto rawB1 = -2.0 * cosW0;
    const auto rawB2 = 1.0 - alpha * A;
    const auto rawA0 = 1.0 + alpha / A;
    const auto rawA1 = -2.0 * cosW0;
    const auto rawA2 = 1.0 - alpha / A;

    const auto b0 = rawB0 / rawA0;
    const auto b1 = rawB1 / rawA0;
    const auto b2 = rawB2 / rawA0;
    const auto a1 = rawA1 / rawA0;
    const auto a2 = rawA2 / rawA0;

    return getBiquadMagnitudeDb (b0, b1, b2, a1, a2, sampleRate, analysisFrequencyHz);
}

double getLowPassFilterMagnitudeDb (double sampleRate,
                                    double frequencyHz,
                                    double q,
                                    double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const auto w0 = 2.0 * pi * frequencyHz / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto cosW0 = std::cos (w0);

    const auto rawB0 = (1.0 - cosW0) * 0.5;
    const auto rawB1 = 1.0 - cosW0;
    const auto rawB2 = (1.0 - cosW0) * 0.5;
    const auto rawA0 = 1.0 + alpha;
    const auto rawA1 = -2.0 * cosW0;
    const auto rawA2 = 1.0 - alpha;

    const auto b0 = rawB0 / rawA0;
    const auto b1 = rawB1 / rawA0;
    const auto b2 = rawB2 / rawA0;
    const auto a1 = rawA1 / rawA0;
    const auto a2 = rawA2 / rawA0;

    return getBiquadMagnitudeDb (b0, b1, b2, a1, a2, sampleRate, analysisFrequencyHz);
}

double getHighPassFilterMagnitudeDb (double sampleRate,
                                     double frequencyHz,
                                     double q,
                                     double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const auto w0 = 2.0 * pi * frequencyHz / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto cosW0 = std::cos (w0);

    const auto rawB0 = (1.0 + cosW0) * 0.5;
    const auto rawB1 = -(1.0 + cosW0);
    const auto rawB2 = (1.0 + cosW0) * 0.5;
    const auto rawA0 = 1.0 + alpha;
    const auto rawA1 = -2.0 * cosW0;
    const auto rawA2 = 1.0 - alpha;

    const auto b0 = rawB0 / rawA0;
    const auto b1 = rawB1 / rawA0;
    const auto b2 = rawB2 / rawA0;
    const auto a1 = rawA1 / rawA0;
    const auto a2 = rawA2 / rawA0;

    return getBiquadMagnitudeDb (b0, b1, b2, a1, a2, sampleRate, analysisFrequencyHz);
}

double getLowShelfFilterMagnitudeDb (double sampleRate,
                                     double frequencyHz,
                                     double gainDb,
                                     double q,
                                     double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const auto A = std::pow (10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequencyHz / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto cosW0 = std::cos (w0);
    const auto twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const auto rawB0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha);
    const auto rawB1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
    const auto rawB2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha);
    const auto rawA0 = (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
    const auto rawA1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
    const auto rawA2 = (A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha;

    const auto b0 = rawB0 / rawA0;
    const auto b1 = rawB1 / rawA0;
    const auto b2 = rawB2 / rawA0;
    const auto a1 = rawA1 / rawA0;
    const auto a2 = rawA2 / rawA0;

    return getBiquadMagnitudeDb (b0, b1, b2, a1, a2, sampleRate, analysisFrequencyHz);
}

double getHighShelfFilterMagnitudeDb (double sampleRate,
                                      double frequencyHz,
                                      double gainDb,
                                      double q,
                                      double analysisFrequencyHz)
{
    constexpr double pi = 3.14159265358979323846;

    frequencyHz = std::clamp (frequencyHz, 20.0, sampleRate * 0.45);
    q = std::clamp (q, 0.1, 20.0);

    const auto A = std::pow (10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequencyHz / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto cosW0 = std::cos (w0);
    const auto twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const auto rawB0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha);
    const auto rawB1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
    const auto rawB2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha);
    const auto rawA0 = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
    const auto rawA1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
    const auto rawA2 = (A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha;

    const auto b0 = rawB0 / rawA0;
    const auto b1 = rawB1 / rawA0;
    const auto b2 = rawB2 / rawA0;
    const auto a1 = rawA1 / rawA0;
    const auto a2 = rawA2 / rawA0;

    return getBiquadMagnitudeDb (b0, b1, b2, a1, a2, sampleRate, analysisFrequencyHz);
}
} // namespace PlainEq::DSP
