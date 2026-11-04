# Build Voice_Call_OF_DUTY — Windows x64

## Cloud build (no Visual Studio installation needed on your PC)

This source archive includes `.github/workflows/windows-vst3.yml`. GitHub Actions runs the build on a hosted Windows Server 2022 machine with MSVC x64 and downloads JUCE 8.0.6 through CMake FetchContent.

1. Create a repository on GitHub (a private repository is fine).
2. Upload the **contents** of this source folder to the repository root. Make sure the `.github/workflows/windows-vst3.yml` file is included; hidden `.github` folders must not be omitted.
3. Open the repository's **Actions** tab.
4. Select **Build Windows x64 VST3** in the workflow list.
5. Click **Run workflow** and confirm.
6. When the run finishes, open it and download the artifact named `Voice-Call-Of-Duty-Windows-x64-VST3` from the Artifacts section.

A green workflow run means the VST3 bundle was compiled and packaged. It does **not** prove that Wave Link 3 loads it or that audio sounds correct; that still needs a host test.

## Local build

Requirements: Visual Studio 2022 with **Desktop development with C++**, Windows SDK, CMake 3.22+, and Git/network access for the pinned JUCE 8.0.6 checkout.

Run `build_windows.bat`, or from an x64 Native Tools PowerShell:

```powershell
.\build_windows.ps1
```

If JUCE is already available locally:

```powershell
.\build_windows.ps1 -JuceDir "C:\Libraries\JUCE"
```

Expected output: `build\Voice_Call_OF_DUTY_artefacts\Release\VST3\Voice Call Of Duty.vst3`.

## Important

This workflow definition and source archive are not themselves a compiled plugin. A compiled artifact exists only after the GitHub Actions run succeeds. Wave Link 3 compatibility and audio quality must then be tested in the host.
