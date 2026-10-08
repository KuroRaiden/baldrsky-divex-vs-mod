param([string]$SourcePath='E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v6.exe',[string]$BuildDirectory=(Join-Path $env:TEMP ('baldr-v11b-'+[guid]::NewGuid().ToString('N'))))
$ErrorActionPreference='Stop'
$expected='7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739'
if((Get-FileHash -LiteralPath $SourcePath).Hash -ne $expected){throw 'Expected exact V6 source'}
$output=Join-Path $PSScriptRoot 'BaldrSky.VSPrototype.v11b.exe'
if(Test-Path -LiteralPath $output){throw 'Preserving existing output'}
New-Item -ItemType Directory -Path $BuildDirectory | Out-Null
$obj=Join-Path $BuildDirectory 'rounds.obj';$dll=Join-Path $BuildDirectory 'rounds.dll';$map=Join-Path $BuildDirectory 'rounds.map'
& clang --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-stack-protector -fno-builtin -c (Join-Path $PSScriptRoot '..\src\vs_rounds_v11b.c') -o $obj
if($LASTEXITCODE){throw 'Compiler failed'}
& lld-link /dll /noentry /nodefaultlib /fixed /base:0xCBE000 /dynamicbase:no /safeseh:no "/out:$dll" "/map:$map" $obj
if($LASTEXITCODE){throw 'Linker failed'}
$helper=[IO.File]::ReadAllBytes($dll);$original=[IO.File]::ReadAllBytes($SourcePath)
function U16($b,$o){[BitConverter]::ToUInt16($b,$o)}
function U32($b,$o){[BitConverter]::ToUInt32($b,$o)}
$hp=U32 $helper 0x3C;$gp=U32 $original 0x3C
$ho=$hp+24;$go=$gp+24
if((U32 $helper ($ho+28)) -ne 0xCBE000 -or (U32 $original ($go+56)) -ne 0x8BF000){throw 'Unexpected image layout'}
if((U32 $helper ($ho+104)) -ne 0){throw 'Helper unexpectedly imports libraries'}
$virtual=(U32 $helper ($ho+56))-0x1000
$rawSize=[int]([Math]::Ceiling($virtual/512.0)*512)
$bytes=New-Object byte[] ($original.Length+$rawSize);[Array]::Copy($original,$bytes,$original.Length)
$hs=$ho+(U16 $helper ($hp+20));$hc=U16 $helper ($hp+6)
for($n=0;$n -lt $hc;$n++){
 $sh=$hs+40*$n;$rva=U32 $helper ($sh+12);$size=U32 $helper ($sh+16);$raw=U32 $helper ($sh+20)
 if($size){if($rva -lt 0x1000 -or $rva-0x1000+$size -gt $virtual){throw 'Helper section out of range'};[Array]::Copy($helper,$raw,$bytes,$original.Length+$rva-0x1000,$size)}
}
$addresses=@{}
foreach($line in Get-Content -LiteralPath $map){if($line -match '\s(@setup@8|@tick@8|_recovery@4|@cursor@20|@cursor_update@8|@attack_select@8|@fighter_update@12|@tracker_render@20)\s+([0-9a-fA-F]+)\s'){$addresses[$matches[1]]=[Convert]::ToUInt32($matches[2],16)}}
if($addresses.Count -ne 8){throw 'Missing helper entries'}
$allowed=[Collections.Generic.List[object]]::new()
function Put($offset,[byte[]]$data){[Array]::Copy($data,0,$bytes,$offset,$data.Length);$allowed.Add(@($offset,$data.Length))}
function Put32($offset,[uint32]$value){Put $offset ([BitConverter]::GetBytes($value))}
$gc=U16 $original ($gp+6);$gs=$go+(U16 $original ($gp+20));$newHeader=$gs+40*$gc
if($newHeader+40 -gt (U32 $original ($go+60))){throw 'No section-header space'}
for($i=$newHeader;$i -lt $newHeader+40;$i++){if($original[$i]){throw 'Section header is occupied'}}
Put $newHeader ([Text.Encoding]::ASCII.GetBytes('.vsround'))
Put32 ($newHeader+8) $virtual;Put32 ($newHeader+12) 0x8BF000;Put32 ($newHeader+16) $rawSize;Put32 ($newHeader+20) $original.Length;Put32 ($newHeader+36) ([Convert]::ToUInt32('E0000060',16))
Put ($gp+6) ([BitConverter]::GetBytes([uint16]($gc+1)))
Put32 ($go+56) (0x8BF000+$virtual)
Put32 ($go+4) ((U32 $original ($go+4))+(U32 $helper ($ho+4)))
Put32 ($go+8) ((U32 $original ($go+8))+(U32 $helper ($ho+8)))
Put32 ($go+12) ((U32 $original ($go+12))+(U32 $helper ($ho+12)))
function FileOffset([uint32]$va){$rva=$va-0x400000;for($n=0;$n -lt $gc;$n++){$sh=$gs+40*$n;$start=U32 $original ($sh+12);$size=U32 $original ($sh+16);if($rva -ge $start -and $rva -lt $start+$size){return (U32 $original ($sh+20))+$rva-$start}};throw 'Unmapped hook'}
foreach($hook in @(@(0x7EEE54,0x5F8CC0,'@tick@8'),@(0x7EEF28,0x5FB8C0,'@setup@8'),@(0x7EEF40,0x5F8D50,'_recovery@4'),@(0x7E0C70,0xCBE411,'@cursor@20'),@(0x7E0C6C,0x4CBD50,'@cursor_update@8'),@(0x7F02E4,0x61E390,'@fighter_update@12'),@(0x7EDF20,0x5E7430,'@tracker_render@20'))){$offset=FileOffset $hook[0];if((U32 $original $offset) -ne $hook[1]){throw 'Unexpected vtable entry'};Put32 $offset $addresses[$hook[2]]}
# Preserve each native tail-jump opcode and replace only its relative destination.
$attackSites=@(0x62A45A,0x62A46E,0x62A49A,0x62B298,0x62B2AF,0x62B2DB,0x62B302,0x62C38F,0x62C3C4,0x62C3EE,0x62DBB6,0x62DBC9,0x62DBF5,0x62E618,0x62E629,0x62E652)
foreach($va in $attackSites){$offset=FileOffset $va;if($original[$offset] -ne 0xE9 -or $va+5+[BitConverter]::ToInt32($original,$offset+1) -ne 0x61A280){throw 'Unexpected attack-selector tail'};Put ($offset+1) ([BitConverter]::GetBytes([int]($addresses['@attack_select@8']-$va-5)))}
$diff=0
for($i=0;$i -lt $original.Length;$i++){if($original[$i] -ne $bytes[$i]){$ok=$false;foreach($r in $allowed){if($i -ge $r[0] -and $i -lt $r[0]+$r[1]){$ok=$true;break}};if(!$ok){throw 'Unexpected change'};$diff++}}
[IO.File]::WriteAllBytes($output,$bytes)
[pscustomobject]@{SourceSHA256=$expected;OutputSHA256=(Get-FileHash -LiteralPath $output).Hash;Length=$bytes.Length;ChangedExistingBytes=$diff;AppendedBytes=$rawSize;Helpers=$addresses;BuildDirectory=$BuildDirectory;RuntimeValidation='Pending'}|ConvertTo-Json -Depth 4




