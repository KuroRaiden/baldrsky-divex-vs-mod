param([string]$Helper='launcher\BaldrVSSetup.dll',[string]$Header='src\launcher_helper_hash.h')
$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$sha=[Security.Cryptography.SHA256]::Create()
try {$hash=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes((Join-Path $root $Helper)))).Replace('-','')} finally {$sha.Dispose()}
[IO.File]::WriteAllText((Join-Path $root $Header), "#pragma once`r`n#define VS_HELPER_SHA256 `"$hash`"`r`n")
