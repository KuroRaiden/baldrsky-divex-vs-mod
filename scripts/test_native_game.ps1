param([string]$SourcePath,[string]$OutputPath,[string]$LauncherPath)
$ErrorActionPreference='Stop'
$a=[IO.File]::ReadAllBytes($SourcePath);$b=[IO.File]::ReadAllBytes($OutputPath)
$pe=[BitConverter]::ToInt32($a,0x3c);$opt=$pe+24
$n=[BitConverter]::ToUInt16($a,$pe+6);$section=$opt+[BitConverter]::ToUInt16($a,$pe+20)+40*$n
$allowed=@(@(($pe+6),2),@(($opt+16),4),@(($opt+56),4),@(($opt+64),4),@(($opt+104),8),@($section,40))
for($i=0;$i -lt $a.Length;$i++){
 if($a[$i] -eq $b[$i]){continue}
 $ok=$false;foreach($range in $allowed){if($i -ge $range[0] -and $i -lt $range[0]+$range[1]){$ok=$true;break}}
 if(!$ok){throw ('Original game data modified at {0:X}' -f $i)}
}
function B32($offset){[BitConverter]::ToUInt32($b,$offset)}
if((B32 ($opt+16)) -ne 0x8c3000 -or (B32 ($opt+56)) -ne 0x8c4000){throw 'Bad entry or image size'}
$raw=B32 ($section+20);$size=B32 ($section+16)
if($raw -lt $a.Length -or $raw+$size -ne $b.Length){throw 'Bad new section bounds'}
if($b[$raw] -ne 0x9c -or $b[$raw+1] -ne 0x60 -or $b[$raw+2] -ne 0xff -or $b[$raw+3] -ne 0x15){throw 'Plugin entry call missing'}
if([BitConverter]::ToInt32($b,$raw+15)+0x8c3013 -ne [BitConverter]::ToInt32($a,$opt+16)){throw 'Original entry point not preserved'}
if($LauncherPath){
 $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($LauncherPath))
 foreach($api in @('CreateRemoteThread','WriteProcessMemory','VirtualAllocEx','ReadProcessMemory','SuspendThread')){if($text.Contains($api)){throw "Unexpected injection API $api"}}
}
'PASS: original game data preserved, import/startup section bounds and return path valid, launcher injection APIs absent.'

