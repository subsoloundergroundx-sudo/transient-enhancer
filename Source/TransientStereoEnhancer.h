#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

// Port of the Android TransientStereoEnhancer to plain C++ (no Android / DspModule deps).
// 3-band Linkwitz-Riley split -> per band transient detector -> transient-only side expansion
// with a per-band phase safety net.
class TransientStereoEnhancer
{
public:
    TransientStereoEnhancer();

    void setSampleRate (double sampleRate);
    void reset();
    void process (float* left, float* right, int numFrames);

    // Crossover
    void setLowMidFreq (float freq);        // 60 .. 1200 Hz
    void setMidHighFreq (float freq);       // 1000 .. 12000 Hz
    void setCrossoverSlope (int slope);     // 0 = 12 dB/oct (LR2), 1 = 24 dB/oct (LR4)

    // Transient detection
    void setSensitivity (float sens);       // 0.1 .. 3.0
    void setThreshold (float thresh);       // 0.01 .. 0.8
    void setTransientDecay (float ms);      // 5 .. 150 ms

    // Stereo width per band (0 .. 3, 1 = neutral)
    void setBandWidth (int band, float width);
    void setGlobalWidth (float width);

    // Phase safety net
    void setPhaseSafetyThreshold (float thresh);   // -0.5 .. 0.8
    void setPhaseSafetyEnabled (bool enabled);

    void setBlend (float blend);            // 0 = dry, 1 = processed
    void setBypass (bool bypass);

    // Telemetry (safe to read from the UI thread)
    float getPhaseCorrelation() const { return globalCorrelation_.load(); }
    float getBandTransient (int band) const { return (band >= 0 && band < 3) ? bandTransientDisplay_[(size_t) band].load() : 0.0f; }
    float getSafetyReduction() const { return safetyReductionDisplay_.load(); }

private:
    float sampleRate_ = 48000.0f;

    std::atomic<float> lowMidFreq_ { 250.0f };
    std::atomic<float> midHighFreq_ { 3000.0f };
    std::atomic<int>   crossoverSlope_ { 1 };

    std::atomic<float> sensitivity_ { 1.0f };
    std::atomic<float> threshold_ { 0.12f };
    std::atomic<float> transientDecayMs_ { 35.0f };

    std::array<std::atomic<float>, 3> bandWidth_;
    std::atomic<float> phaseSafetyThreshold_ { 0.25f };
    std::atomic<bool>  phaseSafetyEnabled_ { true };
    std::atomic<float> blend_ { 1.0f };
    std::atomic<bool>  bypass_ { false };

    std::atomic<float> globalCorrelation_ { 1.0f };
    std::array<std::atomic<float>, 3> bandTransientDisplay_;
    std::atomic<float> safetyReductionDisplay_ { 0.0f };

    // Double precision biquad (transposed direct form II) - stable at low crossover frequencies
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        inline float process (float in)
        {
            const double x = in;
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return (float) y;
        }

        void setLPF (double f0, double sr, double q);
        void setHPF (double f0, double sr, double q);
        void reset() { z1 = z2 = 0.0; }
    };

    // order 1 -> two cascaded Butterworth biquads (LR4, 24 dB/oct)
    // order 0 -> one biquad with Q = 0.5 (LR2, 12 dB/oct)
    struct CrossoverFilter
    {
        Biquad s1, s2;

        void setLPF (float f0, float sr, int order)
        {
            const double q = (order == 1) ? 0.70710678118 : 0.5;
            s1.setLPF (f0, sr, q);
            s2.setLPF (f0, sr, q);
        }
        void setHPF (float f0, float sr, int order)
        {
            const double q = (order == 1) ? 0.70710678118 : 0.5;
            s1.setHPF (f0, sr, q);
            s2.setHPF (f0, sr, q);
        }
        inline float process (float in, int order)
        {
            const float o1 = s1.process (in);
            return (order == 1) ? s2.process (o1) : o1;
        }
        void reset() { s1.reset(); s2.reset(); }
    };

    struct TransientDetector
    {
        float fastEnv = 0.0f;
        float slowEnv = 0.0f;
        float currentTransient = 0.0f;

        inline float process (float monoIn, float fastAttack, float fastRelease, float slowCoeff, float thresh, float sens)
        {
            const float absX = std::abs (monoIn);

            if (absX > fastEnv)
                fastEnv += fastAttack * (absX - fastEnv);
            else
                fastEnv += fastRelease * (absX - fastEnv);

            slowEnv += slowCoeff * (absX - slowEnv);

            const float diff = std::max (0.0f, fastEnv - slowEnv);
            const float excess = diff - thresh;
            if (excess > 0.0f)
            {
                const float raw = (excess / (0.04f + thresh * 0.5f)) * sens;
                currentTransient = std::clamp (raw, 0.0f, 1.0f);
            }
            else
            {
                currentTransient = 0.0f;
            }
            return currentTransient;
        }

        void reset() { fastEnv = slowEnv = currentTransient = 0.0f; }
    };

    std::array<CrossoverFilter, 2> xoverLowMidLP_, xoverLowMidHP_;
    std::array<CrossoverFilter, 2> xoverMidHighLP_, xoverMidHighHP_;
    // Allpass compensation on the low band so the 3 bands sum flat (phase-aligned)
    std::array<CrossoverFilter, 2> lowCompLP_, lowCompHP_;

    std::array<TransientDetector, 3> detectors_;

    float fastAttackCoeff_ = 0.0f;
    float fastReleaseCoeff_ = 0.0f;
    float slowCoeff_ = 0.0f;

    float corrL2_ = 0.0f, corrR2_ = 0.0f, corrLR_ = 0.0f;
    std::array<float, 3> bandCorrL2_ {}, bandCorrR2_ {}, bandCorrLR_ {};

    void updateCoefficients();
    void updateFilters();
};
