#include "TransientStereoEnhancer.h"

namespace
{
    constexpr double kPi = 3.14159265358979323846;
}

void TransientStereoEnhancer::Biquad::setLPF (double f0, double sr, double q)
{
    const double w0 = 2.0 * kPi * f0 / sr;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosw0 = std::cos (w0);
    const double a0 = 1.0 + alpha;
    b0 = ((1.0 - cosw0) / 2.0) / a0;
    b1 = (1.0 - cosw0) / a0;
    b2 = b0;
    a1 = (-2.0 * cosw0) / a0;
    a2 = (1.0 - alpha) / a0;
}

void TransientStereoEnhancer::Biquad::setHPF (double f0, double sr, double q)
{
    const double w0 = 2.0 * kPi * f0 / sr;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double cosw0 = std::cos (w0);
    const double a0 = 1.0 + alpha;
    b0 = ((1.0 + cosw0) / 2.0) / a0;
    b1 = (-(1.0 + cosw0)) / a0;
    b2 = b0;
    a1 = (-2.0 * cosw0) / a0;
    a2 = (1.0 - alpha) / a0;
}

TransientStereoEnhancer::TransientStereoEnhancer()
{
    bandWidth_[0].store (1.0f);   // low: neutral, keeps the bass tight
    bandWidth_[1].store (1.6f);   // mid: +60% transient stereo
    bandWidth_[2].store (2.0f);   // high: +100% transient stereo

    for (auto& d : bandTransientDisplay_)
        d.store (0.0f);

    updateCoefficients();
    updateFilters();
}

void TransientStereoEnhancer::setSampleRate (double sampleRate)
{
    sampleRate_ = sampleRate > 0.0 ? (float) sampleRate : 48000.0f;
    updateCoefficients();
    updateFilters();
}

void TransientStereoEnhancer::reset()
{
    for (size_t ch = 0; ch < 2; ++ch)
    {
        xoverLowMidLP_[ch].reset();
        xoverLowMidHP_[ch].reset();
        xoverMidHighLP_[ch].reset();
        xoverMidHighHP_[ch].reset();
        lowCompLP_[ch].reset();
        lowCompHP_[ch].reset();
    }
    for (auto& d : detectors_)
        d.reset();

    corrL2_ = corrR2_ = corrLR_ = 0.0f;
    bandCorrL2_.fill (0.0f);
    bandCorrR2_.fill (0.0f);
    bandCorrLR_.fill (0.0f);
}

void TransientStereoEnhancer::setLowMidFreq (float freq)
{
    const float f = std::clamp (freq, 60.0f, 1200.0f);
    if (f == lowMidFreq_.load()) return;
    lowMidFreq_.store (f);
    updateFilters();
}

void TransientStereoEnhancer::setMidHighFreq (float freq)
{
    const float f = std::clamp (freq, 1000.0f, 12000.0f);
    if (f == midHighFreq_.load()) return;
    midHighFreq_.store (f);
    updateFilters();
}

void TransientStereoEnhancer::setCrossoverSlope (int slope)
{
    const int s = slope != 0 ? 1 : 0;
    if (s == crossoverSlope_.load()) return;
    crossoverSlope_.store (s);
    updateFilters();
}

void TransientStereoEnhancer::setSensitivity (float sens)      { sensitivity_.store (std::clamp (sens, 0.1f, 3.0f)); }
void TransientStereoEnhancer::setThreshold (float thresh)      { threshold_.store (std::clamp (thresh, 0.01f, 0.8f)); }

void TransientStereoEnhancer::setTransientDecay (float ms)
{
    const float v = std::clamp (ms, 5.0f, 150.0f);
    if (v == transientDecayMs_.load()) return;
    transientDecayMs_.store (v);
    updateCoefficients();
}

void TransientStereoEnhancer::setBandWidth (int band, float width)
{
    if (band >= 0 && band < 3)
        bandWidth_[(size_t) band].store (std::clamp (width, 0.0f, 3.0f));
}

void TransientStereoEnhancer::setGlobalWidth (float width)
{
    const float w = std::clamp (width, 0.0f, 3.0f);
    for (auto& b : bandWidth_)
        b.store (w);
}

void TransientStereoEnhancer::setPhaseSafetyThreshold (float thresh) { phaseSafetyThreshold_.store (std::clamp (thresh, -0.5f, 0.8f)); }
void TransientStereoEnhancer::setPhaseSafetyEnabled (bool enabled)   { phaseSafetyEnabled_.store (enabled); }
void TransientStereoEnhancer::setBlend (float blend)                 { blend_.store (std::clamp (blend, 0.0f, 1.0f)); }
void TransientStereoEnhancer::setBypass (bool bypass)               { bypass_.store (bypass); }

void TransientStereoEnhancer::updateCoefficients()
{
    const float sr = sampleRate_ > 0.0f ? sampleRate_ : 48000.0f;

    const float attackMs = 0.35f;
    fastAttackCoeff_ = 1.0f - std::exp (-1.0f / (attackMs * 0.001f * sr));

    const float releaseMs = transientDecayMs_.load();
    fastReleaseCoeff_ = 1.0f - std::exp (-1.0f / (releaseMs * 0.001f * sr));

    const float slowMs = 65.0f;
    slowCoeff_ = 1.0f - std::exp (-1.0f / (slowMs * 0.001f * sr));
}

void TransientStereoEnhancer::updateFilters()
{
    const float sr = sampleRate_ > 0.0f ? sampleRate_ : 48000.0f;
    const float fLM = std::clamp (lowMidFreq_.load(), 40.0f, sr * 0.45f);
    const float fMH = std::clamp (midHighFreq_.load(), fLM + 50.0f, sr * 0.45f);
    const int slope = crossoverSlope_.load();

    for (size_t ch = 0; ch < 2; ++ch)
    {
        xoverLowMidLP_[ch].setLPF (fLM, sr, slope);
        xoverLowMidHP_[ch].setHPF (fLM, sr, slope);

        xoverMidHighLP_[ch].setLPF (fMH, sr, slope);
        xoverMidHighHP_[ch].setHPF (fMH, sr, slope);

        lowCompLP_[ch].setLPF (fMH, sr, slope);
        lowCompHP_[ch].setHPF (fMH, sr, slope);
    }
}

void TransientStereoEnhancer::process (float* left, float* right, int numFrames)
{
    if (numFrames <= 0)
        return;

    if (bypass_.load())
    {
        float sumL2 = 0.0f, sumR2 = 0.0f, sumLR = 0.0f;
        for (int i = 0; i < numFrames; ++i)
        {
            const float l = left[i];
            const float r = right[i];
            sumL2 += l * l;
            sumR2 += r * r;
            sumLR += l * r;
        }
        const float denom = std::sqrt (sumL2 * sumR2);
        const float corr = (denom > 1e-6f) ? (sumLR / denom) : 1.0f;
        globalCorrelation_.store (std::clamp (corr, -1.0f, 1.0f));
        for (auto& d : bandTransientDisplay_)
            d.store (0.0f);
        safetyReductionDisplay_.store (0.0f);
        return;
    }

    const float sens = sensitivity_.load();
    const float thresh = threshold_.load();
    const float widths[3] = { bandWidth_[0].load(), bandWidth_[1].load(), bandWidth_[2].load() };
    const float phaseThresh = phaseSafetyThreshold_.load();
    const bool phaseSafety = phaseSafetyEnabled_.load();
    const float blend = blend_.load();
    const int slope = crossoverSlope_.load();

    // LR2 needs the high-pass outputs inverted for the bands to sum flat; LR4 does not.
    const float hpSign = (slope == 1) ? 1.0f : -1.0f;

    float maxTransient[3] = { 0.0f, 0.0f, 0.0f };
    float maxSafetyReduction = 0.0f;

    for (int i = 0; i < numFrames; ++i)
    {
        const float inL = left[i];
        const float inR = right[i];

        // 1. 3-band split
        const float lowL0 = xoverLowMidLP_[0].process (inL, slope);
        const float lowR0 = xoverLowMidLP_[1].process (inR, slope);

        const float mhL = hpSign * xoverLowMidHP_[0].process (inL, slope);
        const float mhR = hpSign * xoverLowMidHP_[1].process (inR, slope);

        const float midL  = xoverMidHighLP_[0].process (mhL, slope);
        const float midR  = xoverMidHighLP_[1].process (mhR, slope);
        const float highL = hpSign * xoverMidHighHP_[0].process (mhL, slope);
        const float highR = hpSign * xoverMidHighHP_[1].process (mhR, slope);

        // Phase-align the low band with the mid/high branch (allpass compensation)
        const float lowL = lowCompLP_[0].process (lowL0, slope) + hpSign * lowCompHP_[0].process (lowL0, slope);
        const float lowR = lowCompLP_[1].process (lowR0, slope) + hpSign * lowCompHP_[1].process (lowR0, slope);

        const float bL[3] = { lowL, midL, highL };
        const float bR[3] = { lowR, midR, highR };

        float outBandL[3];
        float outBandR[3];

        // 2. Per band processing
        for (size_t b = 0; b < 3; ++b)
        {
            const float curL = bL[b];
            const float curR = bR[b];

            bandCorrL2_[b] = 0.992f * bandCorrL2_[b] + 0.008f * (curL * curL);
            bandCorrR2_[b] = 0.992f * bandCorrR2_[b] + 0.008f * (curR * curR);
            bandCorrLR_[b] = 0.992f * bandCorrLR_[b] + 0.008f * (curL * curR);

            const float bandDenom = std::sqrt (bandCorrL2_[b] * bandCorrR2_[b]);
            float bandCorr = (bandDenom > 1e-7f) ? (bandCorrLR_[b] / bandDenom) : 1.0f;
            bandCorr = std::clamp (bandCorr, -1.0f, 1.0f);

            float phaseSafeFactor = 1.0f;
            if (phaseSafety && bandCorr < phaseThresh)
            {
                const float range = std::max (0.01f, phaseThresh + 1.0f);
                const float reduction = std::clamp ((phaseThresh - bandCorr) / range, 0.0f, 1.0f);
                phaseSafeFactor = 1.0f - reduction;
                maxSafetyReduction = std::max (maxSafetyReduction, reduction);
            }

            const float monoX = 0.5f * (std::abs (curL) + std::abs (curR));
            const float transFactor = detectors_[b].process (monoX, fastAttackCoeff_, fastReleaseCoeff_, slowCoeff_, thresh, sens);
            maxTransient[b] = std::max (maxTransient[b], transFactor);

            const float mid = 0.5f * (curL + curR);
            const float side = 0.5f * (curL - curR);

            const float dynamicBoost = (widths[b] - 1.0f) * transFactor * phaseSafeFactor;
            const float effectiveSide = side * (1.0f + dynamicBoost);

            outBandL[b] = mid + effectiveSide;
            outBandR[b] = mid - effectiveSide;
        }

        // 3. Sum bands
        const float procL = outBandL[0] + outBandL[1] + outBandL[2];
        const float procR = outBandR[0] + outBandR[1] + outBandR[2];

        // 4. Dry / wet
        const float finalL = (1.0f - blend) * inL + blend * procL;
        const float finalR = (1.0f - blend) * inR + blend * procR;

        left[i] = finalL;
        right[i] = finalR;

        corrL2_ = 0.995f * corrL2_ + 0.005f * (finalL * finalL);
        corrR2_ = 0.995f * corrR2_ + 0.005f * (finalR * finalR);
        corrLR_ = 0.995f * corrLR_ + 0.005f * (finalL * finalR);
    }

    const float denom = std::sqrt (corrL2_ * corrR2_);
    const float corr = (denom > 1e-6f) ? (corrLR_ / denom) : 1.0f;
    globalCorrelation_.store (std::clamp (corr, -1.0f, 1.0f));

    for (size_t b = 0; b < 3; ++b)
    {
        const float prev = bandTransientDisplay_[b].load();
        bandTransientDisplay_[b].store (prev * 0.8f + maxTransient[b] * 0.2f);
    }

    const float prevRed = safetyReductionDisplay_.load();
    safetyReductionDisplay_.store (prevRed * 0.85f + maxSafetyReduction * 0.15f);
}
