#pragma once
// Battle-clock timing avoids duplicate updates in the two split-screen passes.
struct DamageFlash {
 unsigned owner=0,health=0,lastTick=0,hitTick=0;
 bool active=false;
 bool update(unsigned fighter,unsigned hp,unsigned tick){
  if(!fighter||fighter!=owner||tick<lastTick||hp>health){
   owner=fighter;health=hp;lastTick=tick;active=false;return false;
  }
  if(hp<health){hitTick=tick;active=true;}
  health=hp;lastTick=tick;
  if(active&&tick-hitTick>=24)active=false;
  return active&&((tick-hitTick)/6)%2==0;
 }
};
