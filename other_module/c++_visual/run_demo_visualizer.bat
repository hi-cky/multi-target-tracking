@echo off
setlocal

set "MSYS_BIN=D:\msys64\ucrt64\bin"
if not exist "%MSYS_BIN%" (
  echo MSYS2 ucrt64 bin not found at %MSYS_BIN%.
  echo Please edit run_demo_visualizer.bat to match your MSYS2 install path.
  exit /b 1
)

set "PATH=%MSYS_BIN%;%PATH%"
if not exist "%MSYS_BIN%\Qt6Core.dll" (
  echo Qt6 runtime not found. Install in MSYS2:
  echo   pacman -S --needed mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat
)

set "QT_PLUGIN_PATH=D:\msys64\ucrt64\share\qt6\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=D:\msys64\ucrt64\share\qt6\plugins\platforms"
cd /d "%~dp0"
.\build_msys\demo_visualizer.exe
