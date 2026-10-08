param([string]$SourcePath='E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v4.exe')
$ErrorActionPreference='Stop'
$expected='ABE5F4FBB16697810754BBC2FCE431EE3295FF7B28A8440E9746D69D8982730A'
if((Get-FileHash -LiteralPath $SourcePath).Hash -ne $expected){throw 'Expected exact v4 source'}
$original=[IO.File]::ReadAllBytes($SourcePath)
# Extend the last .vsfix section, without altering the prior helpers.
$bytes=New-Object byte[] ($original.Length+1024)
[Array]::Copy($original,$bytes,$original.Length)
$baseVA=0xCBE200
$code=[Collections.Generic.List[byte]]::new()
$starts=[Collections.Generic.List[long]]::new()
$labels=@{}
$fixups=[Collections.Generic.List[object]]::new()
function I([byte[]]$v){$starts.Add($baseVA+$code.Count);$code.AddRange($v)}
function L([string]$name){$labels[$name]=$code.Count}
function J([byte]$cond,[string]$name){$starts.Add($baseVA+$code.Count);$code.AddRange([byte[]]@(0x0F,$cond));$fixups.Add(@($code.Count,$name));$code.AddRange([byte[]]@(0,0,0,0))}
function T([byte]$op,[long]$dest){$at=$baseVA+$code.Count;I (@($op)+[BitConverter]::GetBytes([int]($dest-$at-5)))}
function Global([byte]$op,[byte]$mod,[uint32]$addr){I (@($op,$mod)+[BitConverter]::GetBytes($addr))}
function TrainingGate([string]$fallback){
 I @(0x83,0x3D,0x1C,0xAA,0x86,0,1);J 0x85 $fallback
 I @(0x83,0x3D,0x18,0xAA,0x86,0,2);J 0x85 $fallback
}
function Counter([int]$offset){
 # ESI points to the global renderer state; EAX is the desired value.
 I (@(0x8D,0x8E)+[BitConverter]::GetBytes($offset))
 I @(0xC7,0x41,4,0,0,0,0)
 I @(0xC7,0x41,8,1,0,0,0)
 I @(0xC7,0x41,12,1,0,0,0)
 I @(0xC7,0x41,20,0,0,0,0)
 I @(0x89,0x41,24)
}
function FighterXY([byte]$reg,[byte]$slot){
 # reg is MOV ECX, EDI/Fighter1 (CF) or EBX/Fighter2 (CB).
 I @(0x8B,$reg);I @(0x8B,1);I @(0xFF,0x50,0x4C);I @(0x89,0x44,0x24,$slot)
 I @(0x8B,$reg);I @(0x8B,1);I @(0xFF,0x50,0x50)
 # Use the same body center as the original camera: ground height + body offset + half height.
 $mem=if($reg -eq 0xCF){0x97}else{0x93}
 I (@(0x8B,$mem)+[BitConverter]::GetBytes([int]0x1634));I @(0xD1,0xFA)
 I (@(0x03,$mem)+[BitConverter]::GetBytes([int]0x1630))
 I (@(0x03,$mem)+[BitConverter]::GetBytes([int]0xA8));I @(0x2B,0xC2)
 I @(0x89,0x44,0x24,($slot+4))
}
# Replace the per-frame camera-position call. Its one argument is callee-cleaned.
$cameraVA=$baseVA+$code.Count
I @(0x55);I @(0x8B,0xEC);I @(0x53);I @(0x56);I @(0x57);I @(0x8B,0xF1)
TrainingGate 'camera_original'
Global 0x8B 0x3D 0x86AC48;I @(0x85,0xFF);J 0x84 'camera_original'
Global 0x8B 0x1D 0x86AC54;I @(0x85,0xDB);J 0x84 'camera_original'
I @(0x83,0xBB,0x50,0x18,0,0,0);J 0x85 'camera_original'
I @(0x83,0xEC,16)
FighterXY 0xCF 0
FighterXY 0xCB 8
# World-space midpoint; translations use a fixed 800x600 center before zoom.
I @(0x8B,0x04,0x24);I @(0x03,0x44,0x24,8);I @(0xD1,0xF8);I @(0x2D,0x90,1,0,0)
Global 0x8B 0x35 0x877674
Counter 992
I @(0x8B,0x44,0x24,4);I @(0x03,0x44,0x24,12);I @(0xD1,0xF8);I @(0x2D,0x2C,1,0,0)
Counter 1020
# Fit both centers within 640x400 pixels where supported. Clamp to original minimum 192 to avoid repeated scene buffers.
I @(0x8B,0x04,0x24);I @(0x2B,0x44,0x24,8);I @(0x99);I @(0x33,0xC2);I @(0x2B,0xC2)
I @(0x05,0xC8,0,0,0);I @(0x8B,0xC8);I @(0xB8,0,0x80,2,0);I @(0x33,0xD2);I @(0xF7,0xF1)
I @(0xBB,0,1,0,0);I @(0x3B,0xC3);J 0x83 'zoom_y';I @(0x8B,0xD8)
L 'zoom_y'
I @(0x8B,0x44,0x24,4);I @(0x2B,0x44,0x24,12);I @(0x99);I @(0x33,0xC2);I @(0x2B,0xC2)
I @(0x05,0xC8,0,0,0);I @(0x8B,0xC8);I @(0xB8,0,0x90,1,0);I @(0x33,0xD2);I @(0xF7,0xF1)
I @(0x3B,0xC3);J 0x83 'zoom_min';I @(0x8B,0xD8)
L 'zoom_min'
I @(0x81,0xFB,0xC0,0,0,0);J 0x83 'zoom_apply';I @(0xBB,0xC0,0,0,0)
L 'zoom_apply'
I @(0x8B,0xC3);Counter 692;Counter 720
I @(0x83,0xC4,16);T 0xE9 ($baseVA+0) # fixed below to camera_done
$fixups.Add(@(($code.Count-4),'camera_done'))
L 'camera_original'
I @(0xFF,0x75,8);I @(0x8B,0xCE);T 0xE8 0x4597D0
L 'camera_done'
I @(0x5F);I @(0x5E);I @(0x5B);I @(0x5D);I @(0xC2,4,0)
# P2 target changes must not recreate the singleton P1 HUD/cursor.
# At the intercepted instruction EDI is the source fighter, ESI its target.
$targetVA=$baseVA+$code.Count
TrainingGate 'target_original'
Global 0x39 0x3D 0x86AC54;J 0x85 'target_original'
T 0xE9 0x619CB1
L 'target_original'
I @(0x8B,0x86,0x8C,0x15,0,0);T 0xE9 0x619C8E
# Render only the current lock cursor during local VS. Retired fading cursors
# otherwise overlay the same name while controller targeting changes.
$cursorVA=$baseVA+$code.Count
TrainingGate 'cursor_original'
I @(0x50);I (@(0xA1)+[BitConverter]::GetBytes([uint32]0x86AC78))
I @(0x85,0xC0);J 0x84 'cursor_pop_original'
I @(0x39,0x88,0xD4,1,0,0);J 0x84 'cursor_pop_original'
I @(0x58);I @(0xC2,12,0)
L 'cursor_pop_original';I @(0x58)
L 'cursor_original';T 0xE9 0x4CC050
foreach($fix in $fixups){$v=[BitConverter]::GetBytes([int]($labels[$fix[1]]-$fix[0]-4));for($j=0;$j -lt 4;$j++){$code[$fix[0]+$j]=$v[$j]}}
if($code.Count -gt 1024){throw 'Helpers exceed appended space'}
[Array]::Copy($code.ToArray(),0,$bytes,$original.Length,$code.Count)
$allowed=[Collections.Generic.List[object]]::new()
function Patch([int]$offset,[byte[]]$before,[byte[]]$after){
 if([BitConverter]::ToString($original,$offset,$before.Length) -ne [BitConverter]::ToString($before)){throw ('Unexpected bytes at {0:X}' -f $offset)}
 [Array]::Copy($after,0,$bytes,$offset,$after.Length);$allowed.Add(@($offset,$after.Length))
}
function Hook([int]$va,[byte[]]$before,[long]$to,[byte]$opcode){$after=@($opcode)+[BitConverter]::GetBytes([int]($to-$va-5));while($after.Count -lt $before.Length){$after+=0x90};Patch ($va-0x400C00) $before $after}
Hook 0x5E1413 ([byte[]]@(0xE8,0xB8,0x83,0xE7,0xFF)) $cameraVA 0xE8
Hook 0x619C88 ([byte[]]@(0x8B,0x86,0x8C,0x15,0,0)) $targetVA 0xE9
$pe=[BitConverter]::ToInt32($original,0x3C)
$sections=$pe+24+[BitConverter]::ToUInt16($original,$pe+20)
$rdata=$sections+40
$cursorOffset=0x7E0C70-0x400000-[BitConverter]::ToUInt32($original,$rdata+12)+[BitConverter]::ToUInt32($original,$rdata+20)
Patch $cursorOffset ([BitConverter]::GetBytes([uint32]0x4CC050)) ([BitConverter]::GetBytes([uint32]$cursorVA))
Patch 0x2A0 ([BitConverter]::GetBytes([uint32]406)) ([BitConverter]::GetBytes([uint32](512+$code.Count)))
Patch 0x2A8 ([BitConverter]::GetBytes([uint32]512)) ([BitConverter]::GetBytes([uint32]1536))
Patch ($pe+24+4) ([BitConverter]::GetBytes([uint32]4028416)) ([BitConverter]::GetBytes([uint32](4028416+1024)))
$diff=0
for($i=0;$i -lt $original.Length;$i++){if($original[$i] -ne $bytes[$i]){$ok=$false;foreach($r in $allowed){if($i -ge $r[0] -and $i -lt $r[0]+$r[1]){$ok=$true;break}};if(!$ok){throw 'Unexpected change'};$diff++}}
$output=Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v6.exe'
if(Test-Path -LiteralPath $output){throw 'Preserving existing output'}
[IO.File]::WriteAllBytes($output,$bytes)
[pscustomobject]@{SourceSHA256=$expected;OutputSHA256=(Get-FileHash -LiteralPath $output).Hash;Length=$bytes.Length;ChangedExistingBytes=$diff;AppendedBytes=1024;HelperLength=$code.Count;CameraVA=('0x{0:X}' -f $cameraVA);TargetVA=('0x{0:X}' -f $targetVA);CursorVA=('0x{0:X}' -f $cursorVA);InstructionStarts=@($starts);RuntimeValidation='Pending'}|ConvertTo-Json -Depth 4

