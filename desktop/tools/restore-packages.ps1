$ErrorActionPreference = 'Stop'

$root  = Split-Path -Parent $PSScriptRoot
$tools = Join-Path $env:TEMP 'opencode\tools'
$nuget = Join-Path $tools 'nuget.exe'
$version = '1.8.260804001'

if (-not (Test-Path $nuget)) {
    New-Item -ItemType Directory -Force -Path $tools | Out-Null
    Write-Host "Downloading nuget.exe..."
    Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile $nuget
}

Write-Host "Restoring Microsoft.WindowsAppSDK $version..."
& $nuget install Microsoft.WindowsAppSDK -Version $version -OutputDirectory (Join-Path $root 'packages') -NonInteractive
Write-Host "Done."
