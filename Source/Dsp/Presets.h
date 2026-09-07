#pragma once
/* Factory presets, ported verbatim from src/data/presets.ts.
   Any parameter not listed for a preset falls back to its default (see
   ParamLayout.h), exactly like presetValues() in the original spreading
   DEFAULTS first. */

#include <map>
#include <string>
#include <vector>

namespace argentum
{

struct Preset
{
    std::string name;
    std::string category; // VOCAL / DRUMS / SYNTH / GLUE / FX
    int mode;
    std::string note;
    std::map<std::string, float> values; // sparse overrides, keyed by param id
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets =
    {
        { "140 Vocal Plate", "VOCAL", 0,
          "The reference. 18 ms pre-delay keeps consonants forward; 6.5 k damping stops sibilance ringing out.",
          { {"predelay",18},{"size",1},{"decay",2.1f},{"diffusion",1},{"damp",6500},{"bassMul",1.2f},
            {"crossover",340},{"modDepth",0.25f},{"modRate",0.32f},{"lowCut",120},{"highCut",13000},
            {"width",1},{"mix",0.3f},{"duck",0} } },

        { "Cobalt Ballad", "VOCAL", 1,
          "Dark EMT with the 250 Hz resonance. Long, deep, sits behind the singer without EQ surgery.",
          { {"predelay",30},{"size",1.15f},{"decay",3.4f},{"diffusion",1},{"damp",5200},{"bassMul",1.15f},
            {"crossover",300},{"modDepth",0.3f},{"modRate",0.28f},{"lowCut",140},{"highCut",9500},
            {"width",1.1f},{"mix",0.28f} } },

        { "Ducked Lead Vox", "VOCAL", 0,
          "4.5 s tail that only exists between phrases. Ducker at 55% - the modern pop plate move.",
          { {"predelay",22},{"size",1.2f},{"decay",4.5f},{"damp",5600},{"bassMul",1.0f},{"crossover",320},
            {"modDepth",0.3f},{"lowCut",160},{"highCut",11000},{"mix",0.4f},{"duck",0.55f} } },

        { "Snare Slap 1/16", "DRUMS", 0,
          "125 ms pre-delay = a 1/16 at 120 BPM. Tail is short so the groove stays tight.",
          { {"predelay",125},{"size",0.85f},{"decay",1.25f},{"diffusion",1},{"damp",7800},{"bassMul",0.75f},
            {"crossover",420},{"modDepth",0.18f},{"lowCut",220},{"highCut",14000},{"width",1.15f},{"mix",0.34f} } },

        { "Trap Steel Snare", "DRUMS", 3,
          "Bright stainless tail, aggressive low cut, hard duck so the 808 keeps the low end.",
          { {"predelay",60},{"size",0.7f},{"decay",1.8f},{"damp",11000},{"bassMul",0.45f},{"crossover",500},
            {"modDepth",0.35f},{"lowCut",320},{"highCut",16000},{"width",1.3f},{"mix",0.36f},{"duck",0.4f} } },

        { "Drum Bus Glue", "GLUE", 0,
          "0.9 s, 16% wet, 200 Hz low cut. You should only hear it when you bypass it.",
          { {"predelay",8},{"size",0.8f},{"decay",0.9f},{"diffusion",1},{"damp",6800},{"bassMul",0.6f},
            {"crossover",450},{"modDepth",0.12f},{"lowCut",200},{"highCut",12000},{"width",0.85f},{"mix",0.16f} } },

        { "Parallel Mix Sheen", "GLUE", 2,
          "High modal density at 12% wet across a full mix. Adds depth, not reverb.",
          { {"predelay",6},{"size",1.05f},{"decay",1.4f},{"damp",9000},{"bassMul",0.5f},{"crossover",380},
            {"modDepth",0.2f},{"lowCut",260},{"highCut",15000},{"width",1.05f},{"mix",0.12f} } },

        { "Chamber XL", "SYNTH", 2,
          "SIZE 200% pushes the tank past physical-plate density into plaster-chamber territory.",
          { {"predelay",40},{"size",2.0f},{"decay",6.5f},{"diffusion",1},{"damp",7000},{"bassMul",1.4f},
            {"crossover",300},{"modDepth",0.4f},{"modRate",0.22f},{"lowCut",90},{"highCut",12500},
            {"width",1.25f},{"mix",0.42f} } },

        { "Unobtanium Bloom", "SYNTH", 3,
          "Long HF decay + 55% modulation. Made for pads and plucks; would chorus a solo cello.",
          { {"predelay",55},{"size",1.6f},{"decay",9},{"damp",13000},{"bassMul",1.1f},{"crossover",260},
            {"modDepth",0.55f},{"modRate",0.45f},{"lowCut",100},{"highCut",18000},{"width",1.4f},{"mix",0.5f} } },

        { "Osmium Retro Send", "VOCAL", 4,
          "Mono-in, stereo-out. Dark and booming exactly like a single-driver plate on a 70s desk.",
          { {"predelay",24},{"size",1.1f},{"decay",2.6f},{"damp",4200},{"bassMul",1.6f},{"crossover",260},
            {"modDepth",0.15f},{"lowCut",150},{"highCut",7500},{"width",1.2f},{"mix",0.34f} } },

        { "Lo-Fi Tape Plate", "FX", 1,
          "Small, dark, heavily modulated. 3.5 k high cut sells the tape-return illusion.",
          { {"predelay",16},{"size",0.55f},{"decay",1.6f},{"diffusion",0.8f},{"damp",3000},{"bassMul",1.8f},
            {"crossover",220},{"modDepth",0.65f},{"modRate",0.8f},{"lowCut",180},{"highCut",3500},
            {"width",0.7f},{"mix",0.38f} } },

        { "Scatter / Low Diffusion", "FX", 0,
          "Diffusion pulled to 35%: discrete lattice echoes instead of a wash. Great on sparse percussion.",
          { {"predelay",45},{"size",0.95f},{"decay",3},{"diffusion",0.35f},{"damp",8000},{"bassMul",0.9f},
            {"crossover",400},{"modDepth",0.25f},{"lowCut",150},{"highCut",14000},{"width",1.35f},{"mix",0.4f} } },

        { "Infinite Sustain Pad", "FX", 2,
          "Set 28 s and hit FREEZE for an unending bed. Input is muted in freeze, so the tank never overloads.",
          { {"predelay",0},{"size",1.8f},{"decay",28},{"damp",8500},{"bassMul",1.0f},{"crossover",300},
            {"modDepth",0.45f},{"modRate",0.18f},{"lowCut",110},{"highCut",14000},{"width",1.5f},{"mix",0.6f} } },

        { "Guitar Slapback Plate", "FX", 0,
          "95 ms pre-delay, 0.6 s tail, narrow. Rockabilly depth without smearing the pick attack.",
          { {"predelay",95},{"size",0.6f},{"decay",0.6f},{"damp",7500},{"bassMul",0.8f},{"crossover",400},
            {"modDepth",0.1f},{"lowCut",200},{"highCut",11000},{"width",0.6f},{"mix",0.26f} } },
    };
    return presets;
}

} // namespace argentum
