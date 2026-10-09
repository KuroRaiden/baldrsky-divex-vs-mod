$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$sha=[Security.Cryptography.SHA256]::Create()
try {$hash=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes((Join-Path $root 'launcher\BaldrVSSetup.dll')))).Replace('-','')} finally {$sha.Dispose()}
[IO.File]::WriteAllText((Join-Path $root 'src\launcher_helper_hash.h'), "#pragma once`r`n#define VS_HELPER_SHA256 `"$hash`"`r`n")
