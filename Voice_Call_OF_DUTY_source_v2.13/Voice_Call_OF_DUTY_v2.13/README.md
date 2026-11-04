# Voice_Call_OF_DUTY — Windows x64 VST3

Standalone VST3 audio effect for Wave Link 3. The project uses JUCE 8.0.6 and CMake.

## Build without installing Visual Studio locally

A GitHub Actions workflow is included at `.github/workflows/windows-vst3.yml`.

1. Upload the contents of this folder to a GitHub repository (include the hidden `.github` folder).
2. Open **Actions → Build Windows x64 VST3 → Run workflow**.
3. After a successful run, download `Voice-Call-Of-Duty-Windows-x64-VST3` from the run's Artifacts section.

See [BUILD_WINDOWS.md](BUILD_WINDOWS.md) for details and local build instructions.

## Included effects

- RADIO
- REVERB
- ECHO / DELAY

No hotkeys, FIFINE D6 bindings, Stream Deck bindings, Sample engine, or Environment engine are included.

## Verification status

`python tools/verify_source.py` performs static source checks only. It does not compile the plugin. A successful GitHub Actions run confirms Windows x64 compilation and bundle packaging; loading and listening tests in Wave Link 3 remain separate steps.
