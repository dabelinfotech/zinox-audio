<#
    Zinox Vocals - build helper

    Usage:
        .\build.ps1                 # Release build
        .\build.ps1 -Config Debug   # Debug build
        .\build.ps1 -Clean          # wipe the build directory first
#>

param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Release',

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$buildDir = Join-Path $root 'build'

# --- prerequisites ---------------------------------------------------------
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "CMake was not found on PATH." -ForegroundColor Red
    Write-Host "Install it with:  winget install Kitware.CMake"
    Write-Host "Then open a NEW terminal and run this script again."
    exit 1
}

$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) {
    Write-Host "Git was not found on PATH - CMake needs it to fetch JUCE." -ForegroundColor Red
    Write-Host "Install it with:  winget install Git.Git"
    exit 1
}

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Removing $buildDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# --- configure -------------------------------------------------------------
Write-Host "`nConfiguring ($Config) ..." -ForegroundColor Cyan
Write-Host "The first run clones JUCE and takes a few minutes.`n"

& cmake -B $buildDir -S $root -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nConfigure failed." -ForegroundColor Red
    Write-Host "If the generator was not found, install the C++ build tools:"
    Write-Host '  winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"'
    exit $LASTEXITCODE
}

# --- build -----------------------------------------------------------------
Write-Host "`nBuilding ($Config) ..." -ForegroundColor Cyan
& cmake --build $buildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

# --- report ----------------------------------------------------------------
$artefacts = Join-Path $buildDir "ZinoxVocals_artefacts\$Config"

Write-Host "`nBuild succeeded." -ForegroundColor Green
Write-Host "Artefacts: $artefacts"

$standalone = Join-Path $artefacts 'Standalone\Zinox Vocals.exe'
if (Test-Path $standalone) {
    Write-Host "`nRun the standalone to hear it straight away:" -ForegroundColor Cyan
    Write-Host "  & `"$standalone`""
}

$installed = 'C:\Program Files\Common Files\VST3\Zinox Vocals.vst3'
if (Test-Path $installed) {
    Write-Host "VST3 installed to: $installed" -ForegroundColor Green
} else {
    Write-Host "`nVST3 was not copied to Program Files (needs an elevated terminal)." -ForegroundColor Yellow
    Write-Host "Copy it manually from: $artefacts\VST3\"
}
