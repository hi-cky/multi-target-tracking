$ErrorActionPreference = "Stop"

$msysBin = "D:\msys64\ucrt64\bin"
if (-not (Test-Path $msysBin)) {
  throw "MSYS2 ucrt64 bin not found at $msysBin. Edit run_demo_visualizer.ps1 to your MSYS2 install path."
}

$qtCore = Join-Path $msysBin "Qt6Core.dll"
if (-not (Test-Path $qtCore)) {
  Write-Host "Qt6 runtime not found. Install (in MSYS2): pacman -S --needed mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat"
}

$env:PATH = "$msysBin;$env:PATH"

$qtPlugins = "D:\msys64\ucrt64\share\qt6\plugins"
if (Test-Path $qtPlugins) {
  $env:QT_PLUGIN_PATH = $qtPlugins
  $env:QT_QPA_PLATFORM_PLUGIN_PATH = (Join-Path $qtPlugins "platforms")
}

Set-Location $PSScriptRoot
& ".\build_msys\demo_visualizer.exe"
