param([string]$SourcePath='E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v2.exe')
$ErrorActionPreference='Stop'
$expected='225ACD27E51D2F54E240148A158084D66D41EA0C6A82155CEB1D352020392A3F'
if((Get-FileHash $SourcePath -Algorithm SHA256).Hash -ne $expected){throw 'Expected verified v2 source'}
$original=[IO.File]::ReadAllBytes($SourcePath)
$bytes=[byte[]]$original.Clone()
$baseVA=0xCBE030
$raw=0x46F430
$code=[Collections.Generic.List[byte]]::new()
$labels=@{}
$fixups=[Collections.Generic.List[object]]::new()
function Emit([byte[]]$v){$code.AddRange($v)}
function Label([string]$name){$labels[$name]=$code.Count}
function Branch([byte]$condition,[string]$name){Emit @(0x0F,$condition);$fixups.Add(@($code.Count,$name));Emit @(0,0,0,0)}
function Transfer([byte]$opcode,[long]$to){$at=$baseVA+$code.Count;Emit @($opcode);Emit ([BitConverter]::GetBytes([int]($to-$at-5)))}
function RenderOriginal {Emit @(0xFF,0x75,0x10,0xFF,0x75,0x0C,0xFF,0x75,0x08,0x8B,0xCE);Transfer 0xE8 0x4CD9F0}
# Vtable render wrapper: retain P1 pass, then borrow the same ring for a P2 pass.
Emit @(0x55,0x8B,0xEC,0x53,0x56,0x57,0x8B,0xF1)
RenderOriginal
Emit @(0x83,0x3D,0x1C,0xAA,0x86,0,1);Branch 0x85 'done'
Emit @(0x83,0x3D,0x18,0xAA,0x86,0,2);Branch 0x85 'done'
Emit @(0x8B,0x3D,0x54,0xAC,0x86,0,0x85,0xFF);Branch 0x84 'done'
Emit @(0x83,0xBF,0x50,0x18,0,0,0);Branch 0x85 'done'
# Avoid drawing twice if this ring is already owned by P2.
Emit @(0x39,0xBE,0x08,1,0,0);Branch 0x84 'done'
# Save borrowed owner, position, lock-state byte (as dword), and four UI flags.
foreach($offset in @(0x108,0xA0,0xA4,0x120)){Emit @(0xFF,0xB6);Emit ([BitConverter]::GetBytes([int]$offset))}
foreach($address in @(0x872F5C,0x872F60,0x872F64,0x872F68)){Emit @(0xFF,0x35);Emit ([BitConverter]::GetBytes([int]$address))}
Emit @(0x89,0xBE,0x08,1,0,0)
# Recompute world-space center from P2, using the original ring constructor math.
Emit @(0x8B,0xCF,0x8B,0x07,0xFF,0x50,0x4C,0x89,0x86,0xA0,0,0,0)
Emit @(0x8B,0xCF,0x8B,0x07,0xFF,0x50,0x50,0x8B,0x97,0x08,0x16,0,0,0xD1,0xFA,0x03,0x97,0xA8,0,0,0,0x2B,0xC2,0x89,0x86,0xA4,0,0,0)
Emit @(0x8B,0xCF);Transfer 0xE8 0x618170
Emit @(0x88,0x86,0x20,1,0,0)
# P1's cached target name is not P2's; omit that text and duplicate statistics.
foreach($address in @(0x872F5C,0x872F60,0x872F64,0x872F68)){Emit @(0x80,0x25);Emit ([BitConverter]::GetBytes([int]$address));Emit @(0xFB)}
RenderOriginal
foreach($address in @(0x872F68,0x872F64,0x872F60,0x872F5C)){Emit @(0x8F,0x05);Emit ([BitConverter]::GetBytes([int]$address))}
foreach($offset in @(0x120,0xA4,0xA0,0x108)){Emit @(0x8F,0x86);Emit ([BitConverter]::GetBytes([int]$offset))}
Label 'done'
Emit @(0x5F,0x5E,0x5B,0x5D,0xC2,0x0C,0)
$colorVA=$baseVA+$code.Count
# Hook only this ring's sprite draw. EBP is the original render's ring pointer.
Emit @(0x9C,0x50,0xA1,0x54,0xAC,0x86,0,0x85,0xC0);Branch 0x84 'color_done'
Emit @(0x39,0x85,0x08,1,0,0);Branch 0x85 'color_done'
# With saved flags/EAX, original last argument is at ESP+32.
Emit @(0xC7,0x44,0x24,0x20,0xFF,0x80,0xFF,0)
Label 'color_done'
Emit @(0x58,0x9D);Transfer 0xE9 0x70F800
foreach($fix in $fixups){[Array]::Copy([BitConverter]::GetBytes([int]($labels[$fix[1]]-$fix[0]-4)),0,($temp= $code.ToArray()),$fix[0],4);for($j=0;$j -lt 4;$j++){$code[$fix[0]+$j]=$temp[$fix[0]+$j]}}
if($code.Count -gt (512-48)){throw 'Helpers exceed v2 section padding'}
for($i=0;$i -lt $code.Count;$i++){if($original[$raw+$i] -ne 0){throw 'Occupied padding'}}
[Array]::Copy($code.ToArray(),0,$bytes,$raw,$code.Count)
$pe=[BitConverter]::ToInt32($original,0x3C)
$sections=$pe+24+[BitConverter]::ToUInt16($original,$pe+20)
$rdata=$sections+40
$tableOffset=0x7E0E70-0x400000-[BitConverter]::ToUInt32($original,$rdata+12)+[BitConverter]::ToUInt32($original,$rdata+20)
if([BitConverter]::ToUInt32($original,$tableOffset) -ne 0x4CD9F0){throw 'Unexpected range-ring vtable'}
[Array]::Copy([BitConverter]::GetBytes([uint32]$baseVA),0,$bytes,$tableOffset,4)
$colorOffset=0x4CDE28-0x400C00
if([BitConverter]::ToString($original,$colorOffset,5) -ne 'E8-D3-19-24-00'){throw 'Unexpected ring sprite draw call'}
$bytes[$colorOffset]=0xE8
[Array]::Copy([BitConverter]::GetBytes([int]($colorVA-0x4CDE2D)),0,$bytes,$colorOffset+1,4)
[Array]::Copy([BitConverter]::GetBytes([uint32](48+$code.Count)),0,$bytes,0x2A0,4)
$allowed=@(@($tableOffset,4),@($colorOffset,5),@(0x2A0,4),@($raw,$code.Count))
$diff=0
for($i=0;$i -lt $bytes.Length;$i++){if($original[$i] -ne $bytes[$i]){$valid=$false;foreach($r in $allowed){if($i -ge $r[0] -and $i -lt $r[0]+$r[1]){$valid=$true;break}};if(!$valid){throw 'Unexpected change'};$diff++}}
$output=Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v3.exe'
if(Test-Path $output){throw 'Preserve existing output before rebuilding'}
[IO.File]::WriteAllBytes($output,$bytes)
[pscustomobject]@{SourceSHA256=$expected;OutputSHA256=(Get-FileHash $output -Algorithm SHA256).Hash;Length=$bytes.Length;ChangedBytes=$diff;HelperLength=$code.Count;RenderVA=('0x{0:X}' -f $baseVA);ColorVA=('0x{0:X}' -f $colorVA);HelperBytes=[BitConverter]::ToString($code.ToArray());RuntimeValidation='Pending'}|ConvertTo-Json
