param([string]$BuildDirectory,[string]$OutputDirectory,[string]$ArchivePath)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!$BuildDirectory){$BuildDirectory=Join-Path $root 'online-build'}
if(!$OutputDirectory){$OutputDirectory=Join-Path (Split-Path $root -Parent) 'BaldrVS-Online-Chat-Test'}
if(!$ArchivePath){$ArchivePath=$OutputDirectory+'.zip'}
$BuildDirectory=(Resolve-Path -LiteralPath $BuildDirectory).Path
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$ArchivePath=[IO.Path]::GetFullPath($ArchivePath)
if($OutputDirectory -eq $root -or $OutputDirectory -eq $BuildDirectory){throw 'Choose a separate package directory'}
# Package only reviewed redistributable files. Never enumerate the build or
# install directory: those can contain game binaries, profiles and test logs.
$sources=[ordered]@{}
foreach($name in @('BaldrVSLauncherOnline.exe','BaldrAutoPunch.dll','AutoPunch-LICENSE.txt','GekkoNet-LICENSE.txt','Detours-LICENSE.txt','ImGui-LICENSE.txt')){
 $sources[$name]=Join-Path $BuildDirectory $name
}
foreach($name in @('mechas.tsv','stages.tsv','music.tsv')){$sources[$name]=Join-Path $root ('data/'+$name)}
$sources['README.txt']=Join-Path $root 'docs/online-preview-readme.txt'
foreach($source in $sources.Values){if(!(Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing package source: $source"}}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$manifest=@(foreach($name in $sources.Keys){
 $destination=Join-Path $OutputDirectory $name
 Copy-Item -LiteralPath $sources[$name] -Destination $destination -Force
 [ordered]@{File=$name;SHA256=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash;Bytes=(Get-Item -LiteralPath $destination).Length}
})
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'SHA256.json'),($manifest|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
Add-Type -AssemblyName System.IO.Compression
$archiveParent=Split-Path $ArchivePath -Parent
New-Item -ItemType Directory -Path $archiveParent -Force | Out-Null
$temporary=Join-Path $archiveParent ([IO.Path]::GetRandomFileName())
try{
 $stream=[IO.File]::Open($temporary,[IO.FileMode]::CreateNew)
 try{
  $zip=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create,$true)
  try{foreach($name in (@($sources.Keys)+@('SHA256.json'))){
   $entry=$zip.CreateEntry($name,[IO.Compression.CompressionLevel]::Optimal)
   $entry.LastWriteTime=[DateTimeOffset]::new(2026,10,24,0,0,0,[TimeSpan]::Zero)
   $content=[IO.File]::ReadAllBytes((Join-Path $OutputDirectory $name))
   $entryStream=$entry.Open();try{$entryStream.Write($content,0,$content.Length)}finally{$entryStream.Dispose()}
  }}finally{$zip.Dispose()}
 }finally{$stream.Dispose()}
 Move-Item -LiteralPath $temporary -Destination $ArchivePath -Force
}finally{if(Test-Path -LiteralPath $temporary){Remove-Item -LiteralPath $temporary -Force}}
Get-Item -LiteralPath $ArchivePath | Select-Object FullName,Length
Get-FileHash -LiteralPath $ArchivePath -Algorithm SHA256
