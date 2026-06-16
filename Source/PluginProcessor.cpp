/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Plain_eqAudioProcessor::Plain_eqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                        )
     , parameters (*this, nullptr, juce::Identifier ("PlainEqParameters"), createParameterLayout())
#else
     : parameters (*this, nullptr, juce::Identifier ("PlainEqParameters"), createParameterLayout())
#endif
{
    parameters.state.setProperty (PlainEq::Parameters::activeBandCountProperty, 1, nullptr);
}

Plain_eqAudioProcessor::~Plain_eqAudioProcessor()
{
}

//==============================================================================
const juce::String Plain_eqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Plain_eqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Plain_eqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Plain_eqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Plain_eqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Plain_eqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Plain_eqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Plain_eqAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Plain_eqAudioProcessor::getProgramName (int index)
{
    return {};
}

void Plain_eqAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Plain_eqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate.store (sampleRate);
    filters.resize (static_cast<size_t> (juce::jmax (1, getTotalNumInputChannels())));
    analyzerWriteIndex.store (0, std::memory_order_release);
    analyzerReadIndex.store (0, std::memory_order_release);

    for (auto& channelFilters : filters)
        for (auto& filter : channelFilters)
            filter.reset();

    lastFrequencyHz.fill (-1.0f);
    lastGainDb.fill (1000.0f);
    lastQ.fill (-1.0f);
    lastFilterType.fill (-1);
    lastActiveBandCount = -1;
    updateFiltersIfNeeded();
}

void Plain_eqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Plain_eqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Plain_eqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    pushAnalyzerSamples (buffer, totalNumInputChannels);

    const auto outputGain = juce::Decibels::decibelsToGain (getOutputGainDb());
    const auto bypassed = isBypassed();

    if (! bypassed)
        updateFiltersIfNeeded();

    const auto numChannelsToProcess = juce::jmin (totalNumInputChannels, static_cast<int> (filters.size()));
    const auto bandCount = getActiveBandCount();

    for (int channel = 0; channel < numChannelsToProcess; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        auto& channelFilters = filters[static_cast<size_t> (channel)];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto processedSample = channelData[sample];

            if (! bypassed)
                for (int band = 0; band < bandCount; ++band)
                    processedSample = channelFilters[static_cast<size_t> (band)].process (processedSample);

            channelData[sample] = processedSample * outputGain;
        }
    }
}

int Plain_eqAudioProcessor::pullAnalyzerSamples (float* destination, int maxSamples) noexcept
{
    if (destination == nullptr || maxSamples <= 0)
        return 0;

    auto readIndex = analyzerReadIndex.load (std::memory_order_relaxed);
    const auto writeIndex = analyzerWriteIndex.load (std::memory_order_acquire);

    int samplesRead = 0;

    while (readIndex != writeIndex && samplesRead < maxSamples)
    {
        destination[samplesRead++] = analyzerFifo[readIndex];
        readIndex = (readIndex + 1) % analyzerFifoSize;
    }

    analyzerReadIndex.store (readIndex, std::memory_order_release);
    return samplesRead;
}

void Plain_eqAudioProcessor::pushAnalyzerSamples (const juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept
{
    const auto channelsToRead = juce::jmin (numInputChannels, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();

    if (channelsToRead <= 0 || numSamples <= 0)
        return;

    auto writeIndex = analyzerWriteIndex.load (std::memory_order_relaxed);
    auto readIndex = analyzerReadIndex.load (std::memory_order_acquire);
    const auto gain = 1.0f / static_cast<float> (channelsToRead);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float monoSample = 0.0f;

        for (int channel = 0; channel < channelsToRead; ++channel)
            monoSample += buffer.getReadPointer (channel)[sample];

        analyzerFifo[writeIndex] = monoSample * gain;

        const auto nextWriteIndex = (writeIndex + 1) % analyzerFifoSize;
        if (nextWriteIndex == readIndex)
        {
            readIndex = (readIndex + 1) % analyzerFifoSize;
            analyzerReadIndex.store (readIndex, std::memory_order_release);
        }

        writeIndex = nextWriteIndex;
    }

    analyzerWriteIndex.store (writeIndex, std::memory_order_release);
}

//==============================================================================
bool Plain_eqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Plain_eqAudioProcessor::createEditor()
{
    return new Plain_eqAudioProcessorEditor (*this);
}

//==============================================================================
void Plain_eqAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    parameters.state.setProperty (PlainEq::Parameters::activeBandCountProperty, getActiveBandCount(), nullptr);

    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void Plain_eqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
    {
        if (state->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*state));
    }

    setActiveBandCount (static_cast<int> (parameters.state.getProperty (PlainEq::Parameters::activeBandCountProperty, 1)));
    lastFrequencyHz.fill (-1.0f);
    lastGainDb.fill (1000.0f);
    lastQ.fill (-1.0f);
    lastFilterType.fill (-1);
    lastActiveBandCount = -1;
}

//==============================================================================
juce::String Plain_eqAudioProcessor::getFrequencyParamId (int bandIndex)
{
    return bandIndex == 0 ? juce::String (frequencyParamId)
                          : juce::String (frequencyParamId) + juce::String (bandIndex + 1);
}

juce::String Plain_eqAudioProcessor::getGainParamId (int bandIndex)
{
    return bandIndex == 0 ? juce::String (gainParamId)
                          : juce::String (gainParamId) + juce::String (bandIndex + 1);
}

juce::String Plain_eqAudioProcessor::getQParamId (int bandIndex)
{
    return bandIndex == 0 ? juce::String (qParamId)
                          : juce::String (qParamId) + juce::String (bandIndex + 1);
}

juce::String Plain_eqAudioProcessor::getFilterTypeParamId (int bandIndex)
{
    return bandIndex == 0 ? juce::String (filterTypeParamId)
                          : juce::String (filterTypeParamId) + juce::String (bandIndex + 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout Plain_eqAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;

    for (int band = 0; band < maxBands; ++band)
    {
        const auto bandNumber = juce::String (band + 1);

        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { getFrequencyParamId (band), 1 },
            "Band " + bandNumber + " Frequency",
            juce::NormalisableRange<float> { 20.0f, 20000.0f, 1.0f, 0.35f },
            1000.0f));

        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { getGainParamId (band), 1 },
            "Band " + bandNumber + " Gain",
            juce::NormalisableRange<float> { -20.0f, 20.0f, 0.1f },
            0.0f));

        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { getQParamId (band), 1 },
            "Band " + bandNumber + " Q",
            juce::NormalisableRange<float> { 0.1f, 20.0f, 0.01f, 0.35f },
            1.0f));

        layout.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { getFilterTypeParamId (band), 1 },
            "Band " + bandNumber + " Filter Type",
            juce::StringArray { "Peak", "LPF" },
            peakFilter));
    }

    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outputGainParamId, 1 },
        "Output",
        juce::NormalisableRange<float> { -20.0f, 20.0f, 0.1f },
        0.0f));

    layout.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { bypassParamId, 1 },
        "Bypass",
        false));

    return { layout.begin(), layout.end() };
}

float Plain_eqAudioProcessor::getParameterValue (const juce::String& parameterId, float fallbackValue) const noexcept
{
    if (const auto* parameter = parameters.getRawParameterValue (parameterId))
        return parameter->load();

    jassertfalse;
    return fallbackValue;
}

float Plain_eqAudioProcessor::getFrequencyHz() const noexcept
{
    return getBandFrequencyHz (0);
}

float Plain_eqAudioProcessor::getGainDb() const noexcept
{
    return getBandGainDb (0);
}

float Plain_eqAudioProcessor::getQ() const noexcept
{
    return getBandQ (0);
}

float Plain_eqAudioProcessor::getBandFrequencyHz (int bandIndex) const noexcept
{
    return getParameterValue (getFrequencyParamId (juce::jlimit (0, maxBands - 1, bandIndex)), 1000.0f);
}

float Plain_eqAudioProcessor::getBandGainDb (int bandIndex) const noexcept
{
    return getParameterValue (getGainParamId (juce::jlimit (0, maxBands - 1, bandIndex)), 0.0f);
}

float Plain_eqAudioProcessor::getBandQ (int bandIndex) const noexcept
{
    return getParameterValue (getQParamId (juce::jlimit (0, maxBands - 1, bandIndex)), 1.0f);
}

int Plain_eqAudioProcessor::getBandFilterType (int bandIndex) const noexcept
{
    return juce::roundToInt (getParameterValue (getFilterTypeParamId (juce::jlimit (0, maxBands - 1, bandIndex)), peakFilter));
}

int Plain_eqAudioProcessor::getActiveBandCount() const noexcept
{
    return juce::jlimit (1, maxBands, activeBandCount.load (std::memory_order_acquire));
}

void Plain_eqAudioProcessor::setActiveBandCount (int count)
{
    const auto clampedCount = juce::jlimit (1, maxBands, count);
    activeBandCount.store (clampedCount, std::memory_order_release);
    parameters.state.setProperty (PlainEq::Parameters::activeBandCountProperty, clampedCount, nullptr);
}

float Plain_eqAudioProcessor::getOutputGainDb() const noexcept
{
    return getParameterValue (outputGainParamId, 0.0f);
}

bool Plain_eqAudioProcessor::isBypassed() const noexcept
{
    return getParameterValue (bypassParamId, 0.0f) >= 0.5f;
}

double Plain_eqAudioProcessor::getCurrentSampleRateForDisplay() const noexcept
{
    return currentSampleRate.load();
}

void Plain_eqAudioProcessor::updateFiltersIfNeeded()
{
    const auto bandCount = getActiveBandCount();

    if (bandCount == lastActiveBandCount)
    {
        auto parametersAreUnchanged = true;

        for (int band = 0; band < bandCount; ++band)
            parametersAreUnchanged = parametersAreUnchanged
                && getBandFrequencyHz (band) == lastFrequencyHz[static_cast<size_t> (band)]
                && getBandGainDb (band) == lastGainDb[static_cast<size_t> (band)]
                && getBandQ (band) == lastQ[static_cast<size_t> (band)]
                && getBandFilterType (band) == lastFilterType[static_cast<size_t> (band)];

        if (parametersAreUnchanged)
            return;
    }

    const auto sampleRate = currentSampleRate.load();

    for (int band = 0; band < bandCount; ++band)
    {
        const auto frequencyHz = getBandFrequencyHz (band);
        const auto gainDb = getBandGainDb (band);
        const auto q = getBandQ (band);
        const auto filterType = getBandFilterType (band);

        for (auto& channelFilters : filters)
        {
            auto& filter = channelFilters[static_cast<size_t> (band)];

            if (filterType == lowPassFilter)
                filter.setLowPassFilter (sampleRate, frequencyHz, q);
            else
                filter.setPeakFilter (sampleRate, frequencyHz, gainDb, q);
        }

        lastFrequencyHz[static_cast<size_t> (band)] = frequencyHz;
        lastGainDb[static_cast<size_t> (band)] = gainDb;
        lastQ[static_cast<size_t> (band)] = q;
        lastFilterType[static_cast<size_t> (band)] = filterType;
    }

    lastActiveBandCount = bandCount;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Plain_eqAudioProcessor();
}
