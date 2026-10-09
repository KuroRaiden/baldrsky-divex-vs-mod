param([string]$SourcePath,[string]$OutputPath)
$ErrorActionPreference='Stop'
if((Get-FileHash -LiteralPath $SourcePath).Hash -ne '42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460'){throw 'Exact V11c source required'}
if(Test-Path -LiteralPath $OutputPath){throw 'Preserving existing mod output'}
$source=[IO.File]::ReadAllBytes($SourcePath)
function U16($o){[BitConverter]::ToUInt16($source,$o)}
function U32($o){[BitConverter]::ToUInt32($source,$o)}
$pe=U32 0x3c;$opt=$pe+24;$sections=$opt+(U16 ($pe+20));$count=U16 ($pe+6)
if((U16 ($pe+4)) -ne 0x14c -or (U16 $opt) -ne 0x10b -or (U32 ($opt+56)) -ne 0x8c3000){throw 'Unexpected PE image'}
$header=$sections+40*$count
if($header+40 -gt (U32 ($opt+60))){throw 'No header space'}
for($i=$header;$i -lt $header+40;$i++){if($source[$i]){throw 'Header occupied'}}
function Offset([uint32]$rva){for($i=0;$i -lt $count;$i++){$s=$sections+40*$i;$va=U32 ($s+12);$size=U32 ($s+16);if($rva -ge $va -and $rva -lt $va+$size){return (U32 ($s+20))+$rva-$va}};throw 'RVA not file backed'}
$import=Offset (U32 ($opt+104));$descriptors=[Collections.Generic.List[byte]]::new();$n=0
while((U32 ($import+20*$n)) -or (U32 ($import+20*$n+12))){for($i=0;$i -lt 20;$i++){$descriptors.Add($source[$import+20*$n+$i])};$n++;if($n -gt 100){throw 'Import bounds'}}
$fileAlign=U32 ($opt+36);$raw=[int]([Math]::Ceiling($source.Length/[double]$fileAlign)*$fileAlign)
$rawSize=[int]([Math]::Ceiling(4096/[double]$fileAlign)*$fileAlign)
$bytes=New-Object byte[] ($raw+$rawSize);[Array]::Copy($source,$bytes,$source.Length)
function Put($o,[byte[]]$v){[Array]::Copy($v,0,$bytes,$o,$v.Length)}
function P32($o,[uint32]$v){Put $o ([BitConverter]::GetBytes($v))}
$rva=0x8c3000;$base=0x400000
$table=0x100;$descriptor=$table+$descriptors.Count;$lookup=$descriptor+40;$iat=$lookup+8;$dll=$iat+8
$dllBytes=[Text.Encoding]::ASCII.GetBytes("BaldrVSPlugin.dll`0")
$hint=$dll+$dllBytes.Length;if($hint%2){$hint++}
if($hint+32 -gt 4096){throw 'Import section overflow'}
Put ($raw+$table) ($descriptors.ToArray())
P32 ($raw+$descriptor) ($rva+$lookup);P32 ($raw+$descriptor+12) ($rva+$dll);P32 ($raw+$descriptor+16) ($rva+$iat)
P32 ($raw+$lookup) ($rva+$hint);P32 ($raw+$iat) ($rva+$hint)
Put ($raw+$dll) $dllBytes;Put ($raw+$hint+2) ([Text.Encoding]::ASCII.GetBytes("StartVSPlugin`0"))
# Preserve entry-point registers/flags, call the normal imported plugin, and
# fail closed on setup error. No remote allocation, thread or process writes.
$entry=U32 ($opt+16)
Put $raw ([byte[]]@(0x9c,0x60,0xff,0x15));P32 ($raw+4) ($base+$rva+$iat)
Put ($raw+8) ([byte[]]@(0x85,0xc0,0x74,0x07,0x61,0x9d,0xe9))
Put ($raw+15) ([BitConverter]::GetBytes([int]($entry-($rva+19))))
Put ($raw+19) ([byte[]]@(0x61,0x9d,0x31,0xc0,0xc3))
Put $header ([Text.Encoding]::ASCII.GetBytes('.vsboot'))
P32 ($header+8) 4096;P32 ($header+12) $rva;P32 ($header+16) $rawSize;P32 ($header+20) $raw
P32 ($header+36) ([Convert]::ToUInt32('E0000060',16))
Put ($pe+6) ([BitConverter]::GetBytes([uint16]($count+1)))
P32 ($opt+16) $rva;P32 ($opt+56) 0x8c4000;P32 ($opt+104) ($rva+$table);P32 ($opt+108) ($descriptors.Count+40)
P32 ($opt+64) 0
[IO.File]::WriteAllBytes($OutputPath,$bytes)
Write-Output 'Built separate game with normal DLL import and entry-point initialization.'
