#pragma once
#include "damage_flash.h"
static DamageFlash p2_damage_flash;
static u32 native_ring_sprite_original=0x70F800;
static u32 __cdecl native_ring_tint(u32 fighter,u32 original){
 if(!fighter||fighter!=U(0x86AC54)||U(0x86AA1C)!=1||U(0x86AA18)!=2)return original;
 int hp=(int)U(fighter+5656);
 return p2_damage_flash.update(fighter,hp>0?(u32)hp:0,U(0x8738EC))?0xFFFF00:0xFF80FF;
}
// This exact sprite call has the ring in EBP and color as argument six.
// Preserve the renderer's registers/flags and tail-call the native drawer.
static __declspec(naked) void native_ring_sprite(){
 __asm {
  pushfd
  pushad
  push dword ptr [esp+60]
  push dword ptr [ebp+264]
  call native_ring_tint
  add esp,8
  mov dword ptr [esp+60],eax
  popad
  popfd
  jmp dword ptr [native_ring_sprite_original]
 }
}
extern "C" {
 void configure_round_set(u32);
 int __fastcall tick(void*,void*);
 int __fastcall setup(void*,void*);
 void __stdcall recovery(u32);
 void __fastcall cursor(void*,void*,int,int,int);
 int __fastcall cursor_update(void*,void*);
 int __fastcall fighter_update(void*,void*,unsigned char*);
 void __fastcall tracker_render(void*,void*,int,int,int);
 int __fastcall attack_select(void*,void*);
 u32 countdown(void);
 u32 snapshot(void);
}
static ULONGLONG native_close_at=0;
extern "C" void native_set_finished(u32 winner,u32 p1,u32 p2){
 launcher_config[5]=4;launcher_config[6]=winner|(p1<<8)|(p2<<16);
 native_close_at=GetTickCount64()+2000;publish_native_status();
}
static bool install_native_rounds(){
 const u32 ringCall=0x4CDE28;
 u32 ringTarget=ringCall+5+*(LONG*)(ringCall+1);
 if(*(BYTE*)ringCall!=0xE8||ringTarget<0xCBE000||ringTarget>=0xCBE200)return false;
 const u32 slots[]={0x7EEE54,0x7EEF28,0x7EEF40,0x7E0C70,0x7E0C6C,0x7F02E4,0x7EDF20};
 void* functions[]={(void*)tick,(void*)setup,(void*)recovery,(void*)cursor,(void*)cursor_update,(void*)fighter_update,(void*)tracker_render};
 const u32 tails[]={0x62A45A,0x62A46E,0x62A49A,0x62B298,0x62B2AF,0x62B2DB,0x62B302,0x62C38F,0x62C3C4,0x62C3EE,0x62DBB6,0x62DBC9,0x62DBF5,0x62E618,0x62E629,0x62E652};
 // All replaced functions must belong to the exact V11c round section.
 for(u32 slot:slots)if(U(slot)<0xCBF000||U(slot)>=0xCC3000)return false;
 for(u32 tail:tails){
  u32 target=tail+5+*(LONG*)(tail+1);
  if(*(BYTE*)tail!=0xE9||target<0xCBF000||target>=0xCC3000)return false;
 }
 for(unsigned i=0;i<7;i++){
  DWORD protection=0;if(!VirtualProtect((void*)slots[i],4,PAGE_READWRITE,&protection))return false;
  U(slots[i])=(u32)functions[i];DWORD ignored=0;VirtualProtect((void*)slots[i],4,protection,&ignored);
 }
 for(u32 tail:tails){
  DWORD protection=0;if(!VirtualProtect((void*)tail,5,PAGE_EXECUTE_READWRITE,&protection))return false;
  *(LONG*)(tail+1)=(LONG)((BYTE*)attack_select-(BYTE*)(tail+5));
  FlushInstructionCache(GetCurrentProcess(),(void*)tail,5);
  DWORD ignored=0;VirtualProtect((void*)tail,5,protection,&ignored);
 }
 DWORD protection=0;
 if(!VirtualProtect((void*)ringCall,5,PAGE_EXECUTE_READWRITE,&protection))return false;
 *(LONG*)(ringCall+1)=(LONG)((BYTE*)native_ring_sprite-(BYTE*)(ringCall+5));
 FlushInstructionCache(GetCurrentProcess(),(void*)ringCall,5);
 DWORD ignored=0;VirtualProtect((void*)ringCall,5,protection,&ignored);
 configure_round_set(launcher_config[7]);return true;
}
