# Argentum Plate — VST3 port (JUCE / C++)

This is a from-scratch **JUCE C++ project** that ports the "Argentum Plate"
web reverb (React + AudioWorklet) into a real, host-loadable **VST3 plugin**
for Windows / FL Studio.

I built and wrote this source on a Linux sandbox that has no Windows
toolchain, no JUCE install and no DAW to test in — so I could not compile
the final `.vst3` binary or verify it in FL Studio myself. What's in this
folder is a complete, ready-to-build project; you'll do the actual
`.vst3` build on your own Windows machine (free tools, ~15–20 minutes,
steps below).

## What's an exact port vs. what's approximated

**DSP — exact, line-for-line port** (`Source/Dsp/PlateReverbDSP.h`):
every delay length, coefficient, filter, the LFO/random-walk modulation,
the ducking envelope, the Dattorro figure-of-eight tank routing, the five
ALLOY modes, and the tone/width/mix stages are translated directly from
`src/audio/plate-worklet.ts`, keeping the same variable names and order of
operations so the two engines behave identically.

**Parameters & presets — exact** (`Source/Dsp/ParamLayout.h`,
`Source/Dsp/Presets.h`): same 14 knobs with the same min/max/default and
log/linear taper as `params.ts`, plus FREEZE / BYPASS / TRUE STEREO and
the ALLOY mode choice, and all 14 factory presets from `presets.ts`.

**GUI — close recreation, not pixel-identical:**
- Ported: dark theme/colour palette, header with title + preset menu +
  INIT/FREEZE/BYPASS, the 5 ALLOY mode buttons + TRUE STEREO, the four
  knob groups (SPACE/TONE/MOTION/OUTPUT) with the same knobs in the same
  groups, an RT60-vs-frequency decay plot driven by the same analytic
  model as the web version, and IN/WET/duck meters.
- Simplified or left out for this first pass: the draggable nodes on the
  decay curve (it's currently a read-only reference plot), the animated
  plate-texture visualizer (`PlateField.tsx`) and the little oscilloscope
  (`DecayScope.tsx`), the tempo-synced pre-delay row, and the engineering
  "docs" dossier panel. The web build's transport/source-player (PLAY,
  PINK CLICK, LOAD FILE, etc.) doesn't carry over at all — that was only
  there so the browser demo had something to feed itself; inside FL
  Studio the host provides the audio instead.
- Param tooltips (hover text) are wired up using native JUCE tooltips
  rather than a custom hint bar.

Tell me which of the simplified pieces you want built out next and I'll
add them — the draggable decay-curve nodes and the plate visualizer are
both very doable, just additional GUI work.

## Build it (Windows)

You need two free tools:

1. **Visual Studio 2022 Community** — https://visualstudio.microsoft.com/
   During install, tick the **"Desktop development with C++"** workload.
2. **CMake** — https://cmake.org/download/ (or `winget install Kitware.CMake`).

Then:

```bat
cd path\to\ArgentumPlate
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The first `cmake -B build` step downloads JUCE (github.com) automatically
via `FetchContent` — it needs an internet connection and will take a few
minutes the first time only.

If you'd rather not use the command line, open the folder in Visual
Studio ("Open a local folder") — VS's built-in CMake support will
configure and build it from the Solution Explorer / CMake menu instead.

### Where the .vst3 lands

`COPY_PLUGIN_AFTER_BUILD TRUE` in `CMakeLists.txt` makes JUCE copy the
built plugin straight to your system VST3 folder
(`C:\Program Files\Common Files\VST3\Argentum Plate.vst3`) after a
successful build. You can also find it manually at:

```
build\ArgentumPlate_artefacts\Release\VST3\Argentum Plate.vst3
```

## Install into FL Studio

1. Confirm FL Studio is scanning `C:\Program Files\Common Files\VST3`
   (Options → Manage Plugins → Locate/scan for new or updated plugins →
   make sure that folder is in the search paths, then Find Plugins).
2. Rescan (Options → Manage Plugins → Find plugins), or just reopen FL
   Studio.
3. It should show up under Generators/Effects as **Argentum Plate**
   (VST3, 64-bit).

## If the build errors out

Paste me the error text and I'll fix the source — the most common first-
build issues are a missing "Desktop development with C++" workload, or
CMake not finding a 64-bit generator (add `-A x64` as shown above).

## Project layout

```
ArgentumPlate/
  CMakeLists.txt
  Source/
    PluginProcessor.h/.cpp     JUCE AudioProcessor, param plumbing, state save/load
    PluginEditor.h/.cpp        Main GUI layout
    Dsp/
      PlateReverbDSP.h         The reverb engine itself (ported from plate-worklet.ts)
      ParamLayout.h            Parameter specs + curves + analytic decay math
      Presets.h                Factory presets
    GUI/
      PluginLookAndFeel.h      Colours + custom rotary-knob painting
      KnobComponent.h          Knob + label + value readout
      DecayCurveComponent.h    RT60-vs-frequency plot
      MeterComponent.h         IN / WET / GR bar meters
```
