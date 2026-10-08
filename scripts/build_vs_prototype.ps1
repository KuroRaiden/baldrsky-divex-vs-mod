param([string]$SourcePath = 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe')
$source = $SourcePath
$expected = '0A34F07F2698D8105E593AF4C2A09698807604DFFE2DD18E0B193FDBD6A00F02'
if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $expected) { throw 'Source executable changed.' }
$bytes = [IO.File]::ReadAllBytes($source)
$hookOffset = 0x1FAF99
$hookVA = 0x5FBB99
$caveOffset = 0x3D79B2
$caveVA = 0x7D85B2
if ([BitConverter]::ToString($bytes, $hookOffset, 5) -ne 'E8-32-1A-FE-FF') { throw 'Unexpected Training spawn call.' }
for ($i=0; $i -lt 78; $i++) { if ($bytes[$caveOffset+$i] -ne 0) { throw 'Code padding is occupied.' } }
$code = [Collections.Generic.List[byte]]::new()
function Emit([byte[]]$data) { $code.AddRange($data) }
function Emit-Call([int]$target) {
 $position = $code.Count
 Emit @(0xE8)
 Emit ([BitConverter]::GetBytes([int]($target - ($caveVA+$position+5))))
}
# cdecl wrapper: forward all seven spawn arguments, retain the returned fighter.
Emit @(0x55,0x8B,0xEC)
foreach ($offset in @(0x20,0x1C,0x18,0x14,0x10,0x0C,0x08)) { Emit @(0xFF,0x75,$offset) }
Emit-Call 0x5DD5D0
Emit @(0x83,0xC4,0x1C,0x50)
# Only Survival's Training opponent is reassigned. Other Training contexts retain AI.
Emit @(0x83,0x3D,0x1C,0xAA,0x86,0x00,0x01)
$modeBranch = $code.Count
Emit @(0x75,0x00)
Emit @(0x85,0xC0)
$nullBranch = $code.Count
Emit @(0x74,0x00)
# Existing engine routine assigns human control and controller slot one.
Emit @(0x8B,0x88,0x58,0x01,0x00,0x00,0x6A,0x01)
Emit-Call 0x629330
$done = $code.Count
Emit @(0x58,0x5D,0xC3)
$code[$modeBranch+1] = [byte]($done-($modeBranch+2))
$code[$nullBranch+1] = [byte]($done-($nullBranch+2))
if ($code.Count -gt 78) { throw 'Wrapper exceeds available padding.' }
[Array]::Copy($code.ToArray(),0,$bytes,$caveOffset,$code.Count)
$bytes[$hookOffset] = 0xE8
[Array]::Copy([BitConverter]::GetBytes([int]($caveVA-($hookVA+5))),0,$bytes,$hookOffset+1,4)
# Include the existing raw padding in .text's declared virtual size.
$pe = [BitConverter]::ToInt32($bytes,0x3C)
$section = $pe+24+[BitConverter]::ToUInt16($bytes,$pe+20)
$sizeOffset = $section+8
$oldSize = [BitConverter]::ToUInt32($bytes,$sizeOffset)
$rawSize = [BitConverter]::ToUInt32($bytes,$section+16)
if ($oldSize -ne 4027826 -or $rawSize -ne 4027904) { throw 'Unexpected .text layout.' }
[Array]::Copy([BitConverter]::GetBytes($rawSize),0,$bytes,$sizeOffset,4)
$output = Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.exe'
[IO.File]::WriteAllBytes($output,$bytes)
$original = [IO.File]::ReadAllBytes($source)
$diff = @()
for ($i=0; $i -lt $original.Length; $i++) {
 if ($original[$i] -ne $bytes[$i]) {
  if (!(($i -ge $hookOffset -and $i -lt $hookOffset+5) -or ($i -ge $caveOffset -and $i -lt $caveOffset+$code.Count) -or ($i -ge $sizeOffset -and $i -lt $sizeOffset+4))) { throw 'Unexpected difference.' }
  $diff += $i
 }
}
if ($bytes.Length -ne $original.Length) { throw 'File length changed.' }
$manifest = [pscustomobject]@{Source=$source;SourceSHA256=$expected;Prototype=$output;PrototypeSHA256=(Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash;Length=$bytes.Length;DifferenceCount=$diff.Count;HookOffset=('0x{0:X}' -f $hookOffset);HookBytes=[BitConverter]::ToString($bytes,$hookOffset,5);CodeOffset=('0x{0:X}' -f $caveOffset);CodeVA=('0x{0:X}' -f $caveVA);CodeLength=$code.Count;CodeBytes=[BitConverter]::ToString($code.ToArray());TextSizeHeaderOffset=('0x{0:X}' -f $sizeOffset);TextVirtualSizeBefore=$oldSize;TextVirtualSizeAfter=$rawSize}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'vs-prototype-manifest.json') -Encoding utf8
$manifest | ConvertTo-Json
