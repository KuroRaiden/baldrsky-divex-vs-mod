param([switch]$AutoPunch,[int]$LagMs=0,[int]$JitterMs=0,[int]$LossPercent=0)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root 'online-build'
$test=Join-Path $build 'test_online.exe'
$token='0123456789abcdef0123456789abcdef'
$endpoint='127.0.0.1:24764';$proxy=$h=$j=$null
try{
if($LagMs -or $JitterMs -or $LossPercent){
 if($LagMs -lt 0 -or $LagMs -gt 1000 -or $JitterMs -lt 0 -or $JitterMs -gt 500 -or $LossPercent -lt 0 -or $LossPercent -gt 50){throw 'Invalid impairment settings'}
 $endpoint='127.0.0.1:24766'
 $python=(Get-Command python -ErrorAction Stop).Source
 $proxy=Start-Process -FilePath $python -ArgumentList @(('"'+(Join-Path $PSScriptRoot 'online_udp_impairment.py')+'"'),'--lag',$LagMs,'--jitter',$JitterMs,'--loss',$LossPercent) -WindowStyle Hidden -PassThru
 [void]$proxy.Handle
 Start-Sleep -Milliseconds 500
}
$code=& $test --make-code $endpoint $token
if($LASTEXITCODE -or !$code){throw 'Link code creation failed'}
$punch=@();if($AutoPunch){$punch=@(('"'+(Join-Path $build 'BaldrAutoPunch.dll')+'"'))}
$hostLog=Join-Path $build 'online-test-host.txt';$joinLog=Join-Path $build 'online-test-join.txt'
$h=Start-Process -FilePath $test -ArgumentList (@('0','24764','-', $token)+$punch) -WindowStyle Hidden -PassThru -RedirectStandardOutput $hostLog -RedirectStandardError (Join-Path $build 'online-test-host-error.txt')
[void]$h.Handle
$j=Start-Process -FilePath $test -ArgumentList (@('1','24765',$code,'link')+$punch) -WindowStyle Hidden -PassThru -RedirectStandardOutput $joinLog -RedirectStandardError (Join-Path $build 'online-test-join-error.txt')
 [void]$j.Handle
 if(!$h.WaitForExit(45000)){throw 'Host test timed out'}
 if(!$j.WaitForExit(5000)){throw 'Join test timed out'}
 Get-Content -LiteralPath $hostLog;Get-Content -LiteralPath $joinLog
 if($h.ExitCode -ne 0 -or $j.ExitCode -ne 0){throw "Two-peer integration failed: $($h.ExitCode), $($j.ExitCode)"}
}finally{foreach($p in @($h,$j,$proxy)){if($p -and !$p.HasExited){Stop-Process -InputObject $p -Force}}}

