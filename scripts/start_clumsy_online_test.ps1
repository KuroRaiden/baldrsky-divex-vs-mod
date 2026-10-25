param([Parameter(Mandatory=$true)][string]$ClumsyPath,[ValidateRange(0,1000)][int]$LagMs=60,[ValidateRange(0,50)][int]$LossPercent=5)
$ErrorActionPreference='Stop'
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run this script in an administrator terminal: Clumsy requires its WinDivert driver. No system security settings need changing.'}
$program=(Resolve-Path -LiteralPath $ClumsyPath).Path
if([IO.Path]::GetFileName($program) -ne 'clumsy.exe'){throw 'Choose the official clumsy.exe'}
if(Get-Process clumsy -ErrorAction SilentlyContinue){throw 'Preserving the existing Clumsy session'}
$filter='outbound and ip and udp and ip.DstAddr == 127.0.0.1 and (udp.SrcPort == 24764 or udp.SrcPort == 24765) and (udp.DstPort == 24764 or udp.DstPort == 24765)'
# Only the direct local peer test is filtered; the proxy path is separate.
# Stop automatically after 30 seconds, flushing held packets on clean shutdown.
$arguments=@('--filter',('"'+$filter+'"'),'--lag','on','--lag-time',$LagMs,'--lag-inbound','off','--lag-outbound','on','--drop','on','--drop-chance',$LossPercent,'--drop-inbound','off','--drop-outbound','on','--timeout','30')
Start-Process -FilePath $program -ArgumentList $arguments -WorkingDirectory (Split-Path $program -Parent) -WindowStyle Hidden
'Clumsy scoped to loopback UDP ports 24764/24765 for 30 seconds. Run test_online_peers.ps1 now.'
