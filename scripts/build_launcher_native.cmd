@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "ROOT=%~dp0.."
set "IMGUI=%~1"
if not exist "%IMGUI%\imgui.cpp" exit /b 2
if not exist "%ROOT%\launcher-native\BaldrSky.VSNative.exe" exit /b 3
pushd "%ROOT%\launcher-native"
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_NATIVE_START /DVS_SPLIT_SCREEN /DROUND_NATIVE_PLUGIN /TP /LD "%ROOT%\src\vs_inject.cpp" "%ROOT%\src\vs_rounds_v11c.c" /link /OUT:BaldrVSPlugin.dll /EXPORT:InitializeVS=_InitializeVS@4 /EXPORT:StartVSPlugin=_StartVSPlugin@0 kernel32.lib user32.lib
if errorlevel 1 exit /b 4
powershell -NoProfile -Command "$r='%ROOT%'; $sha=[Security.Cryptography.SHA256]::Create(); $g=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($r+'\launcher-native\BaldrSky.VSNative.exe'))).Replace('-',''); $p=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($r+'\launcher-native\BaldrVSPlugin.dll'))).Replace('-',''); $sha.Dispose(); ('#define VS_NATIVE_GAME_SHA256 '+[char]34+$g+[char]34),('#define VS_NATIVE_PLUGIN_SHA256 '+[char]34+$p+[char]34) | Set-Content -LiteralPath ($r+'\src\launcher_native_hash.h')"
if errorlevel 1 exit /b 5
rc /nologo /I"%ROOT%\assets" /fo launcher_icon.res "%ROOT%\src\launcher.rc"
if errorlevel 1 exit /b 13
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_NATIVE_START /DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD /I"%IMGUI%" /I"%IMGUI%\backends" "%ROOT%\src\vs_launcher_menu.cpp" "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" launcher_icon.res /link /SUBSYSTEM:WINDOWS /OUT:BaldrVSLauncher.Native.exe d3d11.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib advapi32.lib
if errorlevel 1 exit /b 6
cl /nologo /O2 /MT /EHsc /std:c++17 /DVS_NATIVE_START "%ROOT%\src\test_launcher.cpp" /link /OUT:test_launcher.exe
if errorlevel 1 exit /b 7
test_launcher.exe
if errorlevel 1 exit /b 8
cl /nologo /O2 /MT /EHsc /std:c++17 "%ROOT%\src\test_split_screen.cpp" /link /OUT:test_split_screen.exe
if errorlevel 1 exit /b 9
test_split_screen.exe
if errorlevel 1 exit /b 10
cl /nologo /O2 /MT /EHsc /std:c++17 "%ROOT%\src\test_sets.cpp" /link /OUT:test_sets.exe
if errorlevel 1 exit /b 11
test_sets.exe
if errorlevel 1 exit /b 12
popd
endlocal



