param(
 [string]$SourcePath=(Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v11b.exe'),
 [string]$OutputPath=(Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v11b.stages.exe')
)
$ErrorActionPreference='Stop'
$expected='08EDA99FF0CEB12118EF3760A5E7B3B79B98290FCFC07A41BD36C81D333B9C0A'
if((Get-FileHash -LiteralPath $SourcePath).Hash -ne $expected){throw 'Expected exact verified V11b source'}
if(Test-Path -LiteralPath $OutputPath){throw 'Output already exists; preserving it'}
$original=[IO.File]::ReadAllBytes($SourcePath)
$bytes=[byte[]]$original.Clone()
$pe=[BitConverter]::ToInt32($bytes,0x3C)
$opt=$pe+24
$base=[BitConverter]::ToUInt32($bytes,$opt+28)
$sections=$opt+[BitConverter]::ToUInt16($bytes,$pe+20)
$count=[BitConverter]::ToUInt16($bytes,$pe+6)
$va=0x4E0867
$rva=$va-$base
$offset=-1
for($n=0;$n -lt $count;$n++){
 $sh=$sections+40*$n
 $start=[BitConverter]::ToUInt32($bytes,$sh+12)
 $size=[BitConverter]::ToUInt32($bytes,$sh+16)
 if($rva -ge $start -and $rva -lt $start+$size){$offset=[BitConverter]::ToUInt32($bytes,$sh+20)+$rva-$start;break}
}
if($offset -lt 0){throw 'Stage selector call is unmapped'}
if([BitConverter]::ToString($original,$offset,5) -ne 'E8-14-04-F7-FF'){throw 'Unexpected Training stage predicate'}
# The original ownership predicate is thiscall retn 4. Return AL=1 while
# consuming its one stack argument; leave the global save predicate intact.
[byte[]]$patch=0xB0,0x01,0x83,0xC4,0x04
[Array]::Copy($patch,0,$bytes,$offset,5)
$changed=@()
for($i=0;$i -lt $bytes.Length;$i++){
 if($bytes[$i] -ne $original[$i]){
  if($i -lt $offset -or $i -ge $offset+5){throw 'Unexpected modification'}
  $changed+=$i
 }
}
if($bytes.Length -ne $original.Length -or $changed.Count -ne 5){throw 'Length or diff validation failed'}
if((Get-FileHash -LiteralPath $SourcePath).Hash -ne $expected){throw 'Source changed during build'}
[IO.File]::WriteAllBytes($OutputPath,$bytes)
$manifest=[pscustomobject]@{
 SourceSHA256=$expected;OutputSHA256=(Get-FileHash -LiteralPath $OutputPath).Hash
 SourcePath=$SourcePath;OutputPath=$OutputPath;Length=$bytes.Length;ChangedBytes=$changed.Count
 FileOffset=('0x{0:X}' -f $offset);VirtualAddress='0x4E0867'
 Before='E8-14-04-F7-FF';After='B0-01-83-C4-04'
 Scope='Training CSelectMapItem rendering/selection only; native valid-map list preserved'
 RuntimeValidation='Pending; no save files modified'
}
$manifest|ConvertTo-Json|Set-Content -LiteralPath ($OutputPath+'.json') -Encoding utf8
$manifest|ConvertTo-Json
