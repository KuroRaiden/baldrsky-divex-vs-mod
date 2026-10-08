Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class BaldrReadOnly {
 [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint access, bool inherit, int id);
 [DllImport("kernel32.dll", SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] buffer, UIntPtr size, out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$games = @(Get-Process | Where-Object { $_.ProcessName -like 'BaldrSky*' })
if ($games.Count -ne 1) { throw 'Run exactly one Baldr Sky executable before inspection.' }
$game = $games[0]
$handle = [BaldrReadOnly]::OpenProcess(0x10, $false, $game.Id)
if ($handle -eq [IntPtr]::Zero) { throw 'Cannot open game for read-only inspection.' }
function Read-U32([long]$address) {
 $bytes = New-Object byte[] 4
 $count = [UIntPtr]::Zero
 if (![BaldrReadOnly]::ReadProcessMemory($handle, [IntPtr]$address, $bytes, [UIntPtr]4, [ref]$count) -or $count.ToUInt64() -ne 4) { throw ('Cannot read 0x{0:X}' -f $address) }
 [BitConverter]::ToUInt32($bytes, 0)
}
try {
 $practice = Read-U32 0x86AAD4
 $player1 = Read-U32 0x86AC48
 $player2 = Read-U32 0x86AC54
 $input = Read-U32 0x877664
 $enemy = if ($practice) { Read-U32 ($practice + 324) } else { 0 }
 $fighters = foreach ($entry in @(@('Player1',$player1),@('Player2',$player2),@('TrainingOpponent',$enemy))) {
  if ($entry[1]) { [pscustomobject]@{Role=$entry[0];Pointer=('0x{0:X}' -f $entry[1]);ControlKind=(Read-U32 ($entry[1]+6224));ControllerSlot=(Read-U32 ($entry[1]+6228));TeamObject=(Read-U32 ($entry[1]+5508));Character=(Read-U32 ($entry[1]+6308))} }
 }
 [pscustomobject]@{Process=$game.Id;GameMode=(Read-U32 0x86AA1C);BattleMode=(Read-U32 0x86AA18);BattleClass=if($practice){'0x{0:X}' -f (Read-U32 $practice)}else{$null};InputSlot0=if($input){'0x{0:X}' -f (Read-U32 ($input+16))}else{$null};InputSlot1=if($input){'0x{0:X}' -f (Read-U32 ($input+20))}else{$null};Fighters=@($fighters)} | ConvertTo-Json -Depth 4
} finally { [void][BaldrReadOnly]::CloseHandle($handle) }
