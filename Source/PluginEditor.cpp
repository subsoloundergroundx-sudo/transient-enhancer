#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Look and feel
//==============================================================================
IosLookAndFeel::IosLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe9edf2));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xffe9edf2));
}

void IosLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float pos, float startAngle, float endAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.f);
    const float d = std::min (bounds.getWidth(), bounds.getHeight());
    auto area = juce::Rectangle<float> (d, d).withCentre (bounds.getCentre());
    const auto centre = area.getCentre();
    const float radius = d * 0.5f;
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const float angle = startAngle + pos * (endAngle - startAngle);

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (area.translated (0.f, 2.f).expanded (1.f));

    // groove + value arc
    const float arcR = radius - 2.f;
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff07090d));
    g.strokePath (track, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, angle, true);
    g.setColour (accent.withAlpha (0.28f));
    g.strokePath (val, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // metal body
    const float bodyR = radius - 8.f;
    auto body = juce::Rectangle<float> (bodyR * 2.f, bodyR * 2.f).withCentre (centre);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f6f9), centre.x, body.getY(),
                                             juce::Colour (0xff3d424a), centre.x, body.getBottom(), false));
    g.fillEllipse (body.expanded (1.5f));

    auto face = body.reduced (2.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe1e5ea), face.getX(), face.getY(),
                                             juce::Colour (0xff5d636d), face.getRight(), face.getBottom(), false));
    g.fillEllipse (face);

    // brushed rings
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (float rr = face.getWidth() * 0.5f - 2.f; rr > 4.f; rr -= 3.f)
        g.drawEllipse (centre.x - rr, centre.y - rr, rr * 2.f, rr * 2.f, 0.6f);

    // gloss
    juce::Path gloss;
    gloss.addEllipse (face.getX() + face.getWidth() * 0.12f, face.getY() + face.getHeight() * 0.04f,
                      face.getWidth() * 0.76f, face.getHeight() * 0.46f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.6f), 0.f, face.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0.f, face.getY() + face.getHeight() * 0.5f, false));
    g.fillPath (gloss);

    // pointer
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.6f, -bodyR + 5.f, 3.2f, bodyR * 0.45f, 1.5f);
    g.setColour (juce::Colour (0xff12151a));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void IosLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float h = 22.f, w = 42.f;
    auto sw = juce::Rectangle<float> (r.getX() + 2.f, r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();

    // well
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff04060a), 0.f, sw.getY(),
                                             juce::Colour (0xff2c313b), 0.f, sw.getBottom(), false));
    g.fillRoundedRectangle (sw.expanded (1.f), h * 0.5f + 1.f);

    // pill
    if (on)
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff74e48f), 0.f, sw.getY(),
                                                 juce::Colour (0xff22a042), 0.f, sw.getBottom(), false));
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6d737e), 0.f, sw.getY(),
                                                 juce::Colour (0xff383c45), 0.f, sw.getBottom(), false));
    g.fillRoundedRectangle (sw, h * 0.5f);

    // gloss
    auto glossR = sw.reduced (2.f, 1.5f).withHeight (h * 0.42f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.32f), 0.f, glossR.getY(),
                                             juce::Colours::white.withAlpha (0.02f), 0.f, glossR.getBottom(), false));
    g.fillRoundedRectangle (glossR, 6.f);

    // thumb
    const float ts = h - 4.f;
    const float tx = on ? sw.getRight() - ts - 2.f : sw.getX() + 2.f;
    auto thumb = juce::Rectangle<float> (tx, sw.getY() + 2.f, ts, ts);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (thumb.translated (0.f, 1.2f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0.f, thumb.getY(),
                                             juce::Colour (0xffb4bac4), 0.f, thumb.getBottom(), false));
    g.fillEllipse (thumb);

    // text
    g.setColour (juce::Colours::white.withAlpha (on ? 0.95f : 0.6f));
    g.setFont (ios::font (12.f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (w + 10.f), juce::Justification::centredLeft);
}

//==============================================================================
// Spectrum view
//==============================================================================
SpectrumView::SpectrumView (TransientStereoAudioProcessor& p) : proc (p)
{
    smoothed.assign ((size_t) numCols, 0.f);
}

void SpectrumView::update()
{
    constexpr int N = TransientStereoAudioProcessor::scopeSize;

    if (proc.fetchScopeBlock (wave))
    {
        idleTicks = 0;

        std::copy (wave.begin(), wave.end(), fftData.begin());
        std::fill (fftData.begin() + N, fftData.end(), 0.f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) N);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float sr = (float) proc.getSampleRate();
        if (sr > 0.f)
        {
            for (int i = 0; i < numCols; ++i)
            {
                const float k = (float) (numCols - 1);
                const float f0 = 20.f * std::pow (1000.f, ((float) i - 0.5f) / k);
                const float f1 = 20.f * std::pow (1000.f, ((float) i + 0.5f) / k);
                const float fc = 20.f * std::pow (1000.f, (float) i / k);

                int b0 = std::max (1, (int) std::floor (f0 * (float) N / sr));
                int b1 = std::min (N / 2 - 1, (int) std::ceil (f1 * (float) N / sr));
                b0 = std::min (b0, N / 2 - 1);
                if (b1 < b0) b1 = b0;

                float m = 0.f;
                for (int b = b0; b <= b1; ++b)
                    m = std::max (m, fftData[(size_t) b]);

                const float level = m / ((float) N * 0.25f);
                const float db = juce::Decibels::gainToDecibels (level, -120.f) + 3.f * std::log2 (fc / 1000.f);
                const float norm = juce::jlimit (0.f, 1.f, (db + 84.f) / 84.f);

                float& s = smoothed[(size_t) i];
                s += (norm - s) * (norm > s ? 0.65f : 0.18f);
            }
        }
    }
    else if (++idleTicks > 8)
    {
        for (auto& s : smoothed)
            s *= 0.92f;
    }

    repaint();
}

void SpectrumView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // bezel
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0c10), 0.f, bounds.getY(),
                                             juce::Colour (0xff3a404b), 0.f, bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 11.f);

    auto screen = bounds.reduced (3.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101a27), 0.f, screen.getY(),
                                             juce::Colour (0xff070b11), 0.f, screen.getBottom(), false));
    g.fillRoundedRectangle (screen, 8.f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (screen, 8.f);
    g.reduceClipRegion (clip);

    auto xOf = [&screen] (float f)
    {
        const float t = std::log (juce::jlimit (20.f, 20000.f, f) / 20.f) / std::log (1000.f);
        return screen.getX() + t * screen.getWidth();
    };

    // grid
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    const float gridF[] = { 30.f, 50.f, 100.f, 200.f, 300.f, 500.f, 1000.f, 2000.f, 3000.f, 5000.f, 10000.f };
    for (float f : gridF)
        g.drawVerticalLine ((int) xOf (f), screen.getY(), screen.getBottom());
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (screen.getY() + screen.getHeight() * (float) i * 0.25f), screen.getX(), screen.getRight());

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.setFont (ios::font (9.f));
    const std::pair<float, const char*> gridLabels[] = { { 100.f, "100" }, { 1000.f, "1k" }, { 10000.f, "10k" } };
    for (auto& gl : gridLabels)
        g.drawText (gl.second, juce::Rectangle<float> (xOf (gl.first) + 2.f, screen.getBottom() - 13.f, 28.f, 11.f),
                    juce::Justification::centredLeft);

    // crossover positions
    const float fLM = proc.apvts.getRawParameterValue ("lowMid")->load();
    const float fMH = std::max (proc.apvts.getRawParameterValue ("midHigh")->load(), fLM + 50.f);
    const float xLM = xOf (fLM);
    const float xMH = xOf (fMH);

    const juce::Colour cols[3] = { ios::lowCol, ios::midCol, ios::highCol };
    const char* names[3] = { "LOW", "MID", "HIGH" };
    const float edges[4] = { screen.getX(), xLM, xMH, screen.getRight() };

    auto& dsp = proc.getEnhancer();

    // band tint, pulses with the band's transient activity
    for (int b = 0; b < 3; ++b)
    {
        juce::Rectangle<float> r (edges[b], screen.getY(), edges[b + 1] - edges[b], screen.getHeight());
        const float act = std::sqrt (juce::jlimit (0.f, 1.f, dsp.getBandTransient (b)));
        g.setColour (cols[b].withAlpha (0.05f + 0.30f * act));
        g.fillRect (r);
    }

    // spectrum path
    juce::Path line;
    for (int i = 0; i < numCols; ++i)
    {
        const float x = screen.getX() + screen.getWidth() * (float) i / (float) (numCols - 1);
        const float y = screen.getBottom() - 2.f - smoothed[(size_t) i] * (screen.getHeight() - 8.f);
        if (i == 0) line.startNewSubPath (x, y);
        else        line.lineTo (x, y);
    }
    juce::Path fill (line);
    fill.lineTo (screen.getRight(), screen.getBottom());
    fill.lineTo (screen.getX(), screen.getBottom());
    fill.closeSubPath();

    for (int b = 0; b < 3; ++b)
    {
        juce::Rectangle<float> r (edges[b], screen.getY(), edges[b + 1] - edges[b], screen.getHeight());
        g.saveState();
        g.reduceClipRegion (r.toNearestInt());
        g.setGradientFill (juce::ColourGradient (cols[b].withAlpha (0.85f), 0.f, screen.getY(),
                                                 cols[b].withAlpha (0.04f), 0.f, screen.getBottom(), false));
        g.fillPath (fill);
        g.setColour (cols[b].brighter (0.5f));
        g.strokePath (line, juce::PathStrokeType (1.6f));
        g.restoreState();
    }

    // faint waveform of the processed signal
    {
        juce::Path w;
        const int W = std::max (2, (int) screen.getWidth());
        const float midY = screen.getCentreY();
        for (int px = 0; px < W; px += 2)
        {
            const int idx = juce::jlimit (0, TransientStereoAudioProcessor::scopeSize - 1,
                                          px * TransientStereoAudioProcessor::scopeSize / W);
            const float yy = midY - juce::jlimit (-1.f, 1.f, wave[(size_t) idx]) * screen.getHeight() * 0.45f;
            if (px == 0) w.startNewSubPath (screen.getX(), yy);
            else         w.lineTo (screen.getX() + (float) px, yy);
        }
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.strokePath (w, juce::PathStrokeType (1.f));
    }

    // crossover lines + labels
    const float dashes[] = { 3.f, 3.f };
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawDashedLine (juce::Line<float> (xLM, screen.getY(), xLM, screen.getBottom()), dashes, 2, 1.f);
    g.drawDashedLine (juce::Line<float> (xMH, screen.getY(), xMH, screen.getBottom()), dashes, 2, 1.f);

    auto fmt = [] (float f) { return f >= 1000.f ? juce::String (f / 1000.f, 1) + "k" : juce::String ((int) f); };
    g.setFont (ios::font (9.f, true));
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.drawText (fmt (fLM), juce::Rectangle<float> (xLM + 3.f, screen.getY() + 13.f, 40.f, 11.f), juce::Justification::centredLeft);
    g.drawText (fmt (fMH), juce::Rectangle<float> (xMH + 3.f, screen.getY() + 13.f, 40.f, 11.f), juce::Justification::centredLeft);

    // band names
    g.setFont (ios::font (10.f, true));
    for (int b = 0; b < 3; ++b)
    {
        juce::Rectangle<float> r (edges[b], screen.getY() + 2.f, edges[b + 1] - edges[b], 12.f);
        g.setColour (cols[b].brighter (0.3f).withAlpha (0.95f));
        g.drawText (names[b], r, juce::Justification::centred);
    }

    // glass reflection
    auto top = screen.withHeight (screen.getHeight() * 0.45f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.08f), 0.f, top.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0.f, top.getBottom(), false));
    g.fillRect (top);

    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawRoundedRectangle (screen, 8.f, 1.f);
}

//==============================================================================
// Meters
//==============================================================================
void BandMeterView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto labelArea = b.removeFromBottom (14.f);

    const juce::Colour cols[3] = { ios::lowCol, ios::midCol, ios::highCol };
    const char* names[3] = { "LOW", "MID", "HIGH" };
    const float colW = b.getWidth() / 3.f;
    constexpr int segs = 12;

    for (int c = 0; c < 3; ++c)
    {
        juce::Rectangle<float> col (b.getX() + colW * (float) c + colW * 0.2f, b.getY(), colW * 0.6f, b.getHeight());
        const float v = std::sqrt (juce::jlimit (0.f, 1.f, dsp.getBandTransient (c)));
        const float segH = col.getHeight() / (float) segs;

        for (int s = 0; s < segs; ++s)
        {
            auto seg = juce::Rectangle<float> (col.getX(), col.getBottom() - (float) (s + 1) * segH, col.getWidth(), segH).reduced (0.f, 1.f);
            const bool lit = (float) s < v * (float) segs;

            if (lit)
            {
                const auto base = cols[c].interpolatedWith (juce::Colours::white, 0.35f * (float) s / (float) segs);
                g.setGradientFill (juce::ColourGradient (base.brighter (0.3f), 0.f, seg.getY(),
                                                         base.darker (0.2f), 0.f, seg.getBottom(), false));
            }
            else
            {
                g.setColour (juce::Colour (0xff0d1117));
            }
            g.fillRoundedRectangle (seg, 1.5f);

            if (! lit)
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.drawRoundedRectangle (seg, 1.5f, 0.6f);
            }
        }

        g.setColour (cols[c].brighter (0.2f).withAlpha (0.9f));
        g.setFont (ios::font (9.f, true));
        g.drawText (names[c], juce::Rectangle<float> (b.getX() + colW * (float) c, labelArea.getY(), colW, labelArea.getHeight()),
                    juce::Justification::centred);
    }
}

void PhaseMeterView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float corr = dsp.getPhaseCorrelation();
    const float red = dsp.getSafetyReduction();

    // header row
    auto head = b.removeFromTop (14.f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (ios::font (9.f, true));
    g.drawText ("PHASE", head, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText (juce::String (corr >= 0.f ? "+" : "") + juce::String (corr, 2), head, juce::Justification::centredRight);

    // strip
    auto strip = b.removeFromTop (16.f).reduced (0.f, 3.f);
    g.setColour (juce::Colour (0xff06080c));
    g.fillRoundedRectangle (strip, strip.getHeight() * 0.5f);

    const float cx = strip.getCentreX();
    const float px = strip.getX() + (corr * 0.5f + 0.5f) * strip.getWidth();
    juce::Rectangle<float> fillR (std::min (cx, px), strip.getY() + 1.f, std::abs (px - cx), strip.getHeight() - 2.f);
    const auto col = corr >= 0.f ? juce::Colour (0xff34c759) : juce::Colour (0xffff453a);
    g.setGradientFill (juce::ColourGradient (col.brighter (0.4f), 0.f, fillR.getY(),
                                             col.darker (0.3f), 0.f, fillR.getBottom(), false));
    g.fillRoundedRectangle (fillR, 3.f);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (cx, strip.getY(), cx, strip.getBottom(), 1.f);
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (px - 1.5f, strip.getY() - 1.f, 3.f, strip.getHeight() + 2.f, 1.5f);

    // safety LED
    b.removeFromTop (4.f);
    auto row = b.removeFromTop (16.f);
    const float lit = juce::jlimit (0.f, 1.f, red * 4.f);
    const auto amber = juce::Colour (0xffffcc00);
    auto led = juce::Rectangle<float> (10.f, 10.f).withCentre (juce::Point<float> (row.getX() + 7.f, row.getCentreY()));

    g.setColour (juce::Colour (0xff05070a));
    g.fillEllipse (led.expanded (1.5f));
    if (lit > 0.05f)
    {
        g.setColour (amber.withAlpha (0.25f * lit));
        g.fillEllipse (led.expanded (4.f));
    }
    g.setColour (amber.withAlpha (0.15f + 0.85f * lit));
    g.fillEllipse (led);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (led.reduced (2.5f).translated (-0.8f, -1.2f).withSize (3.f, 2.f));

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (ios::font (9.f, true));
    g.drawText ("SAFETY", row.withTrimmedLeft (18.f), juce::Justification::centredLeft);
}

//==============================================================================
// Editor
//==============================================================================
namespace
{
    struct KnobSpec
    {
        const char* id;
        const char* name;
        const char* suffix;
        int decimals;
        juce::Colour colour;
    };

    const KnobSpec knobSpecs[10] = {
        // Detection
        { "sens",     "SENSITIVITY", "",     2, ios::blueCol },
        { "thresh",   "THRESHOLD",   "",     2, ios::blueCol },
        { "decay",    "DECAY",       " ms",  0, ios::blueCol },
        { "phaseThr", "PHASE LIMIT", "",     2, juce::Colour (0xffffcc00) },
        { "blend",    "BLEND",       "",     2, juce::Colour (0xffbf5af2) },
        // Bands
        { "lowMid",    "LOW / MID",  " Hz",  0, juce::Colour (0xffffffff) },
        { "midHigh",   "MID / HIGH", " Hz",  0, juce::Colour (0xffffffff) },
        { "widthLow",  "LOW WIDTH",  "x",    2, ios::lowCol },
        { "widthMid",  "MID WIDTH",  "x",    2, ios::midCol },
        { "widthHigh", "HIGH WIDTH", "x",    2, ios::highCol }
    };

    constexpr int kWidth = 620;
    constexpr int kHeight = 446;
}

TransientStereoAudioProcessorEditor::TransientStereoAudioProcessorEditor (TransientStereoAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), meters (p.getEnhancer()), phase (p.getEnhancer())
{
    setLookAndFeel (&laf);

    for (size_t i = 0; i < knobs.size(); ++i)
        setupKnob (knobs[i], knobSpecs[i].id, knobSpecs[i].name, knobSpecs[i].suffix, knobSpecs[i].decimals, knobSpecs[i].colour);

    setupToggle (bypassToggle, "bypass", "Bypass");
    setupToggle (safetyToggle, "phaseSafety", "Safety");
    setupToggle (slopeToggle, "slope24", "24 dB");

    addAndMakeVisible (spectrum);
    addAndMakeVisible (meters);
    addAndMakeVisible (phase);

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

TransientStereoAudioProcessorEditor::~TransientStereoAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TransientStereoAudioProcessorEditor::setupKnob (Knob& k, const juce::String& id, const juce::String& name,
                                                     const juce::String& suffix, int decimals, juce::Colour colour)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setNumDecimalPlacesToDisplay (decimals);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, colour);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (ios::font (10.f, true));
    k.label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void TransientStereoAudioProcessorEditor::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void TransientStereoAudioProcessorEditor::timerCallback()
{
    spectrum.update();
    meters.repaint();
    phase.repaint();
}

void TransientStereoAudioProcessorEditor::renderBackground()
{
    const int w = getWidth();
    const int h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    background = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (background);

    // base + linen texture
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b4351), 0.f, 0.f,
                                             juce::Colour (0xff151922), 0.f, (float) h, false));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int y = 0; y < h; y += 2)
        g.drawHorizontalLine (y, 0.f, (float) w);
    g.setColour (juce::Colours::black.withAlpha (0.05f));
    for (int x = 0; x < w; x += 2)
        g.drawVerticalLine (x, 0.f, (float) h);

    // glossy header bar
    juce::Rectangle<float> header (0.f, 0.f, (float) w, 40.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5263), 0.f, 0.f,
                                             juce::Colour (0xff1c212b), 0.f, header.getBottom(), false));
    g.fillRect (header);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawHorizontalLine (0, 0.f, (float) w);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawHorizontalLine (39, 0.f, (float) w);

    // embossed title
    g.setFont (ios::font (20.f, true));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawText ("Transient Stereo", juce::Rectangle<float> (18.f, 3.f, 260.f, 34.f), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xfff0f3f7));
    g.drawText ("Transient Stereo", juce::Rectangle<float> (18.f, 2.f, 260.f, 34.f), juce::Justification::centredLeft);

    // small band LEDs next to the title
    const juce::Colour cols[3] = { ios::lowCol, ios::midCol, ios::highCol };
    for (int i = 0; i < 3; ++i)
    {
        auto led = juce::Rectangle<float> (8.f, 8.f).withCentre (juce::Point<float> (212.f + (float) i * 14.f, 20.f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (led.expanded (1.5f));
        g.setColour (cols[i]);
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillEllipse (led.reduced (2.f).translated (-0.5f, -1.f).withSize (3.f, 2.f));
    }

    // plates
    auto plate = [&g] (juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.f, 2.f), 12.f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e3544), 0.f, r.getY(),
                                                 juce::Colour (0xff1a1f29), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 12.f);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (r.reduced (0.5f), 12.f, 1.f);

        auto gloss = r.reduced (2.f, 2.f).withHeight (r.getHeight() * 0.28f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0.f, gloss.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), 0.f, gloss.getBottom(), false));
        g.fillRoundedRectangle (gloss, 10.f);

        g.setFont (ios::font (9.5f, true));
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawText (title, juce::Rectangle<float> (r.getX() + 12.f, r.getY() + 5.f, r.getWidth() - 20.f, 12.f), juce::Justification::centredLeft);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawText (title, juce::Rectangle<float> (r.getX() + 12.f, r.getY() + 4.f, r.getWidth() - 20.f, 12.f), juce::Justification::centredLeft);
    };

    plate ({ 14.f, 204.f, 470.f, 112.f }, "TRANSIENT DETECTION");
    plate ({ 14.f, 322.f, 470.f, 112.f }, "BANDS & CROSSOVER");
    plate ({ 490.f, 204.f, 116.f, 230.f }, "METERS");
}

void TransientStereoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);
}

void TransientStereoAudioProcessorEditor::resized()
{
    renderBackground();

    spectrum.setBounds (14, 46, 592, 150);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / 5;
        const int col = (int) i % 5;
        juce::Rectangle<int> cell (14 + col * 94, 204 + 18 + row * 118, 94, 92);
        knobs[i].label.setBounds (cell.removeFromTop (14));
        knobs[i].slider.setBounds (cell);
    }

    meters.setBounds (498, 222, 100, 96);
    phase.setBounds (498, 324, 100, 52);
    safetyToggle.button.setBounds (500, 380, 100, 24);
    slopeToggle.button.setBounds (500, 405, 100, 24);
    bypassToggle.button.setBounds (kWidth - 116, 8, 104, 24);
}
