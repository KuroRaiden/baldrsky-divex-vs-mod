#include "launcher_protocol.h"
#include "round_rules.h"
#include "damage_flash.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 assert(round_seconds_remaining(0)==90&&round_seconds_remaining(179)==90);
 assert(round_seconds_remaining(180)==90&&round_seconds_remaining(239)==90);
 assert(round_seconds_remaining(240)==89&&round_seconds_remaining(5520)==1);
 assert(round_seconds_remaining(5580)==0&&round_seconds_remaining(~0u)==0);
 for(unsigned target:{2u,3u,5u}){
  assert(valid_set_preset(target));
  assert(!set_complete(target-1,target-1,target));
  assert(set_complete(target,0,target));assert(set_complete(0,target,target));
 }
 assert(valid_set_preset(0));assert(!set_complete(500,1000,0));
 for(unsigned bad:{1u,4u,6u,0xffffffffu})assert(!valid_set_preset(bad));
 assert(round_winner(0,10000,1000,10000)==2);
 assert(round_winner(4000,10000,4000,10000)==0);
 assert(round_winner(0,10000,0,10000)==0);
 assert(valid_music(0xffffffffu)&&valid_music(0xfffffffeu));
 for(unsigned i=0;i<92;i++)assert(valid_music(i));assert(!valid_music(92));
 DamageFlash flash;
 assert(!flash.update(1,10000,100));
 assert(flash.update(1,9900,101));
 assert(flash.update(1,9900,101)); // Two render passes do not consume timing.
 assert(!flash.update(1,9900,107));
 assert(flash.update(1,9800,108)); // A second hit restarts the flash.
 assert(!flash.update(1,9800,132));
 assert(flash.update(1,9700,133));
 assert(!flash.update(1,10000,134)); // Round reset/healing clears it.
 assert(!flash.update(2,9000,135)); // Roster replacement is not damage.
 assert(!flash.update(2,9000,1)); // Clock rewind clears state.
 assert(!flash.update(0,0,2));
 puts("PASS: finite set boundaries, Unlimited, draws, invalid presets and music bounds.");
}
