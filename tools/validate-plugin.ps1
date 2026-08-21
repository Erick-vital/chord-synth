<#
.SYNOPSIS
    Runs pluginval automated validation on the built ChordSynth VST3 plugin.

.DESCRIPTION
    Validates ChordSynth.vst3 at strictness level 5 using pluginval.
    Ensures plugin bus layout, parameter lifecycle, state saving/restoration,
    and realtime audio processing comply with Steinberg VST3 specifications.

.PARAMETER PluginPath
    Path to the ChordSynth.vst3 bundle or directory.
    Defaults to the standard CMake Windows Release build output.

.PARAMETER StrictnessLevel
    Strictness level for pluginval (1-10). Default is 5.

.PARAMETER PluginvalPath
    Path to the pluginval executable. Default assumes pluginval is in PATH or tools directory.

.EXAMPLE
    .\tools\validate-plugin.ps1
    .\tools\validate-plugin.ps1 -PluginPath "C:\Program Files\Common Files\VST3\ChordSynth.vst3" -StrictnessLevel 8
#>

[CmdletBinding()]
param (
    [Parameter(Position = 0)]
    [string]$PluginPath = "$PSScriptRoot\..\build\windows-msvc-release\ChordSynth_artefacts\Release\VST3\ChordSynth.vst3",

    [Parameter(Position = 1)]
    [ValidateRange(1, 10)]
    [int]$StrictnessLevel = 5,

    [Parameter(Position = 2)]
    [string]$PluginvalPath = "pluginval"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ChordSynth VST3 Validation via pluginval" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Resolve pluginval binary
$resolvedPluginval = $null
if (Get-Command $PluginvalPath -ErrorAction SilentlyContinue) {
    $resolvedPluginval = $PluginvalPath
} elseif (Test-Path "$PSScriptRoot\pluginval.exe") {
    $resolvedPluginval = "$PSScriptRoot\pluginval.exe"
} elseif (Test-Path "C:\Program Files\pluginval\pluginval.exe") {
    $resolvedPluginval = "C:\Program Files\pluginval\pluginval.exe"
}

if (-not $resolvedPluginval) {
    Write-Error "pluginval executable not found. Please install pluginval and ensure it is in your PATH or under tools\."
    exit 1
}

if (-not (Test-Path $PluginPath)) {
    Write-Error "Plugin target not found at: $PluginPath`nPlease compile the Release target first via CMake/MSBuild."
    exit 1
}

Write-Host "Pluginval:       $resolvedPluginval"
Write-Host "Plugin:          $PluginPath"
Write-Host "Strictness:      $StrictnessLevel"
Write-Host "------------------------------------------"

$argsList = @(
    "--strictness-level", $StrictnessLevel,
    "--validate", "`"$PluginPath`"",
    "--output-dir", "$PSScriptRoot\..\docs\validation\logs",
    "--verbose"
)

# Ensure logs directory exists
$logDir = "$PSScriptRoot\..\docs\validation\logs"
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
}

$startTime = Get-Date
$process = Start-Process -FilePath $resolvedPluginval -ArgumentList $argsList -NoNewWindow -Wait -PassThru
$exitCode = $process.ExitCode
$elapsed = (Get-Date) - $startTime

Write-Host "------------------------------------------"
if ($exitCode -eq 0) {
    Write-Host "Validation PASSED (Exit Code 0) in $($elapsed.TotalSeconds.ToString('F2'))s" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Validation FAILED (Exit Code $exitCode) in $($elapsed.TotalSeconds.ToString('F2'))s" -ForegroundColor Red
    exit $exitCode
}
