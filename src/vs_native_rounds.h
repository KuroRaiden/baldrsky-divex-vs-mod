#pragma once
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
 configure_round_set(launcher_config[7]);return true;
}
