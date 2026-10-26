# VelvetSpace
Reverb Plugin for Ableton
# Velvet Space 2

Velvet Space is a vintage-flavoured algorithmic reverb plug-in for VST3 (AU too on macOS), built with JUCE 8. Its layout follows the "ALL IN ONE / MODULES" workflow:

- **SPACE** (reverb), **COLOR** (saturation and post EQ), **MOTION** (pan, orbit and gate) and **PUMP** (ducking) panels.
- A 16-lane **MODULATION MATRIX** underneath.

All of the DSP is original and lives in `Source/Dsp.h`.

![All in one](Source%20project/docs/all-in-one.png)

## Features

| # | Feature | What it does |
|---|---|---|
| 1 | Smooth morphing reverb selection | There are 10 reverb modes: Room, Chamber, Plate, Hall, Vintage 80s, Bloom, Cathedral, Spring, Dark Space and Nebula. **Select** has three settings:<br>• **Snap** glides to the chosen mode over the Morph time.<br>• **Blend** interpolates continuously between neighbouring modes; the Reverb Type knob runs 0–9.<br>• **Cycle** drifts through the modes automatically: rate or tempo sync, span, and Loop / Ping-pong / Drift styles.<br>Every coefficient is smoothed per sample (delay lengths, feedback, diffusion, damping, bloom and spring amounts), so morphing doesn't click. |
| 2 | Multi-band, multi-type saturation | It's wet signal only. There are 3 bands with adjustable crossovers (Linkwitz-Riley, sums flat). Each band picks one of 10 types: Clean, Tape, Tube, Soft clip, Hard clip, Fold, Rectify, Asymmetric, Bitcrush or Downsample. Each band has 0–36 dB of level-compensated drive. There's also a trim and a **saturation wet/dry**. |
| 3 | Post-saturation shaping EQ | 3 bands with **adjustable low/mid and mid/high crossovers**, plus a parametric bell. In the COLOR panel you can drag the crossovers, band gains and the bell node. |
| 4 | Motion engine (wet only) | • **Auto pan**: rate or sync, 16 shapes, depth.<br>• **3D space orbit**: Full Orbit, Horizontal, Vertical, Figure 8, Spiral and Pendulum, with Around / Up-Down / Near-Far amounts. It uses inter-aural delay, level, a height filter and distance cues.<br>• **Gate / tremolo**: rhythmic (synced shapes) or envelope (threshold and hold), with depth and smoothing. |
| 5 | Sidechain / ducking engine | Envelope ducking, a tempo-synced pump, or both. Settings: depth, sensitivity, attack, release, pump rate and curve, and **duck wet/dry**. It's keyed from the input or the **sidechain bus**, and a gain-reduction meter shows what it's doing. |
| 6 | Modulation matrix | 16 lanes (add or remove them in the UI). Each lane has Source, Destination, Direction (Both / Up / Down) and Depth (±100 %).<br>• **Sources**: Audio Env, MIDI Env, LFO 1–4, Macro 1–4, Velocity, Random per note, Duck amount and Sidechain Env.<br>• **LFOs**: 16 shapes; free or tempo sync from 1/64 to 8 bars, including dotted and triplet values; Free / Host / MIDI / Audio retrigger; phase.<br>• **Destinations**: all 122 continuous and switchable parameters, including the LFOs themselves.<br>Knobs show a live modulation ring. |
| 7 | Visual graphs | • **Tail field**: the morphing decay shape, with mode weights.<br>• **Dry vs wet spectrum**, with the ducking line.<br>• **Crossover map**: saturation transfer curves per band.<br>• **Post-EQ curve**.<br>• **Orbit / pan / gate scope**.<br>• **Pump shape** and **LFO scope**.<br>A big **VISUALS** page shows them all together. |

Other features:

- 26 named factory presets in categories.
- Saving user presets (`SAVE PRESET`), plus delete, import and export from the `•••` menu.
- **Preset morph**: loading a preset glides from the current sound to the new one.
- A wet/dry mix that leaves the dry path untouched.
- Input and output gain, freeze, and MIDI input (for the MIDI envelope, velocity and retrigger).
- Window size of 75–130 %.
- A collapsible matrix.

## Pages

| ALL IN ONE (matrix hidden) | MODULES / SPACE |
|---|---|
| ![](Source%20project/docs/matrix-hidden.png) | ![](Source%20project/docs/modules-space.png) |
| **MODULES / COLOR** | **MODULES / VISUALS** |
| ![](Source%20project/docs/modules-color.png) | ![](Source%20project/docs/modules-visuals.png) |

## Factory presets

- **Utility**: Clean Canvas
- **Halls**: Velvet Hall, 1984 Concert Hall
- **Ambient**: Bloom Garden, Dark Space Drone, Freeze Pad
- **Plates**: Bright Plate, Vocal Plate Ducked
- **Rooms**: Tight Drum Room
- **Chambers**: Wood Chamber
- **Huge**: Cathedral Wash
- **Vintage**: Surf Spring
- **Morphing**: Nebula Cycle, Endless Morph, Drift Between Worlds
- **Color**: Tube Warm Hall, Lo-Fi Crushed Verb, Folded Shimmer
- **Motion**: Orbital Hall, Auto Pan Plate, Gated 80s Snare, Trance Gate Verb
- **Pump**: Sidechain Pump Hall, Kick-Keyed Ambience
- **Modulated**: Breathing Room, MIDI Swell

User presets are `.vspreset` XML files saved in:

- **Windows**: `%APPDATA%\Velvet Space\Presets`
- **macOS**: `~/Library/Velvet Space/Presets`
- **Linux**: `~/.config/Velvet Space/Presets`

## Install

- **Windows x64**: copy the whole `Velvet Space.vst3` folder to `C:\Program Files\Common Files\VST3\` and rescan plug-ins. In Ableton that's Preferences → Plug-ins → Rescan. It's statically linked, so you don't need a VC++ runtime.
- **Linux x64**: copy the bundle to `~/.vst3/`.
- **macOS**: build it yourself with `./scripts/build-mac.sh` (universal binary, VST3 + AU), or use the GitHub Actions workflow.

**Sidechain in Ableton.** Open the plug-in's device view and use the Sidechain section to pick the key track, then set **Duck source** to *Sidechain* in the PUMP panel.

This is v2, a new plug-in version: parameter IDs changed, so projects saved with v1 open with default settings.

## Build from source

You need CMake 3.22 or later. JUCE 8.0.4 is downloaded automatically, or you can point at a local copy with `-DJUCE_DIR=...`.

- **macOS**: `./scripts/build-mac.sh`
- **Windows** (Visual Studio 2022): `scripts\build-windows.bat`
- **Windows from Linux** (clang-cl + xwin): `scripts/cross-build-windows-from-linux.sh`
- **GitHub Actions**: `.github/workflows/build.yml` builds Windows, macOS and Linux.
- **Tests**: `cmake -B build -DVELVET_BUILD_TESTS=ON && cmake --build build && ./build/velvet_dsp_tests`

## Verification

- `Tests/DspTests.cpp` runs 29 checks, and all of them pass:
  - At 0 % wet the output is bit-identical to the input, with every module active.
  - Measured RT60 is within 25 % of Decay for Hall, Plate and Cathedral at 1, 3 and 8 s.
  - 30 random and extreme settings stay stable, and all 26 presets play cleanly.
  - Ducking reaches −24 dB and recovers after the release.
  - The sidechain key and the tempo pump both work.
  - All 9 non-clean saturation types are active, and saturation wet/dry at 0 % is exact.
  - The post EQ works, and the gate, auto pan and orbit all change the image.
  - Click tests for cycle, snap and blend morphing pass.
  - Every one of the 122 destinations stays stable at full modulation depth.
  - LFO and MIDI lanes work, freeze holds, and the tail dies away to silence.
  - CPU use is about 5 % of one core at 48 kHz with every module and all 16 lanes active.
- The engine was also run under AddressSanitizer and UndefinedBehaviorSanitizer, and both came back clean.
- [pluginval](https://github.com/Tracktion/pluginval) passed at strictness level 10 on the Linux VST3 (including GUI tests) and on the Windows VST3 (under Wine, without GUI tests).

## Signal flow

```
in ─┬──────────────────────────────────────────────────────────── dry ─┐
    └ input → pre-delay → [mode morph: bandwidth, spring, early refl,  │
      diffusers, bloom smear, 16-line FDN] → reverb cuts               │
      → 3-band saturation (wet/dry) → 3-band post EQ + bell → width    │
      → motion: gate → auto pan → 3D orbit                             │
      → pump: envelope ducker / tempo pump (input or sidechain key)    │
      → wet ───────────────────────────────────────────────── mix ─────┴→ output
```

## Licence notes

JUCE is dual-licensed under AGPLv3 or a commercial licence. Personal use is fine; check the JUCE and VST3 SDK licences before you distribute it commercially.
