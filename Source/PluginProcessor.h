/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "DSP/Biquad.h"
#include "Parameters/ParameterIDs.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

//==============================================================================
/**
*/
class Plain_eqAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Plain_eqAudioProcessor();
    ~Plain_eqAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static constexpr const char* frequencyParamId = PlainEq::Parameters::frequencyParamId;
    static constexpr const char* gainParamId = PlainEq::Parameters::gainParamId;
    static constexpr const char* qParamId = PlainEq::Parameters::qParamId;
    static constexpr const char* outputGainParamId = PlainEq::Parameters::outputGainParamId;
    static constexpr const char* bypassParamId = PlainEq::Parameters::bypassParamId;

    juce::AudioProcessorValueTreeState parameters;

    float getFrequencyHz() const noexcept;
    float getGainDb() const noexcept;
    float getQ() const noexcept;
    float getOutputGainDb() const noexcept;
    bool isBypassed() const noexcept;
    double getCurrentSampleRateForDisplay() const noexcept;
    int pullAnalyzerSamples (float* destination, int maxSamples) noexcept;

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float getParameterValue (const juce::String& parameterId, float fallbackValue) const noexcept;
    void updateFiltersIfNeeded();
    void pushAnalyzerSamples (const juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept;

    static constexpr size_t analyzerFifoSize = 32768;

    std::vector<PlainEq::DSP::Biquad> filters;
    std::atomic<double> currentSampleRate { 44100.0 };

    std::array<float, analyzerFifoSize> analyzerFifo {};
    std::atomic<size_t> analyzerWriteIndex { 0 };
    std::atomic<size_t> analyzerReadIndex { 0 };

    float lastFrequencyHz = -1.0f;
    float lastGainDb = 0.0f;
    float lastQ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Plain_eqAudioProcessor)
};
