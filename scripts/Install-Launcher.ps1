param([string]$GameDirectory='E:\GIGA\BALDRSKY_DIVEX')
$ErrorActionPreference='Stop'
$running=@(Get-Process | Where-Object { $_.ProcessName -like 'BaldrSky*' -or $_.ProcessName -eq 'BaldrVSLauncher' })
if($running.Count){throw 'Close the game and launcher first. Restart Windows if a windowless game process will not exit.'}
$game=Join-Path $GameDirectory 'BaldrSky.VSPrototype.v11c.exe'
$expected='42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460'
if((Get-FileHash -LiteralPath $game -Algorithm SHA256).Hash -ne $expected){throw 'The installed V11c game hash does not match; nothing was changed.'}
$names=@('BaldrVSLauncher.exe','BaldrVSSetup.dll','mechas.tsv','stages.tsv','ImGui-LICENSE.txt')
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'SHA256.json') -Raw | ConvertFrom-Json
foreach($name in $names){
 $source=Join-Path $PSScriptRoot $name
 if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $manifest.$name){throw "Package hash mismatch: $name; nothing was changed."}
}
$backup=Join-Path $GameDirectory ('VSLauncher-backup-'+[DateTime]::Now.ToString('yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach($name in $names){
 $existing=Join-Path $GameDirectory $name
 if(Test-Path -LiteralPath $existing){Copy-Item -LiteralPath $existing -Destination $backup}
}
foreach($name in $names){Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $GameDirectory $name) -Force}
Write-Output "Installed launcher in $GameDirectory. Previous files preserved in $backup. V11c was not modified."
