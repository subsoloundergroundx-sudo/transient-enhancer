#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using F = juce::AudioParameterFloat;
        using B = juce::AudioParameterBool;
        using R = juce::NormalisableRange<float>;

        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Crossover
        layout.add (std::make_unique<F> (juce::ParameterID { "lowMid", 1 },  "Low/Mid Freq",  R (60.f, 1200.f, 1.f, 0.4f),    250.f));
        layout.add (std::make_unique<F> (juce::ParameterID { "midHigh", 1 }, "Mid/High Freq", R (1000.f, 12000.f, 1.f, 0.4f), 3000.f));
        layout.add (std::make_unique<B> (juce::ParameterID { "slope24", 1 }, "Slope 24 dB/oct", true));

        // Detection
        layout.add (std::make_unique<F> (juce::ParameterID { "sens", 1 },   "Sensitivity", R (0.1f, 3.0f, 0.01f),        1.0f));
        layout.add (std::make_unique<F> (juce::ParameterID { "thresh", 1 }, "Threshold",   R (0.01f, 0.8f, 0.01f, 0.5f), 0.12f));
        layout.add (std::make_unique<F> (juce::ParameterID { "decay", 1 },  "Decay",       R (5.f, 150.f, 0.5f),         35.f));

        // Band widths
        layout.add (std::make_unique<F> (juce::ParameterID { "widthLow", 1 },  "Low Width",  R (0.f, 3.f, 0.01f), 1.0f));
        layout.add (std::make_unique<F> (juce::ParameterID { "widthMid", 1 },  "Mid Width",  R (0.f, 3.f, 0.01f), 1.6f));
        layout.add (std::make_unique<F> (juce::ParameterID { "widthHigh", 1 }, "High Width", R (0.f, 3.f, 0.01f), 2.0f));

        // Safety / mix
        layout.add (std::make_unique<F> (juce::ParameterID { "phaseThr", 1 },    "Phase Threshold", R (-0.5f, 0.8f, 0.01f), 0.25f));
        layout.add (std::make_unique<B> (juce::ParameterID { "phaseSafety", 1 }, "Phase Safety", true));
        layout.add (std::make_unique<F> (juce::ParameterID { "blend", 1 },       "Blend",           R (0.f, 1.f, 0.01f),    1.0f));
        layout.add (std::make_unique<B> (juce::ParameterID { "bypass", 1 },      "Bypass", false));

        return layout;
    }
}

TransientStereoAudioProcessor::TransientStereoAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool TransientStereoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TransientStereoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    enhancer.setSampleRate (sampleRate);
    enhancer.reset();
    fifo.fill (0.f);
    snapshot.fill (0.f);
    fifoIndex = 0;
    snapshotReady.store (false);
}

void TransientStereoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    enhancer.setLowMidFreq (get ("lowMid"));
    enhancer.setMidHighFreq (get ("midHigh"));
    enhancer.setCrossoverSlope (get ("slope24") > 0.5f ? 1 : 0);
    enhancer.setSensitivity (get ("sens"));
    enhancer.setThreshold (get ("thresh"));
    enhancer.setTransientDecay (get ("decay"));
    enhancer.setBandWidth (0, get ("widthLow"));
    enhancer.setBandWidth (1, get ("widthMid"));
    enhancer.setBandWidth (2, get ("widthHigh"));
    enhancer.setPhaseSafetyThreshold (get ("phaseThr"));
    enhancer.setPhaseSafetyEnabled (get ("phaseSafety") > 0.5f);
    enhancer.setBlend (get ("blend"));
    enhancer.setBypass (get ("bypass") > 0.5f);

    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    enhancer.process (L, R, n);

    // Feed the visualiser with the processed (mono) signal
    for (int i = 0; i < n; ++i)
    {
        fifo[(size_t) fifoIndex++] = 0.5f * (L[i] + R[i]);

        if (fifoIndex >= scopeSize)
        {
            fifoIndex = 0;
            if (! snapshotReady.load (std::memory_order_acquire))
            {
                snapshot = fifo;
                snapshotReady.store (true, std::memory_order_release);
            }
        }
    }
}

bool TransientStereoAudioProcessor::fetchScopeBlock (std::array<float, scopeSize>& dest)
{
    if (! snapshotReady.load (std::memory_order_acquire))
        return false;

    dest = snapshot;
    snapshotReady.store (false, std::memory_order_release);
    return true;
}

juce::AudioProcessorEditor* TransientStereoAudioProcessor::createEditor()
{
    return new TransientStereoAudioProcessorEditor (*this);
}

void TransientStereoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TransientStereoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TransientStereoAudioProcessor();
}
