param(
 [Parameter(Mandatory=$true)][string]$GekkoPath,
 [Parameter(Mandatory=$true)][string]$AutoPunchPath,
 [Parameter(Mandatory=$true)][string]$ImGuiPath,
 [string]$NativeBuild
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'online-build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$GekkoPath=(Resolve-Path $GekkoPath).Path;$AutoPunchPath=(Resolve-Path $AutoPunchPath).Path;$ImGuiPath=(Resolve-Path $ImGuiPath).Path
foreach($pair in @(@($GekkoPath,'9da2363d7c5a68cc77f8dfe2da7cf51d3afe3bee'),@($AutoPunchPath,'db0b962a8bed74179997e755e851f99e885a10d1'))){
 $revision=& git -C $pair[0] rev-parse HEAD
 if($LASTEXITCODE -or $revision -ne $pair[1]){throw 'Dependency revision does not match the audited online build'}
}
if(!$NativeBuild){$NativeBuild=Join-Path $root 'launcher-native'}
$NativeBuild=(Resolve-Path $NativeBuild).Path
foreach($name in @('BaldrVSPlugin.dll','BaldrSky.VSNative.exe','BaldrVSLauncher.Native.exe')){
 if(!(Test-Path -LiteralPath (Join-Path $NativeBuild $name))){throw "Missing native build: $name"}
}
# Preserve the exact stable pair; the online prototype cannot change combat.
$pluginHash=(Get-FileHash -LiteralPath (Join-Path $NativeBuild 'BaldrVSPlugin.dll') -Algorithm SHA256).Hash
$gameHash=(Get-FileHash -LiteralPath (Join-Path $NativeBuild 'BaldrSky.VSNative.exe') -Algorithm SHA256).Hash
@("#define VS_NATIVE_GAME_SHA256 `"$gameHash`"", "#define VS_NATIVE_PLUGIN_SHA256 `"$pluginHash`"") | Set-Content -LiteralPath (Join-Path $root 'src/launcher_native_hash.h')
# Upstream AutoPunch starts network detours under DllMain and waits on its
# mutex during unload. Start explicitly after Windows has finished loading;
# retain it until process termination. No remote-process injector is built.
$inject=[IO.File]::ReadAllText((Join-Path $AutoPunchPath 'autopunch-inject/inject.c'))
$start=$inject.IndexOf('BOOL WINAPI DllMain(')
if($start -lt 0){throw 'AutoPunch entry point changed'}
$inject=$inject.Substring(0,$start)+@'
BOOL WINAPI StartAutoPunch(void) {
 static BOOL initialized=FALSE;
 if(initialized)return TRUE;
 WSADATA ws; if(WSAStartup(MAKEWORD(2,2), &ws))return FALSE;
 load(); initialized=started ? TRUE : FALSE; return initialized;
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
 if(reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(module);
 return TRUE;
}
'@
[IO.File]::WriteAllText((Join-Path $out 'autopunch_start.c'),$inject)
& cmake -S (Join-Path $root 'online') -B (Join-Path $out 'cmake') -G 'Visual Studio 17 2022' -A Win32 "-DGEKKO_PATH=$GekkoPath" "-DAUTOPUNCH_PATH=$AutoPunchPath" "-DIMGUI_PATH=$ImGuiPath" "-DONLINE_OUT=$out"
if($LASTEXITCODE){throw 'Online configuration failed'}
& cmake --build (Join-Path $out 'cmake') --config Release --parallel 1
if($LASTEXITCODE){throw 'Online build failed'}
foreach($name in @('BaldrVSPlugin.dll','BaldrSky.VSNative.exe')){Copy-Item -LiteralPath (Join-Path $NativeBuild $name) -Destination $out -Force}
Copy-Item -LiteralPath (Join-Path $GekkoPath 'LICENSE') -Destination (Join-Path $out 'GekkoNet-LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $AutoPunchPath 'LICENSE') -Destination (Join-Path $out 'AutoPunch-LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $AutoPunchPath 'autopunch-inject/detours/LICENSE.md') -Destination (Join-Path $out 'Detours-LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $ImGuiPath 'LICENSE.txt') -Destination (Join-Path $out 'ImGui-LICENSE.txt') -Force
& (Join-Path $out 'test_online.exe')
if($LASTEXITCODE){throw 'Online tests failed'}
& (Join-Path $out 'test_online_chat.exe')
if($LASTEXITCODE){throw 'Online chat tests failed'}

$catalogInfo=[Diagnostics.ProcessStartInfo]::new((Join-Path $out 'BaldrVSLauncherOnline.exe'),'--verify-catalogs')
$catalogInfo.UseShellExecute=$false;$catalogInfo.CreateNoWindow=$true
$catalogTest=[Diagnostics.Process]::Start($catalogInfo)
if(!$catalogTest.WaitForExit(10000)){Stop-Process -InputObject $catalogTest -Force;throw 'Embedded catalog test timed out'}
if($catalogTest.ExitCode -ne 0){throw 'Embedded roster/stage/music resource test failed'}
$catalogTest.Dispose()
'PASS: embedded 99-mecha, 220-stage and 92-track catalogs; no external catalog required.'
