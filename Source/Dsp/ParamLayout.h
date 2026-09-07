#pragma once
/* Parameter model — ported from src/audio/params.ts. Min/max/default/curve
   for every knob match the web version exactly, including the log-curve
   normalisation (toNorm/fromNorm), so automation and knob taper feel
   identical to the original. */

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <algorithm>

namespace argentum
{

enum class Curve { Lin, Log };

struct ParamSpec
{
    const char* id;
    const char* label;   // long label, e.g. "PRE-DELAY"
    const char* shortLbl;// short label under the knob, e.g. "PRE"
    float min, max, def;
    Curve curve;
    const char* unit;
    const char* tip;
};

// Order mirrors PARAM_ORDER in params.ts (display order in the GUI groups).
static const ParamSpec kParams[] =
{
    { "predelay",  "PRE-DELAY", "PRE",  0.0f,   250.0f,   14.0f,  Curve::Lin,
      "ms", "Gap between dry transient and the onset of plate diffusion. 10-30 ms keeps vocals intelligible; sync to 1/16 for rhythmic depth." },
    { "size",      "SIZE",      "SIZE", 0.25f,  2.5f,     1.0f,   Curve::Log,
      "%",  "Scales every tank delay = modal density. <100% is a small, metallic physical plate. >100% pushes into chamber density." },
    { "decay",     "DECAY",     "DEC",  0.2f,   30.0f,    2.4f,   Curve::Log,
      "s",  "Mid-band RT60. Feedback coefficient is solved from loop time, so decay stays constant when you change SIZE." },
    { "diffusion", "DIFFUSION", "DIFF", 0.2f,   1.0f,     1.0f,   Curve::Lin,
      "%",  "Lattice all-pass coefficients. High = instant echo density (true plate). Low = discrete, fluttery scatter." },
    { "damp",      "HF DAMP",   "DAMP", 700.0f, 20000.0f, 6200.0f,Curve::Log,
      "Hz", "In-tank one-pole LP, applied once per half-loop." },
    { "bassMul",   "BASS MULT", "BASS", 0.1f,   4.0f,     1.35f,  Curve::Log,
      "x",  "Low-band decay multiplier. Auto-clamped so the tank can never build up runaway low end." },
    { "crossover", "CROSSOVER", "XOVER",60.0f,  1600.0f,  380.0f, Curve::Log,
      "Hz", "Corner between the bass-multiplied band and the mid band." },
    { "modDepth",  "MOD DEPTH", "MOD",  0.0f,   1.0f,     0.28f,  Curve::Lin,
      "%",  "Excursion of the two modulated tank all-passes. De-metallizes the tail." },
    { "modRate",   "MOD RATE",  "RATE", 0.02f,  4.0f,     0.35f,  Curve::Log,
      "Hz", "Hybrid LFO: 70% sine + 30% band-limited random walk." },
    { "lowCut",    "LOW CUT",   "LC",   20.0f,  1000.0f,  90.0f,  Curve::Log,
      "Hz", "12 dB/oct on the wet bus only." },
    { "highCut",   "HIGH CUT",  "HC",   1200.0f,20000.0f, 12000.0f,Curve::Log,
      "Hz", "6 dB/oct on the wet bus. Tames sibilance ring-out." },
    { "width",     "WIDTH",     "WID",  0.0f,   2.0f,     1.0f,   Curve::Lin,
      "%",  "M/S scaling of the wet bus." },
    { "duck",      "DUCK",      "DUCK", 0.0f,   1.0f,     0.0f,   Curve::Lin,
      "%",  "Envelope-follower ducking of the wet bus from the dry input. 4 ms attack / 180 ms release." },
    { "mix",       "MIX",       "MIX",  0.0f,   1.0f,     0.32f,  Curve::Lin,
      "%",  "Equal-power dry/wet." },
};
static constexpr int kNumContinuousParams = (int) (sizeof (kParams) / sizeof (ParamSpec));

static const char* kModeNames[5] = { "CHROME", "COBALT", "ALUMINIUM", "UNOBTANIUM", "OSMIUM" };
static const char* kModeBlurbs[5] =
{
    "Neutral cold-rolled steel. The reference 140. Soft attack, slightly bright, zero character tax.",
    "Dark, deeper attack, with a 250 Hz low-mid resonance measured off a well-worn EMT 140.",
    "Extra lattice stage: far higher modal density. Above 120% SIZE it behaves like a plaster chamber.",
    "Stainless / Ecoplate brightness and long HF tail - with the metallic ring engineered out.",
    "Mono-in, stereo-out. Dark and booming; the densest tail here. Retro send-bus behaviour.",
};

inline float toNorm (const ParamSpec& s, float v)
{
    const float c = std::clamp (v, s.min, s.max);
    if (s.curve == Curve::Log) return std::log (c / s.min) / std::log (s.max / s.min);
    return (c - s.min) / (s.max - s.min);
}

inline float fromNorm (const ParamSpec& s, float n)
{
    const float c = std::clamp (n, 0.0f, 1.0f);
    if (s.curve == Curve::Log) return s.min * std::pow (s.max / s.min, c);
    return s.min + (s.max - s.min) * c;
}

/** Builds a JUCE NormalisableRange whose 0..1 <-> real mapping matches
    toNorm/fromNorm exactly (so host automation curves match the web knob taper). */
inline juce::NormalisableRange<float> makeRange (const ParamSpec& s)
{
    juce::NormalisableRange<float> r (s.min, s.max,
        [s] (float, float, float normalised)  { return fromNorm (s, normalised); },
        [s] (float, float, float value)       { return toNorm (s, value); });
    return r;
}

//======================================================================
// Boolean / choice parameter IDs (not part of kParams above).
static constexpr const char* kFreezeId     = "freeze";
static constexpr const char* kBypassId     = "bypass";
static constexpr const char* kTrueStereoId = "trueStereo";
static constexpr const char* kModeId       = "mode";

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (auto& s : kParams)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { s.id, 1 }, s.label, makeRange (s), s.def,
            juce::AudioParameterFloatAttributes().withLabel (s.unit)));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kFreezeId, 1 }, "Freeze", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kBypassId, 1 }, "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kTrueStereoId, 1 }, "True Stereo", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { kModeId, 1 }, "Alloy",
        juce::StringArray { "CHROME", "COBALT", "ALUMINIUM", "UNOBTANIUM", "OSMIUM" }, 0));

    return { params.begin(), params.end() };
}

//======================================================================
// Analytic decay-curve model, ported from the same functions in params.ts.
// Used to draw the RT60-vs-frequency curve in the GUI without having to
// run the DSP offline.
static constexpr double kLoopSamples = 21589.0; // 672+4453+1800+3720+908+4217+2656+3163
inline double loopTime (double size) { return (kLoopSamples * size) / kBaseSampleRate; }

inline double onePoleMag (double f, double fc, double sr)
{
    const double c = std::exp ((-2.0 * M_PI * std::min (fc, sr * 0.48)) / sr);
    const double w = (2.0 * M_PI * f) / sr;
    return (1.0 - c) / std::sqrt (1.0 - 2.0 * c * std::cos (w) + c * c);
}

struct DecayCurveInputs
{
    double size, decay, damp, bassMul, crossover;
};

/** RT60 in seconds at frequency f — mirrors the DSP's feedback path. */
inline double rt60At (double f, const DecayCurveInputs& p, double hfMul, double sr = 48000.0)
{
    const double T = loopTime (p.size);
    const double dG = std::min (0.99995, std::pow (10.0, (-0.75 * T) / std::max (0.05, p.decay)));
    const double bassLimit = 0.9994 / (dG * dG);
    const double bass = std::min (p.bassMul, bassLimit);

    const double hd = onePoleMag (f, p.damp * hfMul, sr);
    const double hlp = onePoleMag (f, p.crossover, sr);
    const double hb = std::abs (1.0 + (bass - 1.0) * hlp);

    double G = std::pow (dG, 4.0) * hd * hd * hb * hb;
    G = std::min (0.9999999, std::max (1e-12, G));
    return (3.0 * T) / -std::log10 (G);
}

} // namespace argentum
