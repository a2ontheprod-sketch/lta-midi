# LTA // HARMONY 1.0

**An original MIDI-first VST3 for Windows and macOS.** Inspired by the creative workflow of chord-generation tools, not a reverse-engineered or copied copy of any commercial plugin. Built for FL Studio, underground rap, rage, cloud rap, trap and other styles.

## Features

- Select any of 12 root notes and 7 scales.
- Choose from 12 moods and 14 genres, including UK underground, Rage, Cloud Rap, Drill, Neo Soul, Ambient and Detroit.
- Generate voiced **chords**, melodic **toplines** and matching **basslines**.
- 4, 8 or 16 bars. Tempo, complexity, note density and swing controls.
- Regenerate every part separately; lock parts while you generate a new complete idea.
- Colour-coded MIDI previews, chord names and a simple built-in synth audition while DAW transport runs.
- Drag a lane's **MIDI FILE** to another window if the DAW accepts native file drags; otherwise click the lane's **SAVE .MID** button.
- **EXPORT ALL 4 MIDI FILES** saves chords, topline, bass and a combined multitrack MIDI file with embedded tempo and channel information.
- MIDI output on channels 1 (chords), 2 (topline) and 3 (bass) where the DAW supports VST3 MIDI output.
- Remembers your settings and current idea in your DAW project.
- Runs locally and offline after installation. No account/subscription/AI API required.

The generator is **procedural**, not an AI/ML model. Repeated seeds produce reproducible output; changing the random seeds offers extremely many possible results, not a mathematical guarantee of no repeats. The generated MIDI is original algorithmic output, intended for your personal and commercial music.

## Start here: GitHub (no local programming required)

1. **Extract the `LTA-Harmony-Source.zip` file** on your computer. Do not upload the ZIP as a single repository file: GitHub does not extract it automatically.
2. Create a **new PUBLIC repository** on GitHub named `LTA-Harmony`. Don't add a README or `.gitignore` when creating it, since both are already here.
3. Upload the **contents of the extracted folder**, ensuring the hidden `.github/workflows` folder is included. Alternatively, use GitHub Desktop to publish the folder, or `scripts/PUBLISH_TO_GITHUB.bat` if Git is installed.
4. Click **Actions** on the repository. If GitHub asks to enable workflows, allow them.
5. Select **Build Windows VST3** on the left, click **Run workflow**, choose `main`, then run it. The workflow also runs automatically when source changes are pushed to `main`.
6. Open the completed workflow run and download **LTA-Harmony-Windows-VST3** from the **Artifacts** section near the bottom of the run's page.
7. **Extract the artifact ZIP** and copy the entire `LTA Harmony.vst3` folder to `C:\Program Files\Common Files\VST3\`. See `scripts/INSTALL_WINDOWS.txt`.
8. In FL Studio, go to **Options > Manage plugins > Find installed plugins**, then add **LTA Harmony** as a generator/instrument.

### macOS

Actions > **Build macOS VST3 + AU (manual)** > Run workflow. On success, download the Mac artifact, extract it, and follow `scripts/INSTALL_MAC.txt`. Mac runner availability/quotas depend on your GitHub plan and settings. The Windows workflow is the main path.

### If Actions is missing or the workflow is queued

- **No workflow in Actions?** Your repo doesn't have `.github/workflows/build-windows.yml` on the `main` branch. This hidden folder is crucial. Check the **Code** tab first, then go back to Actions.
- **No Run workflow button?** Check the `main` branch is present and enabled in Settings > Actions; `workflow_dispatch` must be on the default branch.
- **Queued?** GitHub has not assigned a runner. Check for repository quotas/settings at Settings > Actions, and the GitHub Actions status dashboard. Windows public-repository builds are usually easier to obtain than macOS jobs.
- **Workflow failed?** Open the red step in Actions and read the build error; do not download an artifact from an unsuccessful run. Open a GitHub issue with the first full compiler error.
- **No artifact?** Artifacts only appear after the **Upload downloadable Windows plugin** step succeeds. There is no direct compiled ZIP on the Code tab.

## How to use in FL Studio

1. Add **LTA Harmony** as a generator in the Channel Rack. This is an *instrument-style MIDI generator*, not an audio insert effect.
2. Select key, scale, mood and genre; press **GENERATE NEW IDEA**.
3. Each lane has **REMAKE** (generate just that lane), **LOCK** (keep this lane when pressing Generate New Idea), a draggable **DRAG ... MIDI** pad, and a **SAVE .MID** button.
4. To use a lead sound, create your desired instrument in the Channel Rack, then drag the **TOPLINE** MIDI pad onto its piano roll. Repeat with **CHORDS** on a keys/pad instrument and **BASS** on your 808 or bass instrument.
5. If drag/drop is not supported inside your FL Studio version, click **SAVE .MID** for a lane and import the MIDI file into the destination instrument's piano roll using FL Studio's piano-roll MIDI import function. This is the most robust route.
6. Press Play in FL Studio to hear the simple built-in audition synth. Turn audition OFF to suppress preview audio. The MIDI is not locked to the audition sound.
7. Set your DAW tempo to match the BPM selector if you want exported MIDI to align with your existing track; exported MIDI also contains tempo metadata for DAWs that import it.

**Note:** The bundled instrument preview is a simple sine-based sound, deliberately not a full synth. LTA Harmony creates MIDI, so you can use any VST instrument, preset or sampler for the final sound. VST3 MIDI-out support varies by host; direct MIDI drag and file export are the main workflow.

## Build locally

You need CMake 3.22+, a supported C++17 compiler and internet access on first configure to fetch **JUCE 8.0.12**. JUCE has its own licensing requirements; check its licence if you distribute binaries commercially.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --target LTAHarmony_VST3 LTAHarmonyEngineTests --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

The built Windows plugin should be in `build/LTAHarmony_artefacts/Release/VST3/`.

On macOS use the supplied GitHub Actions workflow or Xcode+CMake. For manual builds run `cmake -S . -B build -G Xcode` and build the `LTAHarmony_VST3` and `LTAHarmony_AU` targets.

## Source organisation

- `Source/MusicEngine.*` — algorithmic, scale-aware composition and voice-leading.
- `Source/MidiWriter.*` — standards-compliant MIDI type-1 file writer with tempo and three MIDI channels.
- `Source/PluginProcessor.*` — JUCE audio plugin, playback sync, native MIDI output, sine audition, saved state.
- `Source/PluginEditor.*` — native flat pink/white UI, piano-roll visualisations and drag/export controls.
- `tests/EngineTests.cpp` — exercises every genre, mood and scale combination, checks note boundaries, validates MIDI structure and verifies deterministic seeds.
- `.github/workflows/` — GitHub build and artifact pipelines.

## Licensing / limitations

This project is original example software licensed under MIT (see `LICENSE`). Commercial plugin brands, artwork, source code and trained models are not included. JUCE is separately licensed by its authors. You can use generated musical phrases in tracks; similarity to music created by others is possible with any chord/melody generator.

A **source ZIP is not a compiled plugin**. Builds must complete in GitHub Actions or your local build environment before you can install VST3/AU bundles. The standalone compositional algorithm and MIDI writing have been tested separately, but compile/runtime behaviour in FL Studio and GitHub runners should be validated on your setup.
