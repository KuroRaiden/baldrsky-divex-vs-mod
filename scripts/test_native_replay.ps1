param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'online-build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if((Get-FileHash -LiteralPath (Join-Path $game 'BaldrSky.VSNative.exe') -Algorithm SHA256).Hash -ne '0B34948AF88F3CFD37FA01DE80C863114306F8C18C16BC650ED31F1E7F092670'){throw 'Unsupported native executable'}
$audit=Join-Path $out 'NativeReplayAudit.dll'
if(!(Test-Path -LiteralPath $audit)){throw 'Build the explicit NativeReplayAudit target first'}
if(@(Get-Process | Where-Object {$_.ProcessName -like 'BaldrSky*' -or $_.ProcessName -like 'BaldrVSLauncher*'}).Count){throw 'Preserving a running game or launcher'}
$names=@('BaldrVSPlugin.dll','BaldrVS-start.bin','BaldrVS-equipment.bin')
$prior=@{};foreach($name in $names){$path=Join-Path $game $name;$prior[$name]=if(Test-Path -LiteralPath $path){[IO.File]::ReadAllBytes($path)}else{$null}}
$cfg=[uint32[]]@(0x42565331,1,0,1,1,1,0,0,[uint32]::MaxValue);$bytes=New-Object byte[] 36
for($i=0;$i -lt $cfg.Length;$i++){[BitConverter]::GetBytes($cfg[$i]).CopyTo($bytes,4*$i)}
$gear=[Collections.Generic.List[uint32]]::new();$gear.AddRange([uint32[]]@(0x42564531,1,0,1,0));for($i=0;$i -lt 28;$i++){$gear.Add([uint32]::MaxValue-1)}
$loadout=New-Object byte[] 132;for($i=0;$i -lt $gear.Count;$i++){[BitConverter]::GetBytes($gear[$i]).CopyTo($loadout,4*$i)}
try{
 Copy-Item -LiteralPath $audit -Destination (Join-Path $game 'BaldrVSPlugin.dll') -Force
 [IO.File]::WriteAllBytes((Join-Path $game 'BaldrVS-start.bin'),$bytes);[IO.File]::WriteAllBytes((Join-Path $game 'BaldrVS-equipment.bin'),$loadout)
 for($run=1;$run -le 2;$run++){
  $test=Start-Process -FilePath (Join-Path $game 'BaldrSky.VSNative.exe') -ArgumentList '-u' -WorkingDirectory $game -WindowStyle Hidden -PassThru
  [void]$test.Handle
  if(!$test.WaitForExit(30000)){Stop-Process -InputObject $test -Force;throw 'Native replay audit timed out'}
  $trace=Join-Path $game 'online-native-replay-audit.csv'
  if(Test-Path -LiteralPath $trace){Copy-Item -LiteralPath $trace -Destination (Join-Path $out "online-test-native-replay-$run.csv") -Force}
  if($test.ExitCode -ne 0){throw "Native replay audit exited $($test.ExitCode)"}
 }
 $a=Get-Content -LiteralPath (Join-Path $out "online-test-native-replay-1.csv");$b=Get-Content -LiteralPath (Join-Path $out "online-test-native-replay-2.csv")
 if($a.Count -ne 601 -or $b.Count -ne 601){throw 'Unexpected trace length'}
 if(@($a|Where-Object {$_ -like '*,DIFF,*'}).Count -or @($b|Where-Object {$_ -like '*,DIFF,*'}).Count){throw 'Native playback diverged from the recorded combat trace'}
 $different=@();for($i=0;$i -lt $a.Count;$i++){if($a[$i] -ne $b[$i]){$different+=($i-1)}}
 "Native replay audit completed. Two runs of 300 recorded + 300 playback frames. Differences: $($different.Count)"
 if($different.Count){throw "First differing frame: $($different[0])"}else{'PASS: native playback matches both-player input/RNG, restored clock and sampled fighter state.'}
}finally{
 if($test -and !$test.HasExited){Stop-Process -InputObject $test -Force}
 foreach($name in $names){$path=Join-Path $game $name;if($null -ne $prior[$name]){[IO.File]::WriteAllBytes($path,$prior[$name])}elseif(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}}
}
