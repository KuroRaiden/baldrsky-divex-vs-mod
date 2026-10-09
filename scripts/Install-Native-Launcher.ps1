param([string]$GameDirectory='E:\GIGA\BALDRSKY_DIVEX')
$ErrorActionPreference='Stop'
$game=Join-Path $GameDirectory 'BaldrSky.VSNative.exe'
if(@(Get-Process | Where-Object {$_.ProcessName -like 'BaldrSky*' -or $_.ProcessName -eq 'BaldrVSLauncher.Native'}).Count){throw 'Close the game and native launcher before installing'}
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'SHA256.json') -Raw | ConvertFrom-Json
foreach($file in $manifest){if((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $file.File)).Hash -ne $file.Hash){throw "Package mismatch: $($file.File)"}}
if(!(Test-Path -LiteralPath $game)){
 $source=Join-Path $GameDirectory 'BaldrSky.VSPrototype.v11c.exe'
 & (Join-Path $PSScriptRoot 'build_native_game.ps1') -SourcePath $source -OutputPath $game
}
if((Get-FileHash -LiteralPath $game).Hash -ne '0B34948AF88F3CFD37FA01DE80C863114306F8C18C16BC650ED31F1E7F092670'){throw 'Unexpected native game output; preserving it'}
foreach($name in @('BaldrVSLauncher.Native.exe','BaldrVSPlugin.dll')){Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $GameDirectory $name)}
'Installed native launcher/plugin beside the separate native mod game. Original builds preserved.'
