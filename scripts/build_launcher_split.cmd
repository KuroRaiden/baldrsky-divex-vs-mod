@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "ROOT=%~dp0.."
set "IMGUI=%~1"
set "HELPER=BaldrVSSetup.SplitScreen.dll"
set "LAUNCHER=BaldrVSLauncher.SplitScreen.exe"
set "VARIANT="
if "%~2"=="v2" (
set "HELPER=BaldrVSSetup.SplitScreen.v2.dll"
set "LAUNCHER=BaldrVSLauncher.SplitScreen.v2.exe"
set "VARIANT=/DVS_SPLIT_SCREEN_V2"
)
if not exist "%IMGUI%\imgui.cpp" exit /b 2
if not exist "%ROOT%\launcher-split" mkdir "%ROOT%\launcher-split"
pushd "%ROOT%\launcher-split"
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_SPLIT_SCREEN %VARIANT% /LD "%ROOT%\src\vs_inject.cpp" /link /OUT:%HELPER% /EXPORT:InitializeVS=_InitializeVS@4 kernel32.lib
if errorlevel 1 exit /b 3
powershell -NoProfile -File "%ROOT%\scripts\launcher_helper_hash.ps1" -Helper "launcher-split\%HELPER%" -Header "src\launcher_helper_hash_split.h"
if errorlevel 1 exit /b 4
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_SPLIT_SCREEN %VARIANT% /DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD /I"%IMGUI%" /I"%IMGUI%\backends" "%ROOT%\src\vs_launcher_menu.cpp" "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" /link /SUBSYSTEM:WINDOWS /OUT:%LAUNCHER% d3d11.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib advapi32.lib
if errorlevel 1 exit /b 5
cl /nologo /O2 /MT /EHsc /std:c++17 "%ROOT%\src\test_split_screen.cpp" /link /OUT:test_split_screen.exe
if errorlevel 1 exit /b 6
test_split_screen.exe
if errorlevel 1 exit /b 7
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_SPLIT_SCREEN %VARIANT% "%ROOT%\src\test_launcher.cpp" /link /OUT:test_launcher.exe
if errorlevel 1 exit /b 8
test_launcher.exe
if errorlevel 1 exit /b 9
popd
endlocal
