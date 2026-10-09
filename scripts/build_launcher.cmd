@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "ROOT=%~dp0.."
set "IMGUI=%~1"
if not exist "%IMGUI%\imgui.cpp" exit /b 2
if not exist "%ROOT%\launcher" mkdir "%ROOT%\launcher"
copy /y "%ROOT%\data\mechas.tsv" "%ROOT%\launcher\mechas.tsv" >nul
if errorlevel 1 exit /b 9
copy /y "%ROOT%\data\stages.tsv" "%ROOT%\launcher\stages.tsv" >nul
if errorlevel 1 exit /b 9
copy /y "%ROOT%\docs\ImGui-LICENSE.txt" "%ROOT%\launcher\ImGui-LICENSE.txt" >nul
if errorlevel 1 exit /b 9
pushd "%ROOT%\launcher"
cl /nologo /O2 /MT /EHsc /std:c++17 /LD "%ROOT%\src\vs_inject.cpp" /link /OUT:BaldrVSSetup.dll /EXPORT:InitializeVS=_InitializeVS@4 kernel32.lib
if errorlevel 1 exit /b 5
powershell -NoProfile -File "%ROOT%\scripts\launcher_helper_hash.ps1"
if errorlevel 1 exit /b 6
cl /nologo /O2 /MT /EHsc /std:c++17 /I"%IMGUI%" /I"%IMGUI%\backends" "%ROOT%\src\vs_launcher_menu.cpp" "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" /link /SUBSYSTEM:WINDOWS /OUT:BaldrVSLauncher.exe d3d11.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib advapi32.lib
if errorlevel 1 exit /b 3
cl /nologo /O2 /MT /EHsc /std:c++17 "%ROOT%\src\test_launcher.cpp" /link /OUT:test_launcher.exe
if errorlevel 1 exit /b 7
test_launcher.exe
if errorlevel 1 exit /b 8
popd
endlocal
