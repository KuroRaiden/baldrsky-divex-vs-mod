#pragma once
#include "equipment_rules.h"
#include <string>
static VSEquipmentConfig native_equipment;
static u32 native_equipment_records[2][14][4];
static bool native_equipment_ready=false,native_p2_moves_loaded=false;
static u32 equipment_asset(u32 mecha){u32 pool=U(0x86AAC4);return pool?U(U(pool+16)+4*mecha):0;}
static std::string equipment_string(u32 at){
 u32 length=U(at+20),capacity=U(at+24);if(length>8192)return "";
 const char* text=(const char*)(capacity>=16?U(at+4):at+4);
 int n=MultiByteToWideChar(932,0,text,length,nullptr,0);std::wstring wide(n,L'\0');
 if(n)MultiByteToWideChar(932,0,text,length,wide.data(),n);
 n=WideCharToMultiByte(CP_UTF8,0,wide.data(),n,nullptr,0,nullptr,nullptr);std::string utf(n,'\0');
 if(n)WideCharToMultiByte(CP_UTF8,0,wide.data(),(int)wide.size(),utf.data(),n,nullptr,nullptr);
 for(char& c:utf)if(c=='\t'||c=='\r'||c=='\n')c=' ';return utf;
}
static bool export_equipment_catalog(){
 // Export only from the user's loaded game; proprietary catalogs are not shipped.
 FILE* out=nullptr;if(_wfopen_s(&out,L"equipment.tsv.tmp",L"wb")||!out)return false;
 fputs("BALDR-EQUIPMENT\t1\n",out);
 for(u32 mecha=0;mecha<103;mecha++)if(valid_mecha(mecha)){
  u32 asset=equipment_asset(mecha);
  if(U(asset+108)==U(asset+112))((int (__thiscall *)(void*))0x652F40)((void*)asset);
  u32 begin=U(asset+108),end=U(asset+112);
  if(end<begin||(end-begin)%4||(end-begin)/4>500){fclose(out);return false;}
  fprintf(out,"M\t%u",mecha);
  for(unsigned slot=0;slot<14;slot++)fprintf(out,"\t%u",U(U(0x873C08)+4336*mecha+4+4*equipment_save_slot(slot)));
  fputc('\n',out);
  for(u32 i=0;i<(end-begin)/4;i++){
   u32 move=U(begin+4*i);auto name=equipment_string(move+8),description=equipment_string(move+64);
   if(*(BYTE*)(move+164)&&!name.empty())fprintf(out,"W\t%u\t%u\t%u\t%s\t%s\n",mecha,i,U(move+120),name.c_str(),description.c_str());
  }
 }
 bool ok=fclose(out)==0;if(ok)ok=MoveFileExW(L"equipment.tsv.tmp",L"equipment.tsv",MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;return ok;
}
static bool initialize_equipment(){
 for(unsigned player=0;player<2;player++){
  u32 mecha=launcher_config[2+player],asset=equipment_asset(mecha);
  u32 count=(U(asset+112)-U(asset+108))/4;
  EquipmentSlots resolved;
  for(unsigned slot=0;slot<14;slot++){
   u32 id=native_equipment.players[player][slot];
   if(id==equipment_default)id=U(U(0x873C08)+4336*mecha+4+4*equipment_save_slot(slot));
   if(id!=equipment_empty&&(id>=count||!equipment_type_allowed(slot,U(U(U(asset+108)+4*id)+120))))return false;
   resolved[slot]=id;
   auto& record=native_equipment_records[player][slot];
   record[0]=id;record[1]=mecha==0?2:0;
   record[2]=slot<12?slot/4:slot-12;record[3]=slot<12?slot%4:4;
  }
  if(!equipment_unique(resolved))return false;
 }
 native_equipment_ready=true;
 for(unsigned slot=0;slot<14;slot++){
  u32 at=U(0x86AA2C+4*slot);for(unsigned k=0;k<4;k++)U(at+4*k)=native_equipment_records[0][slot][k];
  U(0x87347C+4*equipment_save_slot(slot))=native_equipment_records[0][slot][0];
 }
 return true;
}
struct NativeEquipmentScope {
 u32 pointers[14],choices[14],mecha;bool active;
 explicit NativeEquipmentScope(unsigned player):active(native_equipment_ready){
  if(!active)return;mecha=U(0x86AA14);U(0x86AA14)=launcher_config[2+player];
  for(unsigned slot=0;slot<14;slot++){
   pointers[slot]=U(0x86AA2C+4*slot);U(0x86AA2C+4*slot)=(u32)native_equipment_records[player][slot];
   choices[slot]=U(0x87347C+4*slot);U(0x87347C+4*slot)=native_equipment_records[player][slot<12?(slot%3)*4+slot/3:slot][0];
  }
 }
 ~NativeEquipmentScope(){if(!active)return;for(unsigned slot=0;slot<14;slot++){U(0x86AA2C+4*slot)=pointers[slot];U(0x87347C+4*slot)=choices[slot];}U(0x86AA14)=mecha;}
};
extern "C" u32 __cdecl native_equipment_enter(u32 fighter){
 if(!native_equipment_ready)return 0;unsigned player=fighter==U(0x86AC54)?1:0;
 if(fighter!=U(0x86AC48)&&fighter!=U(0x86AC54))return 0;
 u32 selected=launcher_config[2+player],actual=U(fighter+6308);
 if(actual!=selected&&!((selected==19||selected==22||selected==48)&&actual==selected+1))return 0;
 return (u32)new NativeEquipmentScope(player);
}
extern "C" void __cdecl native_equipment_leave(u32 scope){delete (NativeEquipmentScope*)scope;}
static u32 __cdecl equipment_spawn_p2(int side,int mecha,int flags,int x,int y,int z,int angle){
 typedef u32 (__cdecl *Spawn)(int,int,int,int,int,int,int);
 if(!native_equipment_ready||(u32)mecha!=launcher_config[3])return ((Spawn)0x7D85B2)(side,mecha,flags,x,y,z,angle);
 NativeEquipmentScope scope(1);
 u32 fighter=((Spawn)0x5DD5D0)(side,mecha,flags,x,y,z,angle);
 if(fighter){
  if(!native_p2_moves_loaded){
   typedef int (__thiscall *LoadMove)(void*,u32,int,int);
   // The pool API also loads the native paired transformation assets.
   for(auto& record:native_equipment_records[1])if(record[0]!=equipment_empty)((LoadMove)0x656DF0)((void*)U(0x86AAC4),mecha,record[0],record[1]);
   native_p2_moves_loaded=true;
  }
  typedef void (__thiscall *Human)(void*,int);
  ((Human)0x629330)((void*)U(fighter+344),1);
  // Enemy construction applied CPU level bonuses before human conversion.
  // Recompute that category for its controller owner, then refresh stocks.
  u32 parameters=U(U(fighter+344)+124);
  ((int (__thiscall *)(void*))0x620A50)((void*)parameters);
  ((int (__thiscall *)(void*))0x617990)((void*)fighter);
 }
 return fighter;
}
static bool install_equipment_spawn(){
 u32 call=0x5FBB99;if(*(BYTE*)call!=0xE8||call+5+*(LONG*)(call+1)!=0x7D85B2)return false;
 DWORD old,ignored;if(!VirtualProtect((void*)call,5,PAGE_EXECUTE_READWRITE,&old))return false;
 *(LONG*)(call+1)=(LONG)((BYTE*)equipment_spawn_p2-(BYTE*)(call+5));
 FlushInstructionCache(GetCurrentProcess(),(void*)call,5);VirtualProtect((void*)call,5,old,&ignored);return true;
}
