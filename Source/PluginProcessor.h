#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "TransientStereoEnhancer.h"

class TransientStereoAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int scopeSize = 2048;

    TransientStereoAudioProcessor();
    ~TransientStereoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Transient Stereo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    TransientStereoEnhancer& getEnhancer() { return enhancer; }

    // UI side: returns true and fills dest when a fresh block of output samples is available
    bool fetchScopeBlock (std::array<float, scopeSize>& dest);

    juce::AudioProcessorValueTreeState apvts;

private:
    TransientStereoEnhancer enhancer;

    std::array<float, scopeSize> fifo {};
    std::array<float, scopeSize> snapshot {};
    int fifoIndex = 0;
    std::atomic<bool> snapshotReady { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientStereoAudioProcessor)
};
