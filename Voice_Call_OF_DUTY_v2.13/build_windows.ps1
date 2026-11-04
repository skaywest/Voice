param(
    [string]$JuceDir = "",
    [string]$BuildDir = "build",
    [string]$Configuration = "Release"
)
$ErrorActionPreference = "Stop"

function Fail([string]$Message) {
    Write-Error $Message
    exit 1
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "CMake not found. Install CMake 3.22 or newer and reopen PowerShell."
}
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Fail "MSVC compiler cl.exe not found. Open 'x64 Native Tools Command Prompt for VS 2022' or install the Desktop development with C++ workload."
}

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
if ([string]::IsNullOrWhiteSpace($JuceDir)) {
    $Candidate = Join-Path $Root "JUCE"
    if (Test-Path (Join-Path $Candidate "CMakeLists.txt")) { $JuceDir = $Candidate }
}

$ConfigureArgs = @("-S", ".", "-B", $BuildDir, "-A", "x64")
if (-not [string]::IsNullOrWhiteSpace($JuceDir)) {
    if (-not (Test-Path (Join-Path $JuceDir "CMakeLists.txt"))) {
        Fail "JUCE_DIR does not contain CMakeLists.txt: $JuceDir"
    }
    $ConfigureArgs += "-DJUCE_DIR=$JuceDir"
}
Write-Host "Configuring Voice_Call_OF_DUTY (Windows x64, $Configuration)..." -ForegroundColor Cyan
& cmake @ConfigureArgs
if ($LASTEXITCODE -ne 0) { Fail "CMake configuration failed (exit $LASTEXITCODE). Check JUCE availability and the messages above." }

Write-Host "Building VST3..." -ForegroundColor Cyan
& cmake --build $BuildDir --config $Configuration --target Voice_Call_OF_DUTY_VST3 --parallel
if ($LASTEXITCODE -ne 0) { Fail "VST3 compilation failed (exit $LASTEXITCODE)." }

$Artifact = Join-Path $Root "$BuildDir\Voice_Call_OF_DUTY_artefacts\$Configuration\VST3\Voice Call Of Duty.vst3"
if (-not (Test-Path $Artifact)) {
    Fail "Build command succeeded but expected VST3 bundle was not found: $Artifact"
}
Write-Host "BUILD SUCCEEDED: $Artifact" -ForegroundColor Green
