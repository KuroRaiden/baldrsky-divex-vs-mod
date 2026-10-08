param([string]$BuildDirectory=(Join-Path $env:TEMP ('baldr-round-tests-'+[guid]::NewGuid().ToString('N'))))
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path $BuildDirectory | Out-Null
$src=Join-Path $PSScriptRoot '..\src'
$sdk=Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Lib' -Directory | Sort-Object Name | Select-Object -Last 1
$lib=Join-Path $sdk.FullName 'um\x86\kernel32.lib'
function Check {if($LASTEXITCODE){throw "Build/test failed: $LASTEXITCODE"}}
& clang --target=i686-pc-windows-msvc -O0 -ffreestanding -fno-stack-protector -c (Join-Path $src 'test_round_rules.c') -o (Join-Path $BuildDirectory 'rules.obj');Check
& lld-link /nodefaultlib /entry:entry /subsystem:console "/out:$BuildDirectory\rules.exe" "$BuildDirectory\rules.obj" $lib;Check
& "$BuildDirectory\rules.exe";Check
# Use translated mock addresses so Windows' own low-address stack/heap cannot collide.
& clang --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-stack-protector -fno-builtin -DROUND_ADDRESS_BIAS=0x20000000 -c (Join-Path $src 'vs_rounds_v9.c') -o "$BuildDirectory\rounds.obj";Check
& lld-link /dll /noentry /nodefaultlib /fixed /base:0x40000000 /dynamicbase:no /safeseh:no "/out:$BuildDirectory\rounds.dll" "$BuildDirectory\rounds.obj";Check
& clang --target=i686-pc-windows-msvc -O0 -ffreestanding -fno-stack-protector -c (Join-Path $src 'test_round_integration_v9.c') -o "$BuildDirectory\integration.obj";Check
& lld-link /nodefaultlib /entry:entry /subsystem:console /base:0x10000000 /fixed "/out:$BuildDirectory\integration.exe" "$BuildDirectory\integration.obj" $lib;Check
& "$BuildDirectory\integration.exe";Check
'Scoring and mock integration tests passed. Live game validation remains pending.'


