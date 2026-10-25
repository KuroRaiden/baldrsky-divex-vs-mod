#pragma once
// Test-only: native record -> native playback, with a second human input stream.
// Never enabled in the local or online launcher's distributed plugin.
static constexpr unsigned audit_frames=300;
static unsigned audit_phase=0,audit_index=0,audit_previous[2]={};
static bool audit_started=false,audit_pending=false;
static FILE* audit_log=nullptr;
static std::array<std::array<u32,20>,audit_frames> audit_recorded;
static DWORD WINAPI replay_audit_seed(){return 0x42565331;}
static void audit_input(unsigned index,u32 (&pressed)[2],u32 (&held)[2]){
 for(unsigned p=0;p<2;p++){
  held[p]=(index/60%2)?(p?0x40:4):(p?4:0x40);
  if(index>=180&&index%45<2)held[p]|=0x100u<<((index/45+p)%3);
  pressed[p]=held[p]&~audit_previous[p];audit_previous[p]=held[p];
 }
}
static void audit_sample(const u32 (&pressed)[2],const u32 (&held)[2],u32 replayFrame){
 if(audit_index>=audit_frames)return;
 std::array<u32,20> values={audit_index,U(0x8738EC),pressed[0],held[0],pressed[1],held[1],U(((u32(__cdecl*)())0x67C340)()+20),replayFrame};
 unsigned field=8;
 for(unsigned p=0;p<2;p++){
  u32 fighter=U(p?0x86AC54:0x86AC48);if(!fighter){fclose(audit_log);TerminateProcess(GetCurrentProcess(),33);return;}
  for(unsigned offset:{160u,164u,168u,5656u,6284u,6364u})values[field++]=U(fighter+offset);
 }
 bool same=!audit_phase||values==audit_recorded[audit_index];
 if(!audit_phase)audit_recorded[audit_index]=values;
 fprintf(audit_log,"%u,%u,%s",audit_phase,audit_index,same?"MATCH":"DIFF");for(auto value:values)fprintf(audit_log,",%u",value);fputc('\n',audit_log);
 if(++audit_index==audit_frames){
  if(!audit_phase)audit_pending=true;
  else {fclose(audit_log);TerminateProcess(GetCurrentProcess(),0);}
 }
}
static int __fastcall audit_record(void* replay,void*,u32 pressed,u32 held){
 if(!audit_started||audit_phase)return ((int(__thiscall*)(void*,u32,u32))0x4BFC40)(replay,pressed,held);
 u32 p[2]={},h[2]={};audit_input(audit_index,p,h);
 U(0x874BE0)=p[0];U(0x874BDC)=h[0];U(0x874BD8)=p[1];U(0x874BD4)=h[1];
 audit_sample(p,h,U((u32)replay+5528));
 return ((int(__thiscall*)(void*,u32,u32))0x4BFC40)(replay,p[0],h[0]);
}
static void __fastcall audit_playback(void* replay,void*,u32* pressed,u32* held){
 ((void(__thiscall*)(void*,u32*,u32*))0x4BFA20)(replay,pressed,held);
 if(!audit_started||!audit_phase)return;
 u32 p[2]={},h[2]={};audit_input(audit_index,p,h);
 // 1P comes from the actual native replay; only 2P uses our supplemental stream.
 p[0]=*pressed;h[0]=*held;U(0x874BD8)=p[1];U(0x874BD4)=h[1];
 audit_sample(p,h,U((u32)replay+5528)-1);
}
static bool install_replay_audit(){
 BYTE expected[]={0xff,0x15,0x7c,0x93,0x7d,0x00};BYTE* site=(BYTE*)0x442F98;
 if(memcmp(site,expected,6)||!split_hook_matches(0x5F6BDF,0x4BFC40)||!split_hook_matches(0x5F6BFD,0x4BFA20))return false;
 DWORD old=0;if(!VirtualProtect(site,6,PAGE_EXECUTE_READWRITE,&old))return false;
 site[0]=0xe8;*(LONG*)(site+1)=LONG((BYTE*)replay_audit_seed-(site+5));site[5]=0x90;
 FlushInstructionCache(GetCurrentProcess(),site,6);DWORD ignored=0;VirtualProtect(site,6,old,&ignored);
 return split_install_call(0x5F6BDF,(void*)audit_record)&&split_install_call(0x5F6BFD,(void*)audit_playback);
}
static void native_replay_audit(){
 if(launcher_config[5]!=3)return;
 if(!audit_started){
  audit_started=true;
  if(_wfopen_s(&audit_log,L"online-native-replay-audit.csv",L"wb")||!audit_log){TerminateProcess(GetCurrentProcess(),31);return;}
  ((void(__stdcall*)(char))0x442F90)(0);
  *(BYTE*)0x86A9E8=1;*(BYTE*)0x86E954=1;*(BYTE*)0x86E955=0;
  fprintf(audit_log,"seed,%08X,recording,%u\n",U(0x86D3C0+44),*(BYTE*)(0x86D3C0+5524));
  if(U(0x86D3C0+44)!=0x42565331||!*(BYTE*)(0x86D3C0+5524)){fclose(audit_log);TerminateProcess(GetCurrentProcess(),32);return;}
 }else if(audit_pending){
  audit_pending=false;audit_phase=1;audit_index=0;audit_previous[0]=audit_previous[1]=0;
  ((void(__stdcall*)(char,char,char))0x446100)(0,0,0);
  // Native replay rebuilds combat but keeps the global battle clock. The mod's
  // timer/round logic also needs its starting clock restored for equivalence.
  U(0x8738EC)=audit_recorded[0][1];
  *(BYTE*)0x86A9E8=1;*(BYTE*)0x86E954=0;*(BYTE*)0x86E955=1;
 }
 // Real devices and menus cannot inject extra test inputs between combat steps.
 U(0x874BE0)=U(0x874BDC)=U(0x874BD8)=U(0x874BD4)=0;
}
