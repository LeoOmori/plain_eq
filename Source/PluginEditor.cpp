#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "DSP/EqMath.h"
#include "UI/KnobImage.h"
#include "UI/UiFormatters.h"
#include "UI/UiStyle.h"

namespace
{
using PlainEq::DSP::getPeakFilterMagnitudeDb;
using PlainEq::DSP::getLowPassFilterMagnitudeDb;
using PlainEq::DSP::getHighPassFilterMagnitudeDb;
using PlainEq::UiFormatters::formatDecibels;
using PlainEq::UiFormatters::formatFrequency;
using PlainEq::UiFormatters::formatQ;
using PlainEq::UiStyle::drawSketchLine;
using PlainEq::UiStyle::drawSketchRect;
using PlainEq::UiStyle::gridColour;
using PlainEq::UiStyle::inkColour;
using PlainEq::UiStyle::lightInkColour;
using PlainEq::UiStyle::paperColour;

void configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText, SketchLookAndFeel& lookAndFeel)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 24);
    slider.setLookAndFeel (&lookAndFeel);
    slider.setColour (juce::Slider::textBoxTextColourId, inkColour);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, paperColour);
    slider.setColour (juce::Slider::textBoxOutlineColourId, inkColour);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, inkColour);
    label.setFont (juce::FontOptions (14.0f));
}

enum FilterTypeMenuItemId
{
    peakFilterMenuItem = 100,
    lowPassFilterMenuItem,
    highPassFilterMenuItem
};

juce::String getFilterTypeLabel (int filterType)
{
    if (filterType == Plain_eqAudioProcessor::lowPassFilter)
        return "LPF";

    if (filterType == Plain_eqAudioProcessor::highPassFilter)
        return "HPF";

    return "Peaking";
}

void addFilterTypeMenuItems (juce::PopupMenu& menu, int currentFilterType)
{
    menu.addItem (peakFilterMenuItem, getFilterTypeLabel (Plain_eqAudioProcessor::peakFilter), true, currentFilterType == Plain_eqAudioProcessor::peakFilter);
    menu.addItem (lowPassFilterMenuItem, getFilterTypeLabel (Plain_eqAudioProcessor::lowPassFilter), true, currentFilterType == Plain_eqAudioProcessor::lowPassFilter);
    menu.addItem (highPassFilterMenuItem, getFilterTypeLabel (Plain_eqAudioProcessor::highPassFilter), true, currentFilterType == Plain_eqAudioProcessor::highPassFilter);
}

int getFilterTypeFromMenuResult (int result)
{
    if (result == lowPassFilterMenuItem)
        return Plain_eqAudioProcessor::lowPassFilter;

    if (result == highPassFilterMenuItem)
        return Plain_eqAudioProcessor::highPassFilter;

    if (result == peakFilterMenuItem)
        return Plain_eqAudioProcessor::peakFilter;

    return -1;
}

class AboutDialog final : public juce::Component
{
public:
    AboutDialog()
    {
        okButton.setButtonText ("OK");
        okButton.onClick = [this]
        {
            if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
                dialog->exitModalState (0);
        };

        addAndMakeVisible (okButton);
        setSize (400, 220);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (paperColour);
        g.setColour (inkColour);
        g.setFont (juce::FontOptions (22.0f));
        g.drawText ("About Plain EQ", getLocalBounds().removeFromTop (54), juce::Justification::centred);

        g.setFont (juce::FontOptions (16.0f));
        g.drawFittedText ("Made with love by Leon\n\nCredits to RBJ Audio-EQ-Cookbook",
                          getLocalBounds().reduced (28).withTrimmedTop (54).withTrimmedBottom (64),
                          juce::Justification::centred,
                          4);
    }

    void resized() override
    {
        okButton.setBounds (getLocalBounds().withSizeKeepingCentre (96, 32).withY (getHeight() - 52));
    }

private:
    juce::TextButton okButton;
};

void showAboutDialog (juce::Component& target)
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new AboutDialog());
    options.dialogTitle = {};
    options.dialogBackgroundColour = paperColour;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = false;

    if (auto* dialog = options.launchAsync())
        dialog->centreAroundComponent (&target, 400, 220);
}

} // namespace

//==============================================================================
void SketchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider&)
{
    static const auto knobImage = juce::ImageCache::getFromMemory (PlainEq::KnobImage::pngData,
                                                                    PlainEq::KnobImage::pngSize);

    const auto diameter = static_cast<float> (juce::jmin (width, height)) - 10.0f;
    const auto radius = diameter * 0.5f;
    const auto centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const auto centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f - 2.0f;
    const auto bounds = juce::Rectangle<float> (centreX - radius, centreY - radius, diameter, diameter);

    g.setColour (paperColour);
    g.fillEllipse (bounds);

    if (knobImage.isValid())
    {
        const auto imageBounds = bounds.expanded (4.0f).getSmallestIntegerContainer();
        g.drawImageWithin (knobImage,
                           imageBounds.getX(), imageBounds.getY(),
                           imageBounds.getWidth(), imageBounds.getHeight(),
                           juce::RectanglePlacement::centred);
    }
    else
    {
        g.setColour (inkColour);
        g.drawEllipse (bounds, 1.8f);
        g.drawEllipse (bounds.reduced (3.5f).translated (1.0f, -0.7f), 0.8f);
    }

    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto pointerLength = radius * 0.74f;
    const auto pointerStart = radius * 0.18f;
    const auto start = juce::Point<float> (centreX + std::sin (angle) * pointerStart,
                                          centreY - std::cos (angle) * pointerStart);
    const auto end = juce::Point<float> (centreX + std::sin (angle) * pointerLength,
                                        centreY - std::cos (angle) * pointerLength);

    g.setColour (inkColour);
    drawSketchLine (g, { start, end }, 2.6f);
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent (Plain_eqAudioProcessor& p)
    : audioProcessor (p)
{
    spectrumMagnitudes.fill (-90.0f);
    setMouseCursor (juce::MouseCursor::NormalCursor);
    startTimerHz (30);
}

juce::Rectangle<float> ResponseCurveComponent::getGraphBounds() const
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    auto graphBounds = bounds.reduced (38.0f, 20.0f);
    graphBounds.removeFromBottom (14.0f);

    return graphBounds;
}

juce::Point<float> ResponseCurveComponent::getNodePosition (int bandIndex) const
{
    auto graphBounds = getGraphBounds();

    const auto minFreq = std::log10 (20.0f);
    const auto maxFreq = std::log10 (20000.0f);
    const auto frequency = juce::jlimit (20.0f, 20000.0f, audioProcessor.getBandFrequencyHz (bandIndex));
    auto displayGain = juce::jlimit (-20.0f, 20.0f, audioProcessor.getBandGainDb (bandIndex));

    const auto filterType = audioProcessor.getBandFilterType (bandIndex);

    if (filterType == Plain_eqAudioProcessor::lowPassFilter)
    {
        const auto sampleRate = juce::jmax (audioProcessor.getCurrentSampleRateForDisplay(), 44100.0);
        displayGain = static_cast<float> (getLowPassFilterMagnitudeDb (sampleRate,
                                                                        static_cast<double> (frequency),
                                                                        static_cast<double> (audioProcessor.getBandQ (bandIndex)),
                                                                        static_cast<double> (frequency)));
    }
    else if (filterType == Plain_eqAudioProcessor::highPassFilter)
    {
        const auto sampleRate = juce::jmax (audioProcessor.getCurrentSampleRateForDisplay(), 44100.0);
        displayGain = static_cast<float> (getHighPassFilterMagnitudeDb (sampleRate,
                                                                        static_cast<double> (frequency),
                                                                        static_cast<double> (audioProcessor.getBandQ (bandIndex)),
                                                                        static_cast<double> (frequency)));
    }

    const auto normalisedFrequency = (std::log10 (frequency) - minFreq) / (maxFreq - minFreq);
    const auto x = graphBounds.getX() + normalisedFrequency * graphBounds.getWidth();
    const auto y = graphBounds.getY() + juce::jmap (juce::jlimit (-20.0f, 20.0f, displayGain), -20.0f, 20.0f, 1.0f, 0.0f) * graphBounds.getHeight();

    return { x, y };
}

void ResponseCurveComponent::setParameterFromValue (const juce::String& parameterId, float value)
{
    if (auto* parameter = audioProcessor.parameters.getParameter (parameterId))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

int ResponseCurveComponent::getNodeAtPosition (juce::Point<float> position) const
{
    for (int band = audioProcessor.getActiveBandCount() - 1; band >= 0; --band)
        if (position.getDistanceFrom (getNodePosition (band)) <= 18.0f)
            return band;

    return -1;
}

bool ResponseCurveComponent::isPositionInGraph (juce::Point<float> position) const
{
    return getGraphBounds().contains (position);
}

void ResponseCurveComponent::processPendingAnalyzerSamples()
{
    int samplesRead = 0;

    do
    {
        samplesRead = audioProcessor.pullAnalyzerSamples (analyzerPullBuffer.data(), static_cast<int> (analyzerPullBuffer.size()));

        for (int i = 0; i < samplesRead; ++i)
        {
            fftInput[static_cast<size_t> (fftInputIndex++)] = analyzerPullBuffer[static_cast<size_t> (i)];

            if (fftInputIndex == fftSize)
            {
                nextFFTBlockReady = true;
                fftInputIndex = 0;
            }
        }
    }
    while (samplesRead == static_cast<int> (analyzerPullBuffer.size()));

    if (nextFFTBlockReady)
        updateSpectrumFrame();
}

void ResponseCurveComponent::updateSpectrumFrame()
{
    nextFFTBlockReady = false;

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (fftInput.begin(), fftInput.end(), fftData.begin());

    fftWindow.multiplyWithWindowingTable (fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (int bin = 0; bin < spectrumSize; ++bin)
    {
        const auto magnitude = juce::jmax (fftData[static_cast<size_t> (bin)] / static_cast<float> (fftSize), 1.0e-6f);
        const auto decibels = juce::Decibels::gainToDecibels (magnitude);
        const auto smoothed = spectrumMagnitudes[static_cast<size_t> (bin)] * 0.78f + decibels * 0.22f;
        spectrumMagnitudes[static_cast<size_t> (bin)] = juce::jlimit (-90.0f, 0.0f, smoothed);
    }

    repaint();
}

void ResponseCurveComponent::selectBand (int bandIndex)
{
    selectedBandIndex = juce::jlimit (0, audioProcessor.getActiveBandCount() - 1, bandIndex);
    isNodeSelected = true;

    if (onSelectedBandChanged != nullptr)
        onSelectedBandChanged (selectedBandIndex);
}

void ResponseCurveComponent::setSelectedBandIndex (int bandIndex)
{
    selectedBandIndex = juce::jlimit (0, audioProcessor.getActiveBandCount() - 1, bandIndex);
    isNodeSelected = true;
    repaint();
}

void ResponseCurveComponent::showNodeActionsMenu (int bandIndex, const juce::MouseEvent& event)
{
    enum MenuItemId
    {
        resetToDefaultItem = 1,
        deleteItem
    };

    juce::PopupMenu menu;
    juce::PopupMenu filterTypeMenu;
    addFilterTypeMenuItems (filterTypeMenu, audioProcessor.getBandFilterType (bandIndex));
    menu.addSubMenu ("Filter type", filterTypeMenu);
    menu.addSeparator();
    menu.addItem (resetToDefaultItem, "Reset to default");
    menu.addSeparator();
    menu.addItem (deleteItem, "Delete", audioProcessor.getActiveBandCount() > 1);

    const auto targetArea = juce::Rectangle<int> (event.getScreenPosition().x,
                                                 event.getScreenPosition().y,
                                                 1,
                                                 1);

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (this)
                            .withTargetScreenArea (targetArea),
                        [safeThis = juce::Component::SafePointer<ResponseCurveComponent> (this), bandIndex] (int result)
                        {
                            if (safeThis == nullptr)
                                return;

                            const auto filterType = getFilterTypeFromMenuResult (result);
                            if (filterType >= 0)
                            {
                                safeThis->setParameterFromValue (Plain_eqAudioProcessor::getFilterTypeParamId (bandIndex), static_cast<float> (filterType));
                                safeThis->selectBand (bandIndex);
                                safeThis->repaint();
                            }
                            else if (result == resetToDefaultItem)
                                safeThis->resetBandToDefault (bandIndex);
                            else if (result == deleteItem)
                                safeThis->deleteBand (bandIndex);
                        });
}

void ResponseCurveComponent::resetBandToDefault (int bandIndex)
{
    const auto clampedBandIndex = juce::jlimit (0, audioProcessor.getActiveBandCount() - 1, bandIndex);

    setParameterFromValue (Plain_eqAudioProcessor::getFrequencyParamId (clampedBandIndex), 1000.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getGainParamId (clampedBandIndex), 0.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getQParamId (clampedBandIndex), 1.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getFilterTypeParamId (clampedBandIndex), Plain_eqAudioProcessor::peakFilter);
    selectBand (clampedBandIndex);
    repaint();
}

void ResponseCurveComponent::deleteBand (int bandIndex)
{
    const auto bandCount = audioProcessor.getActiveBandCount();

    if (bandCount <= 1)
        return;

    const auto clampedBandIndex = juce::jlimit (0, bandCount - 1, bandIndex);

    for (int band = clampedBandIndex; band < bandCount - 1; ++band)
    {
        setParameterFromValue (Plain_eqAudioProcessor::getFrequencyParamId (band), audioProcessor.getBandFrequencyHz (band + 1));
        setParameterFromValue (Plain_eqAudioProcessor::getGainParamId (band), audioProcessor.getBandGainDb (band + 1));
        setParameterFromValue (Plain_eqAudioProcessor::getQParamId (band), audioProcessor.getBandQ (band + 1));
        setParameterFromValue (Plain_eqAudioProcessor::getFilterTypeParamId (band), static_cast<float> (audioProcessor.getBandFilterType (band + 1)));
    }

    setParameterFromValue (Plain_eqAudioProcessor::getFrequencyParamId (bandCount - 1), 1000.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getGainParamId (bandCount - 1), 0.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getQParamId (bandCount - 1), 1.0f);
    setParameterFromValue (Plain_eqAudioProcessor::getFilterTypeParamId (bandCount - 1), Plain_eqAudioProcessor::peakFilter);

    audioProcessor.setActiveBandCount (bandCount - 1);
    selectBand (juce::jmin (clampedBandIndex, bandCount - 2));
    repaint();
}

void ResponseCurveComponent::updateBandParametersFromPosition (int bandIndex, juce::Point<float> position)
{
    auto graphBounds = getGraphBounds();

    const auto normalisedX = juce::jlimit (0.0f, 1.0f, (position.x - graphBounds.getX()) / graphBounds.getWidth());
    const auto normalisedY = juce::jlimit (0.0f, 1.0f, (position.y - graphBounds.getY()) / graphBounds.getHeight());

    const auto minFreq = std::log10 (20.0f);
    const auto maxFreq = std::log10 (20000.0f);
    const auto frequency = std::pow (10.0f, minFreq + normalisedX * (maxFreq - minFreq));
    const auto gain = juce::jmap (normalisedY, 1.0f, 0.0f, -20.0f, 20.0f);

    setParameterFromValue (Plain_eqAudioProcessor::getFrequencyParamId (bandIndex), frequency);

    if (audioProcessor.getBandFilterType (bandIndex) == Plain_eqAudioProcessor::peakFilter)
        setParameterFromValue (Plain_eqAudioProcessor::getGainParamId (bandIndex), gain);

    repaint();
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    g.fillAll (paperColour);

    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (inkColour);
    drawSketchRect (g, bounds, 0.0f, 1.2f);

    auto graphBounds = getGraphBounds();

    const auto mapX = [&graphBounds] (double frequency)
    {
        const auto minFreq = std::log10 (20.0);
        const auto maxFreq = std::log10 (20000.0);
        const auto normalised = (std::log10 (frequency) - minFreq) / (maxFreq - minFreq);
        return graphBounds.getX() + static_cast<float> (normalised) * graphBounds.getWidth();
    };

    const auto mapY = [&graphBounds] (double db)
    {
        const auto clampedDb = std::clamp (db, -20.0, 20.0);
        const auto normalised = juce::jmap (static_cast<float> (clampedDb), -20.0f, 20.0f, 1.0f, 0.0f);
        return graphBounds.getY() + normalised * graphBounds.getHeight();
    };

    const double gridFrequencies[] { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
    const double labelledFrequencies[] { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
    const double gridGains[] { -20.0, -10.0, 0.0, 10.0, 20.0 };

    g.setColour (gridColour);
    for (const auto frequency : gridFrequencies)
        drawSketchLine (g, { mapX (frequency), graphBounds.getY(), mapX (frequency), graphBounds.getBottom() }, 0.6f);

    for (const auto db : gridGains)
        drawSketchLine (g, { graphBounds.getX(), mapY (db), graphBounds.getRight(), mapY (db) }, db == 0.0 ? 1.4f : 0.6f);

    g.setColour (inkColour);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("dB", juce::roundToInt (bounds.getX() + 12.0f), juce::roundToInt (bounds.getY() + 8.0f), 30, 18, juce::Justification::left);
    g.drawText ("Hz", juce::roundToInt (bounds.getRight() - 28.0f), juce::roundToInt (bounds.getBottom() - 24.0f), 24, 18, juce::Justification::right);

    for (const auto db : gridGains)
    {
        const auto y = juce::roundToInt (mapY (db) - 8.0f);
        g.drawText (juce::String (db, 0), juce::roundToInt (bounds.getX() + 7.0f), y, 28, 16, juce::Justification::right);
        g.drawText (juce::String (db, 0), juce::roundToInt (bounds.getRight() - 34.0f), y, 28, 16, juce::Justification::left);
    }

    for (const auto frequency : labelledFrequencies)
    {
        auto text = frequency >= 1000.0 ? juce::String (frequency / 1000.0, frequency >= 10000.0 ? 0 : 0) + "k"
                                        : juce::String (juce::roundToInt (frequency));
        if (frequency == 20000.0)
            text = "20k";

        g.drawText (text, juce::roundToInt (mapX (frequency) - 18.0f), juce::roundToInt (graphBounds.getBottom() + 3.0f), 36, 18, juce::Justification::centred);
    }

    const auto analyzerPoints = juce::jmax (1, juce::roundToInt (graphBounds.getWidth()));
    juce::Path analyzerPath;
    analyzerPath.startNewSubPath (graphBounds.getX(), graphBounds.getBottom());

    const auto sampleRate = juce::jmax (audioProcessor.getCurrentSampleRateForDisplay(), 44100.0);
    const auto nyquist = sampleRate * 0.5;
    const auto minAnalyzerDb = -90.0f;
    const auto maxAnalyzerDb = 0.0f;

    for (int x = 0; x <= analyzerPoints; ++x)
    {
        const auto proportion = static_cast<double> (x) / static_cast<double> (analyzerPoints);
        const auto frequency = std::pow (10.0, std::log10 (20.0) + proportion * (std::log10 (20000.0) - std::log10 (20.0)));
        const auto bin = juce::jlimit (1,
                                       spectrumSize - 1,
                                       static_cast<int> (std::round (frequency / nyquist * static_cast<double> (spectrumSize - 1))));
        const auto decibels = spectrumMagnitudes[static_cast<size_t> (bin)];
        const auto normalisedLevel = juce::jmap (decibels, minAnalyzerDb, maxAnalyzerDb, 1.0f, 0.0f);
        const auto y = graphBounds.getY() + juce::jlimit (0.0f, 1.0f, normalisedLevel) * graphBounds.getHeight();
        analyzerPath.lineTo (graphBounds.getX() + static_cast<float> (x), y);
    }

    analyzerPath.lineTo (graphBounds.getRight(), graphBounds.getBottom());
    analyzerPath.closeSubPath();

    g.setColour (juce::Colour (0x2288847c));
    g.fillPath (analyzerPath);
    g.setColour (juce::Colour (0x6688847c));
    g.strokePath (analyzerPath, juce::PathStrokeType (0.8f));

    juce::Path responsePath;
    auto getResponseDb = [this, sampleRate] (double analysisFrequency)
    {
        auto decibels = 0.0;

        for (int band = 0; band < audioProcessor.getActiveBandCount(); ++band)
        {
            const auto frequencyHz = static_cast<double> (audioProcessor.getBandFrequencyHz (band));
            const auto q = static_cast<double> (audioProcessor.getBandQ (band));

            if (audioProcessor.getBandFilterType (band) == Plain_eqAudioProcessor::lowPassFilter)
                decibels += getLowPassFilterMagnitudeDb (sampleRate, frequencyHz, q, analysisFrequency);
            else if (audioProcessor.getBandFilterType (band) == Plain_eqAudioProcessor::highPassFilter)
                decibels += getHighPassFilterMagnitudeDb (sampleRate, frequencyHz, q, analysisFrequency);
            else
                decibels += getPeakFilterMagnitudeDb (sampleRate,
                                                      frequencyHz,
                                                      static_cast<double> (audioProcessor.getBandGainDb (band)),
                                                      q,
                                                      analysisFrequency);
        }

        return decibels;
    };

    responsePath.startNewSubPath (mapX (20.0), mapY (getResponseDb (20.0)));

    for (int x = 1; x <= analyzerPoints; ++x)
    {
        const auto proportion = static_cast<double> (x) / static_cast<double> (analyzerPoints);
        const auto analysisFrequency = std::pow (10.0, std::log10 (20.0) + proportion * (std::log10 (20000.0) - std::log10 (20.0)));
        responsePath.lineTo (mapX (analysisFrequency), mapY (getResponseDb (analysisFrequency)));
    }

    g.setColour (inkColour);
    g.strokePath (responsePath, juce::PathStrokeType (2.2f));

    for (int band = 0; band < audioProcessor.getActiveBandCount(); ++band)
    {
        const auto nodePosition = getNodePosition (band);
        const auto handleX = nodePosition.x;
        const auto handleY = nodePosition.y;
        g.setColour (paperColour);
        g.fillEllipse (handleX - 13.0f, handleY - 13.0f, 26.0f, 26.0f);

        if (isNodeSelected && band == selectedBandIndex)
        {
            g.setColour (lightInkColour);
            g.drawEllipse (handleX - 18.0f, handleY - 18.0f, 36.0f, 36.0f, 2.0f);
        }

        g.setColour (inkColour);
        g.drawEllipse (handleX - 13.0f, handleY - 13.0f, 26.0f, 26.0f, 1.8f);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String (band + 1), juce::roundToInt (handleX - 8.0f), juce::roundToInt (handleY - 8.0f), 16, 16, juce::Justification::centred);
    }
}

void ResponseCurveComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto nodeIndex = getNodeAtPosition (event.position);

    if (nodeIndex >= 0)
    {
        selectBand (nodeIndex);

        if (event.mods.isPopupMenu())
        {
            isDraggingNode = false;
            showNodeActionsMenu (nodeIndex, event);
            repaint();
            return;
        }

        isDraggingNode = true;
        nodeDragOffset = getNodePosition (selectedBandIndex) - event.position;

        if (auto* frequencyParameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getFrequencyParamId (selectedBandIndex)))
            frequencyParameter->beginChangeGesture();

        if (audioProcessor.getBandFilterType (selectedBandIndex) == Plain_eqAudioProcessor::peakFilter)
            if (auto* gainParameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getGainParamId (selectedBandIndex)))
                gainParameter->beginChangeGesture();

        repaint();
        return;
    }

    isNodeSelected = false;
    repaint();
}

void ResponseCurveComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (isDraggingNode)
        updateBandParametersFromPosition (selectedBandIndex, event.position + nodeDragOffset);
}

void ResponseCurveComponent::mouseUp (const juce::MouseEvent&)
{
    if (! isDraggingNode)
        return;

    isDraggingNode = false;

    if (auto* frequencyParameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getFrequencyParamId (selectedBandIndex)))
        frequencyParameter->endChangeGesture();

    if (audioProcessor.getBandFilterType (selectedBandIndex) == Plain_eqAudioProcessor::peakFilter)
        if (auto* gainParameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getGainParamId (selectedBandIndex)))
            gainParameter->endChangeGesture();
}

void ResponseCurveComponent::mouseMove (const juce::MouseEvent& event)
{
    setMouseCursor (getNodeAtPosition (event.position) >= 0
                        ? juce::MouseCursor::DraggingHandCursor
                        : juce::MouseCursor::NormalCursor);
}

void ResponseCurveComponent::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! isPositionInGraph (event.position))
        return;

    const auto existingNodeIndex = getNodeAtPosition (event.position);
    if (existingNodeIndex >= 0)
    {
        selectBand (existingNodeIndex);
        repaint();
        return;
    }

    const auto bandCount = audioProcessor.getActiveBandCount();
    if (bandCount >= Plain_eqAudioProcessor::maxBands)
        return;

    const auto newBandIndex = bandCount;
    audioProcessor.setActiveBandCount (bandCount + 1);
    selectBand (newBandIndex);

    setParameterFromValue (Plain_eqAudioProcessor::getQParamId (newBandIndex), 1.0f);
    updateBandParametersFromPosition (newBandIndex, event.position);
}

void ResponseCurveComponent::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (! isNodeSelected)
        return;

    if (auto* qParameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getQParamId (selectedBandIndex)))
    {
        const auto direction = wheel.isReversed ? -1.0f : 1.0f;
        const auto step = wheel.deltaY * direction * 0.08f;
        const auto newValue = juce::jlimit (0.0f, 1.0f, qParameter->getValue() + step);

        qParameter->beginChangeGesture();
        qParameter->setValueNotifyingHost (newValue);
        qParameter->endChangeGesture();

        repaint();
    }
}

void ResponseCurveComponent::timerCallback()
{
    processPendingAnalyzerSamples();
    repaint();
}

//==============================================================================
Plain_eqAudioProcessorEditor::Plain_eqAudioProcessorEditor (Plain_eqAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), responseCurve (p)
{
    setLookAndFeel (&sketchLookAndFeel);
    addAndMakeVisible (responseCurve);
    responseCurve.onSelectedBandChanged = [this] (int bandIndex) { setSelectedBandIndex (bandIndex); };

    configureSlider (frequencySlider, frequencyLabel, "Freq", sketchLookAndFeel);
    configureSlider (gainSlider, gainLabel, "Gain", sketchLookAndFeel);
    configureSlider (qSlider, qLabel, "Q", sketchLookAndFeel);
    configureSlider (outputSlider, outputLabel, "Output", sketchLookAndFeel);
    outputSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 66, 22);

    frequencySlider.textFromValueFunction = [] (double value) { return formatFrequency (value); };
    gainSlider.textFromValueFunction = [] (double value) { return formatDecibels (value); };
    qSlider.textFromValueFunction = [] (double value) { return formatQ (value); };
    outputSlider.textFromValueFunction = [] (double value) { return formatDecibels (value); };

    frequencySlider.setDoubleClickReturnValue (true, 1000.0);
    gainSlider.setDoubleClickReturnValue (true, 0.0);
    qSlider.setDoubleClickReturnValue (true, 1.0);
    outputSlider.setDoubleClickReturnValue (true, 0.0);

    bypassButton.setButtonText ("BYPASS");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::ToggleButton::textColourId, inkColour);
    bypassButton.setColour (juce::ToggleButton::tickColourId, inkColour);
    bypassButton.setColour (juce::ToggleButton::tickDisabledColourId, lightInkColour);

    filterTypeButton.onClick = [this]
    {
        showSelectedFilterTypeMenu();
    };
    filterTypeButton.setColour (juce::TextButton::buttonColourId, paperColour);
    filterTypeButton.setColour (juce::TextButton::textColourOnId, inkColour);
    filterTypeButton.setColour (juce::TextButton::textColourOffId, inkColour);

    aboutButton.setButtonText ("ABOUT");
    aboutButton.onClick = [this]
    {
        showAboutDialog (*this);
    };
    aboutButton.setColour (juce::TextButton::buttonColourId, paperColour);
    aboutButton.setColour (juce::TextButton::textColourOnId, inkColour);
    aboutButton.setColour (juce::TextButton::textColourOffId, inkColour);

    for (auto* slider : { &frequencySlider, &gainSlider, &qSlider, &outputSlider })
        addAndMakeVisible (slider);

    for (auto* label : { &frequencyLabel, &gainLabel, &qLabel, &outputLabel })
        addAndMakeVisible (label);

    addAndMakeVisible (bypassButton);
    addAndMakeVisible (filterTypeButton);
    addAndMakeVisible (aboutButton);

    setSelectedBandIndex (0);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, Plain_eqAudioProcessor::outputGainParamId, outputSlider);
    bypassAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, Plain_eqAudioProcessor::bypassParamId, bypassButton);

    setSize (900, 680);
    setResizable (true, true);
    setResizeLimits (880, 660, 1400, 900);
}

Plain_eqAudioProcessorEditor::~Plain_eqAudioProcessorEditor()
{
    frequencySlider.setLookAndFeel (nullptr);
    gainSlider.setLookAndFeel (nullptr);
    qSlider.setLookAndFeel (nullptr);
    outputSlider.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

//==============================================================================
void Plain_eqAudioProcessorEditor::setSelectedBandIndex (int bandIndex)
{
    frequencyAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();

    selectedBandIndex = juce::jlimit (0, audioProcessor.getActiveBandCount() - 1, bandIndex);
    responseCurve.setSelectedBandIndex (selectedBandIndex);

    frequencyAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters,
                                                              Plain_eqAudioProcessor::getFrequencyParamId (selectedBandIndex),
                                                              frequencySlider);
    gainAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters,
                                                         Plain_eqAudioProcessor::getGainParamId (selectedBandIndex),
                                                         gainSlider);
    qAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters,
                                                       Plain_eqAudioProcessor::getQParamId (selectedBandIndex),
                                                       qSlider);

    updateSelectedFilterControls();
    repaint();
}

void Plain_eqAudioProcessorEditor::showSelectedFilterTypeMenu()
{
    juce::PopupMenu menu;
    addFilterTypeMenuItems (menu, audioProcessor.getBandFilterType (selectedBandIndex));

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&filterTypeButton),
                         [safeThis = juce::Component::SafePointer<Plain_eqAudioProcessorEditor> (this)] (int result)
                         {
                             if (safeThis == nullptr)
                                 return;

                             const auto filterType = getFilterTypeFromMenuResult (result);
                             if (filterType >= 0)
                                 safeThis->setSelectedFilterType (filterType);
                         });
}

void Plain_eqAudioProcessorEditor::setSelectedFilterType (int filterType)
{
    if (auto* parameter = audioProcessor.parameters.getParameter (Plain_eqAudioProcessor::getFilterTypeParamId (selectedBandIndex)))
    {
        const auto clampedFilterType = juce::jlimit (static_cast<int> (Plain_eqAudioProcessor::peakFilter),
                                                    static_cast<int> (Plain_eqAudioProcessor::highPassFilter),
                                                    filterType);

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (clampedFilterType)));
        parameter->endChangeGesture();
    }

    updateSelectedFilterControls();
    responseCurve.repaint();
    repaint();
}

void Plain_eqAudioProcessorEditor::updateSelectedFilterControls()
{
    const auto filterType = audioProcessor.getBandFilterType (selectedBandIndex);
    const auto isPeak = filterType == Plain_eqAudioProcessor::peakFilter;

    filterTypeButton.setButtonText (getFilterTypeLabel (filterType));
    gainSlider.setEnabled (isPeak);
    gainLabel.setEnabled (isPeak);
}

//==============================================================================
void Plain_eqAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (paperColour);

    auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (inkColour);
    drawSketchRect (g, bounds, 10.0f, 2.0f);

    auto header = getLocalBounds().reduced (24, 18).removeFromTop (58);

    g.setColour (inkColour);
    g.setFont (juce::FontOptions (36.0f));
    g.drawText ("Plain EQ", header.removeFromLeft (170), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (14.5f));

    auto module = controlsCardBounds.toFloat();
    drawSketchRect (g, module, 8.0f, 1.4f);

    g.setColour (paperColour);
    g.fillEllipse (bandBadgeBounds.toFloat());
    g.setColour (inkColour);
    g.drawEllipse (bandBadgeBounds.toFloat(), 1.5f);
    g.setFont (juce::FontOptions (18.0f));
    g.drawText (juce::String (selectedBandIndex + 1), bandBadgeBounds, juce::Justification::centred);

    drawSketchRect (g, filterTypeBounds.toFloat(), 3.0f, 1.2f);

    g.setFont (juce::FontOptions (12.5f));
    g.setColour (lightInkColour);

    drawSketchRect (g, bypassButton.getBounds().toFloat(), 5.0f, 1.4f);


}

void Plain_eqAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (24, 18);
    bounds.removeFromTop (72);

    footerBounds = bounds.removeFromBottom (56);
    bounds.removeFromBottom (12);

    const auto desiredCardHeight = juce::jlimit (176, 226, getHeight() / 3);
    controlsCardBounds = bounds.removeFromBottom (desiredCardHeight);
    bounds.removeFromBottom (16);

    responseCurve.setBounds (bounds);

    auto cardContent = controlsCardBounds.reduced (22, 14);
    auto cardHeader = cardContent.removeFromTop (44);

    bandBadgeBounds = cardHeader.withWidth (34).withHeight (34).withY (cardHeader.getY() + 4);
    filterTypeBounds = juce::Rectangle<int> (bandBadgeBounds.getRight() + 20,
                                            cardHeader.getY() + 6,
                                            juce::jmin (210, juce::jmax (150, controlsCardBounds.getWidth() / 4)),
                                            30);
    filterTypeButton.setBounds (filterTypeBounds.reduced (4, 2));

    cardContent.removeFromBottom (26);
    const auto gutter = juce::jlimit (10, 28, controlsCardBounds.getWidth() / 40);
    const auto columnWidth = (cardContent.getWidth() - gutter * 2) / 3;

    auto layoutControl = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider)
    {
        area = area.reduced (6, 0);
        label.setBounds (area.removeFromTop (24));
        const auto knobSize = juce::jmin (area.getWidth(), area.getHeight());
        auto sliderArea = juce::Rectangle<int> (knobSize, area.getHeight()).withCentre (area.getCentre());
        slider.setBounds (sliderArea.reduced (2, 0));
    };

    layoutControl (cardContent.removeFromLeft (columnWidth), frequencyLabel, frequencySlider);
    cardContent.removeFromLeft (gutter);
    layoutControl (cardContent.removeFromLeft (columnWidth), gainLabel, gainSlider);
    cardContent.removeFromLeft (gutter);
    layoutControl (cardContent, qLabel, qSlider);

    auto headerControls = getLocalBounds().reduced (24, 18).removeFromTop (58);
    auto outputArea = headerControls.removeFromRight (168).reduced (2, 0);
    outputLabel.setBounds (outputArea.removeFromTop (18));
    outputSlider.setBounds (outputArea.reduced (0, 1));

    aboutButtonBounds = headerControls.removeFromRight (96).reduced (0, 4);
    aboutButton.setBounds (aboutButtonBounds);

    auto footer = footerBounds.reduced (0, 9);
    bypassButton.setBounds (footer.removeFromRight (96).reduced (0, 4));
}
