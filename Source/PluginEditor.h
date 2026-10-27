#pragma once
#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <vector>

namespace ios
{
    const juce::Colour lowCol  { 0xffff9500 };
    const juce::Colour midCol  { 0xff34c759 };
    const juce::Colour highCol { 0xff5ac8fa };
    const juce::Colour blueCol { 0xff0a84ff };

    inline juce::Font font (float size, bool bold = false)
    {
        auto o = juce::FontOptions (size);
        if (bold)
            o = o.withStyle ("Bold");
        return juce::Font (o);
    }
}

//==============================================================================
// Old-school (iOS 6 era) skeuomorphic look: glossy metal knobs, glass, LED segments.
class IosLookAndFeel : public juce::LookAndFeel_V4
{
public:
    IosLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

//==============================================================================
// Real-time spectrum + waveform on a glass screen, tinted by band
class SpectrumView : public juce::Component
{
public:
    explicit SpectrumView (TransientStereoAudioProcessor&);
    void update();
    void paint (juce::Graphics&) override;

private:
    static constexpr int fftOrder = 11;
    static constexpr int numCols = 220;
    static_assert ((1 << fftOrder) == TransientStereoAudioProcessor::scopeSize, "scope size must match FFT size");

    TransientStereoAudioProcessor& proc;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) TransientStereoAudioProcessor::scopeSize,
                                                 juce::dsp::WindowingFunction<float>::hann };

    std::array<float, TransientStereoAudioProcessor::scopeSize * 2> fftData {};
    std::array<float, TransientStereoAudioProcessor::scopeSize> wave {};
    std::vector<float> smoothed;
    int idleTicks = 0;
};

//==============================================================================
class BandMeterView : public juce::Component
{
public:
    explicit BandMeterView (TransientStereoEnhancer& e) : dsp (e) {}
    void paint (juce::Graphics&) override;

private:
    TransientStereoEnhancer& dsp;
};

class PhaseMeterView : public juce::Component
{
public:
    explicit PhaseMeterView (TransientStereoEnhancer& e) : dsp (e) {}
    void paint (juce::Graphics&) override;

private:
    TransientStereoEnhancer& dsp;
};

//==============================================================================
class TransientStereoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit TransientStereoAudioProcessorEditor (TransientStereoAudioProcessor&);
    ~TransientStereoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& id, const juce::String& name,
                    const juce::String& suffix, int decimals, juce::Colour colour);
    void setupToggle (Toggle&, const juce::String& id, const juce::String& text);
    void renderBackground();
    void timerCallback() override;

    IosLookAndFeel laf;
    TransientStereoAudioProcessor& proc;

    SpectrumView spectrum;
    BandMeterView meters;
    PhaseMeterView phase;

    std::array<Knob, 10> knobs;
    Toggle bypassToggle, safetyToggle, slopeToggle;

    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientStereoAudioProcessorEditor)
};
