#pragma once
// Only compiled into a temporary integration-test DLL, never the release.
static void equipment_runtime_test(){
 if(launcher_config[5]!=3)return;
 u32 wins=snapshot()&15,age=U(0x8738EC);static u32 lastRound=~0u;static bool attacked[2]={};
 FILE* out=nullptr;
 if(wins!=lastRound){
  if(_wfopen_s(&out,L"equipment-runtime-test.txt",L"ab")==0&&out){
   fprintf(out,"ROUND %u\n",wins);
   for(unsigned p=0;p<2;p++){
    u32 fighter=U(p?0x86AC54:0x86AC48),eq=fighter?U(fighter+340):0;
    bool good=eq&&U(eq+36)-U(eq+32)==56;
    if(good)for(unsigned s=0;s<14;s++){
     u32 row=U(U(eq+32)+4*s),id=native_equipment_records[p][s][0];
     if(U(row+80)!=id||(id!=equipment_empty&&!U(row+76)))good=false;
    }
    fprintf(out,"P%u slots=%s dash=%u\n",p+1,good?"PASS":"FAIL",fighter?U(fighter+6444):0);
   }fclose(out);
  }lastRound=wins;
 }
 for(unsigned p=0;p<2;p++){
  u32 fighter=U(p?0x86AC54:0x86AC48);
  if(fighter&&(int)U(fighter+6364)>=0&&!attacked[p]){
   attacked[p]=true;if(_wfopen_s(&out,L"equipment-runtime-test.txt",L"ab")==0&&out){fprintf(out,"P%u ATTACK %u\n",p+1,U(fighter+6364));fclose(out);}
  }
 }
 if(countdown()==0){
  u32 button=0x100u<<((age/90)%3),press=age%90==0?button:0;
  U(0x874BE0)=press;U(0x874BDC)=0;
  U(0x874BD8)=press;U(0x874BD4)=0;
#ifdef VS_ROTATED_SPLIT_AUTOTEST
  static DWORD inspectionStart=0;
  if(age>=300&&!inspectionStart)inspectionStart=GetTickCount();
  if(inspectionStart&&GetTickCount()-inspectionStart<20000&&age>=300)U(0x8738EC)=300;
  if(age%60==0&&_wfopen_s(&out,L"split-runtime-test.txt",L"ab")==0&&out){
   fprintf(out,"age=%u active=%d angle=%.3f P1=%d,%d P2=%d,%d\n",age,split_state.active,split_orientation.angle,split_x[0],split_y[0],split_x[1],split_y[1]);
   fclose(out);
  }
#endif
  if(age>
#ifdef VS_ROTATED_SPLIT_AUTOTEST
     1400
#else
     900
#endif
     &&U(0x86AC54))U(U(0x86AC54)+5656)=0;
 }
}
