/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>

//==============================================================================
class SketchLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
class ResponseCurveComponent final  : public juce::Component,
                                      private juce::Timer
{
public:
    explicit ResponseCurveComponent (Plain_eqAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> getGraphBounds() const;
    juce::Point<float> getNodePosition() const;
    bool isPositionOnNode (juce::Point<float>) const;
    void updateParametersFromPosition (juce::Point<float>);
    void setParameterFromValue (const juce::String& parameterId, float value);
    void processPendingAnalyzerSamples();
    void updateSpectrumFrame();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int spectrumSize = fftSize / 2;
    static constexpr int analyzerPullBufferSize = 512;

    Plain_eqAudioProcessor& audioProcessor;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize> fftInput {};
    std::array<float, fftSize * 2> fftData {};
    std::array<float, spectrumSize> spectrumMagnitudes {};
    std::array<float, analyzerPullBufferSize> analyzerPullBuffer {};
    int fftInputIndex = 0;
    bool nextFFTBlockReady = false;
    juce::Point<float> nodeDragOffset;
    bool isDraggingNode = false;
    bool isNodeSelected = false;
};

//==============================================================================
/**
*/
class Plain_eqAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Plain_eqAudioProcessorEditor (Plain_eqAudioProcessor&);
    ~Plain_eqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Plain_eqAudioProcessor& audioProcessor;

    SketchLookAndFeel sketchLookAndFeel;

    ResponseCurveComponent responseCurve;

    juce::Slider frequencySlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::Slider outputSlider;
    juce::ToggleButton bypassButton;

    juce::Label frequencyLabel;
    juce::Label gainLabel;
    juce::Label qLabel;
    juce::Label outputLabel;

    juce::Rectangle<int> controlsCardBounds;
    juce::Rectangle<int> bandBadgeBounds;
    juce::Rectangle<int> filterTypeBounds;
    juce::Rectangle<int> footerBounds;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> frequencyAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> qAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Plain_eqAudioProcessorEditor)
};
