param([string]$SourcePath = 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.exe')
$ErrorActionPreference = 'Stop'
$expected = '969C4CA6DBD933A2FDD2C075C790B52202D0A357DAC1E5DC493A4D027EFFC502'
if ((Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash -ne $expected) { throw 'Expected verified v1 prototype.' }
$original = [IO.File]::ReadAllBytes($SourcePath)
$pe = [BitConverter]::ToInt32($original,0x3C)
$opt = $pe+24
$count = [BitConverter]::ToUInt16($original,$pe+6)
$section = $opt+[BitConverter]::ToUInt16($original,$pe+20)
$header = $section+40*$count
$fileAlign = [BitConverter]::ToUInt32($original,$opt+36)
$sectionAlign = [BitConverter]::ToUInt32($original,$opt+32)
$imageBase = [BitConverter]::ToUInt32($original,$opt+28)
$rva = [BitConverter]::ToUInt32($original,$opt+56)
function Align([long]$value,[long]$alignment) { [long]([Math]::Ceiling($value/$alignment)*$alignment) }
if ($count -ne 4 -or $fileAlign -ne 512 -or $sectionAlign -ne 4096 -or $imageBase -ne 0x400000) { throw 'Unexpected PE layout.' }
if ($header+40 -gt [BitConverter]::ToUInt32($original,$opt+60)) { throw 'No room for section header.' }
for($i=0;$i -lt 40;$i++){if($original[$header+$i] -ne 0){throw 'Section header padding occupied.'}}
$raw = Align $original.Length $fileAlign
$code = [Collections.Generic.List[byte]]::new()
function Emit([byte[]]$data){$code.AddRange($data)}
function Jump([long]$target){$at=$imageBase+$rva+$code.Count; Emit @(0xE9); Emit ([BitConverter]::GetBytes([int]($target-($at+5))))}
$readVA = $imageBase+$rva
# Preserve the AI direction cache when present; human fighters use +1 if tied.
# Keep flags, all registers except the original EAX/EDX outputs, and stack intact.
Emit @(0x9C,0x8B,0x93,0x3C,0x18,0,0,0x85,0xD2,0x74,0x05,0x8B,0x42,0x28,0xEB,0x05,0xB8,1,0,0,0,0x9D)
Jump 0x63A243
$writeVA = $imageBase+$rva+$code.Count
# Skip only the absent AI cache write; replay displaced mov al,1.
Emit @(0x9C,0x85,0xC9,0x74,0x03,0x89,0x41,0x28,0x9D,0xB0,1)
Jump 0x63A263
$rawSize = Align $code.Count $fileAlign
$bytes = New-Object byte[] ($raw+$rawSize)
[Array]::Copy($original,$bytes,$original.Length)
$changes = [Collections.Generic.List[object]]::new()
function Patch([int]$offset,[string]$before,[byte[]]$after,[string]$reason){
 if([BitConverter]::ToString($original,$offset,$after.Length) -ne $before){throw ('Unexpected bytes at 0x{0:X}' -f $offset)}
 [Array]::Copy($after,0,$bytes,$offset,$after.Length)
 $changes.Add([pscustomobject]@{FileOffset=('0x{0:X}' -f $offset);Before=$before;After=[BitConverter]::ToString($after);Reason=$reason})
}
function Detour([long]$from,[long]$to,[int]$size){[byte[]]$result=@(0xE9)+[BitConverter]::GetBytes([int]($to-($from+5))); while($result.Length -lt $size){$result+=0x90}; return ,$result}
Patch (0x63A233-0x400C00) '8B-93-3C-18-00-00' (Detour 0x63A233 $readVA 6) 'Guard AI direction-cache read'
Patch (0x63A25E-0x400C00) '89-41-28-B0-01' (Detour 0x63A25E $writeVA 5) 'Guard AI direction-cache write'
# Ownership predicate was thiscall retn 4: consume its argument and return AL=1.
Patch (0x4DDD0A-0x400C00) 'E8-91-2E-F7-FF' @(0xB0,1,0x83,0xC4,4) 'Unlock existing Training enemy roster'
function Put32([int]$at,[uint32]$value){[Array]::Copy([BitConverter]::GetBytes($value),0,$bytes,$at,4)}
[Array]::Copy([Text.Encoding]::ASCII.GetBytes('.vsfix'),0,$bytes,$header,6)
Put32 ($header+8) $code.Count
Put32 ($header+12) $rva
Put32 ($header+16) $rawSize
Put32 ($header+20) $raw
Put32 ($header+36) 0x60000020
[Array]::Copy([BitConverter]::GetBytes([uint16]($count+1)),0,$bytes,$pe+6,2)
Put32 ($opt+4) ([BitConverter]::ToUInt32($original,$opt+4)+$rawSize)
Put32 ($opt+56) (Align ($rva+$code.Count) $sectionAlign)
[Array]::Copy($code.ToArray(),0,$bytes,$raw,$code.Count)
# Verify every original-file change is confined to the explicit patches/PE fields.
$allowed=@(@(($pe+6),2),@(($opt+4),4),@(($opt+56),4),@($header,40),@((0x63A233-0x400C00),6),@((0x63A25E-0x400C00),5),@((0x4DDD0A-0x400C00),5))
$diff=0
for($i=0;$i -lt $original.Length;$i++){if($bytes[$i] -ne $original[$i]){ $valid=$false; foreach($range in $allowed){if($i -ge $range[0] -and $i -lt $range[0]+$range[1]){$valid=$true;break}};if(!$valid){throw 'Unexpected modification'};$diff++}}
$output=Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v2.exe'
if(Test-Path -LiteralPath $output){throw 'Output already exists; preserve it before rebuilding.'}
[IO.File]::WriteAllBytes($output,$bytes)
if((Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash -ne $expected){throw 'Source changed during build'}
$manifest=[pscustomobject]@{SourceSHA256=$expected;OutputSHA256=(Get-FileHash $output -Algorithm SHA256).Hash;LengthBefore=$original.Length;LengthAfter=$bytes.Length;ChangedExistingBytes=$diff;Patches=$changes.ToArray();SectionHeader=('0x{0:X}' -f $header);SectionRVA=('0x{0:X}' -f $rva);SectionFileOffset=('0x{0:X}' -f $raw);ReadHelperVA=('0x{0:X}' -f $readVA);WriteHelperVA=('0x{0:X}' -f $writeVA);HelperBytes=[BitConverter]::ToString($code.ToArray());RuntimeValidation='Pending'}
$manifest | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $PSScriptRoot 'vs-v2-manifest.json') -Encoding utf8
$manifest | ConvertTo-Json -Depth 6
