# Zinox Vocals

A professional vocal channel strip in VST3 / AU / Standalone form, built with JUCE 8.

One knob per job, arranged the way you actually work on a vocal: clean it, shape it,
control it, thicken it, colour it, and catch the peaks. Every stage is a real DSP
implementation — nothing is a placeholder.

---

## The signal chain

```
Input trim
   → Denoise (adaptive spectral noise reduction)
   → Low Cut (24 dB/oct)  →  High Cut (24 dB/oct)
   → Low band  →  Mid band  →  High band
   → De-Ess (split-band)
   → Control (compressor)
   ┌──── oversampled 2x / 4x / 8x ────┐
   │  Push (parallel density)         │
   │  Saturate (Rich/Warm/Tape/Tube)  │
   └──────────────────────────────────┘
   → Output trim
   → Look-ahead brickwall Limiter
   → Dry/Wet mix
```

## The controls

### Left column

| Control      | What it does |
|--------------|--------------|
| **Input**    | ±24 dB trim into the strip. Set this so Control sees a healthy signal. |
| **Denoise**  | Adaptive spectral noise reduction. Runs an STFT that continuously learns the noise floor in every frequency bin during quiet passages (a fast-fall, slow-rise tracker, so speech is never mistaken for noise), then subtracts it back out with a spectral safety floor plus frequency- and time-axis smoothing to avoid "musical noise" artifacts. This is a classical DSP denoiser, not a trained neural network — it excels at steady noise (hiss, hum, fans, room tone) but won't isolate one-off transient noises the way a model trained specifically for that can. |
| **Low Cut**  | 24 dB/oct high-pass, 20 Hz – 500 Hz. Fully anticlockwise is **OFF**. |
| **High Cut** | 24 dB/oct low-pass, 2 kHz – 22 kHz. Fully clockwise is **OFF**. |

### Tone row

| Control      | What it does |
|--------------|--------------|
| **Low**      | ±12 dB. The **PEAK / SHELF** lever switches the band's shape. |
| **Low Freq** | 40 Hz – 320 Hz corner/centre for the low band. |
| **Mid**      | ±12 dB. |
| **Tone**     | Sweeps the mid band from body (350 Hz) up to bite (5 kHz). |
| **High**     | ±12 dB, with three voicings: **AIR** (11 kHz shelf), **BRIGHT** (6.5 kHz shelf), **PRESENCE** (3.8 kHz bell). |
| **De-Ess**   | Split-band sibilance control. Threshold and ratio move together, so one knob covers gentle polish through to hard taming. The bar beside it shows how hard it's working. |
| **Freq**     | De-esser crossover, 3 kHz – 12 kHz. |

### Dynamics row

| Control       | What it does |
|---------------|--------------|
| **Control**   | The main compressor. **AGGRO** is fast and obvious, **SMOOTH** is opto-style and forgiving, **VOCAL** sits between them. |
| **Push**      | Parallel density — a hard, fast compressor with drive blended against the dry signal. It raises the floor without flattening the transients Control is shaping. **PUNCH** keeps consonants intact, **FAT** maximises sustain, **TIGHT** is aggressive and controlled. |
| **-3 / -6 / -9** | A hard ceiling on how much gain reduction that stage may apply. This is what keeps the compressors musical when you push the amount knob: the detector works harder, but the gain reduction never exceeds the range you selected. |
| **Saturate**  | Harmonic saturation, running inside the oversampled section. **RICH** (asymmetric tanh, forward), **WARM** (soft cubic, rounded), **TAPE** (level-dependent squash + HF loss), **TUBE** (heavy even harmonics). Output is level-compensated, so the knob changes character rather than loudness. |
| **Output**    | ±24 dB trim, applied *before* the limiter. |
| **Mix**       | Dry/wet across the whole strip. |

### Right column

| Control          | What it does |
|------------------|--------------|
| **Out meter**    | Stereo peak with 1-second peak-hold. |
| **Limit**        | Look-ahead brickwall limiter with a 1.5 ms window. The thin bar shows gain reduction; the fader next to it sets the ceiling. |
| **Oversampling** | **OFF / 2X / 4X / 8X** for the Push and Saturate stages. Higher settings fold back less aliasing at the cost of CPU and a little latency. |

Reported latency (limiter look-ahead plus any oversampling) is published to the host,
so delay compensation is handled automatically.

## Presets

Ten factory presets ship in the plugin: *Default, Modern Pop Lead, Warm Soul Vocal,
Rap / Trap Vocal, Airy Backing Stack, Rock Shout, Podcast / Voice Over, Vintage Tape
Vocal, Aggressive Ad-Lib, Gentle Polish.*

**SAVE** writes a user preset as XML to:

```
C:\Users\Public\Documents\Zinox Audio\Zinox Vocals\Presets\
```

User presets appear in the same dropdown as the factory ones, and the `<` `>` arrows
step through the whole list.

---

## Building

### Prerequisites

- **CMake** 3.22 or newer
- **Windows:** Visual Studio 2022 (or the Build Tools) with the *Desktop development
  with C++* workload
- **macOS:** Xcode command line tools
- **Git** (CMake fetches JUCE 8.0.6 automatically — no manual download)

Install on Windows with winget:

```powershell
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Open a **new** terminal afterwards so `cmake` lands on your PATH.

### Build

```powershell
cd C:\Users\opeye\ZinoxVocals
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Or just run `.\build.ps1`.

The first configure clones JUCE, so it takes a few minutes. Later builds are fast.

### Where the plugin lands

`COPY_PLUGIN_AFTER_BUILD` is on, so a Release build installs itself:

| Format     | Path |
|------------|------|
| VST3       | `C:\Program Files\Common Files\VST3\Zinox Vocals.vst3` |
| Standalone | `build\ZinoxVocals_artefacts\Release\Standalone\Zinox Vocals.exe` |
| AU (macOS) | `~/Library/Audio/Plug-Ins/Components/` |

Copying to `Program Files` needs an elevated terminal. If you'd rather not elevate,
set `COPY_PLUGIN_AFTER_BUILD FALSE` in [CMakeLists.txt](CMakeLists.txt) and copy the
`.vst3` folder from `build\ZinoxVocals_artefacts\Release\VST3\` yourself.

Run the **Standalone** build first — it's the quickest way to hear the strip without
opening a DAW.

---

## Layout of the source

| Path | Contents |
|------|----------|
| [Source/Parameters.h](Source/Parameters.h) / [.cpp](Source/Parameters.cpp) | Parameter IDs, ranges and value formatting — the single source of truth |
| [Source/PluginProcessor.cpp](Source/PluginProcessor.cpp) | Signal chain, oversampling, latency reporting, metering |
| [Source/PluginEditor.cpp](Source/PluginEditor.cpp) | Front panel layout and painting |
| [Source/PresetManager.cpp](Source/PresetManager.cpp) | Factory snapshots and user preset files |
| [Source/DSP/](Source/DSP/) | `Denoiser`, `ToneStack`, `DeEsser`, `Compressor`, `Push`, `Saturator`, `Limiter`, `Envelope` |
| [Source/GUI/](Source/GUI/) | `Theme` (palette), `ZinoxLookAndFeel` (drawing), `Widgets` (knobs, meters, switches) |

### Real-time safety

The audio thread never allocates. IIR coefficient construction — the one place JUCE
allocates behind your back — is guarded by change detection in
[ToneStack](Source/DSP/ToneStack.h) and [DeEsser](Source/DSP/DeEsser.h), and the
oversampled stages are prepared at the 8x worst case in `prepareToPlay` so switching
oversampling at runtime never resizes a buffer.

---

## Notes

Zinox Vocals is an independent, from-scratch implementation. It is not affiliated
with, endorsed by, or derived from Black Salt Audio or any of their products, and it
contains none of their code, presets, or assets.
