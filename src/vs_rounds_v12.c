/* Freestanding x86 helper for the exact V6 image. No CRT or new imports. */
#include "round_rules.h"
typedef unsigned int u32;
#ifndef ROUND_ADDRESS_BIAS
#define ROUND_ADDRESS_BIAS 0
#endif
#define G(a) ((a)+ROUND_ADDRESS_BIAS)
#define U(a) (*(volatile u32 *)G(a))
#define B(a) (*(volatile unsigned char *)G(a))
#define F(a,o) (*(volatile u32 *)((a)+(o)))
typedef int (__thiscall *Method)(void *);
typedef int (__thiscall *Predicate)(void *,u32);
typedef int (__stdcall *WindowText)(u32,const char *);
typedef int (__cdecl *Format)(char *,const char *,...);
#ifndef ROUND_FORMAT_ADDR
#define ROUND_FORMAT_ADDR 0x676305
#endif
static u32 owner,p1wins,p2wins,start,finished,announced,lastresult;
static u32 lasttick,prior1,prior2;
static u32 second[3];
static char title[192];
static int eligible(void);
/* Battle clock is 60 Hz; movement updates continue throughout preparation. */
static u32 elapsed(void) {u32 now=U(0x8738EC);return now>=start?now-start:0;}
__declspec(dllexport) u32 countdown(void) {u32 t=elapsed();return eligible() && !finished && t<180 ? (180-t+59)/60:0;}
__declspec(dllexport) u32 snapshot(void) {return p1wins|(p2wins<<4)|(finished<<8)|(lastresult<<12);}
__declspec(dllexport) u32 second_cursor(void) {return second[0];}
static int eligible(void) {
 return U(0x86AA1C)==1 && U(0x86AA18)==2 && U(0x86AC48) && U(0x86AC54)
  && F(U(0x86AC54),0x1850)==0;
}
static int training(void) {return U(0x86AA1C)==1 && U(0x86AA18)==2;}
#include "shared_select_v12.h"
static u32 tracker[13],trackerOwner,trackerMode,trackerReady;
__declspec(dllexport) u32 second_tracker(void) {return trackerReady?(u32)tracker:0;}
static void drop_tracker(void) {
 if(!trackerReady)return;
 /* Dispose GUI rows first, then release their native vector references.
    Do not call the manager's reset: it also clears shared attack records. */
 for(u32 p=tracker[4];p<tracker[5];p+=12) {
  u32 row=F(p,0);if(row)((Method)F(F(row,0),44))((void *)row);
 }
 ((Method)G(0x481D20))(tracker+1);
 for(u32 i=0;i<13;i++)tracker[i]=0;
 trackerReady=trackerOwner=trackerMode=0;
}
static void sync_tracker(void) {
 u32 mode=U(0x8734C4);
 if(!eligible()||!U(0x877640)||!U(0x86AAB0)||!F(U(0x86AC54),340)||(mode!=1&&mode!=2)) {drop_tracker();return;}
 if(trackerReady&&(trackerOwner!=U(0x86AC54)||trackerMode!=mode))drop_tracker();
 if(trackerReady)return;
 ((Method)G(0x5E5540))(tracker);
 /* Native HUD coordinates are 800x600, the row extends about 230 pixels. */
 tracker[7]=570;tracker[8]=530;tracker[9]=13;
 trackerOwner=U(0x86AC54);trackerMode=mode;trackerReady=1;
 u32 saved=U(0x86AC48);U(0x86AC48)=trackerOwner;
 ((Method)G(0x5E5E00))(tracker);
 U(0x86AC48)=saved;
}
/* Keep every native movement/combat update, but give P2 their own HUD manager.
   This scopes selection, committed-weapon highlights and combo-history events. */
__declspec(dllexport) int __fastcall fighter_update(void *self,void *unused,unsigned char *state) {
 typedef int (__thiscall *Update)(void *,unsigned char *);
 if(trackerReady&&eligible()&&(u32)self==trackerOwner&&trackerMode==U(0x8734C4)) {
  u32 saved=U(0x86AAB0);U(0x86AAB0)=(u32)tracker;
  int r=((Update)G(0x61E390))(self,state);
  U(0x86AAB0)=saved;return r;
 }
 return ((Update)G(0x61E390))(self,state);
}
__declspec(dllexport) void __fastcall tracker_render(void *self,void *unused,int surface,int x,int y) {
 typedef void (__thiscall *Render)(void *,int,int,int);
 int ours=0;
 if(trackerReady)for(u32 p=tracker[4];p<tracker[5];p+=12)if(F(p,0)==(u32)self){ours=1;break;}
 if(!ours){((Render)G(0x5E7430))(self,surface,x,y);return;}
 if(!eligible()||trackerOwner!=U(0x86AC54))return;
 u32 savedPlayer=U(0x86AC48),savedManager=U(0x86AAB0);
 U(0x86AC48)=trackerOwner;U(0x86AAB0)=(u32)tracker;
 ((Render)G(0x5E7430))(self,surface,x,y);
 U(0x86AC48)=savedPlayer;U(0x86AAB0)=savedManager;
}
/* All 16 native human-state tails call this weapon/Force Crash selector.
   Its -1 result means no attack. Direction, dash and movement code precede it. */
__declspec(dllexport) int __fastcall attack_select(void *self,void *unused) {
 u32 fighter=(u32)self;
 if(owner && eligible() && !finished && elapsed()<180 &&
    (fighter==U(0x86AC48)||fighter==U(0x86AC54))) {
  /* Remove only A/B/C bits from the native 64-frame history. Force Crash
     detects simultaneous A/B/C presses there; countdown presses must not queue. */
  u32 history=F(fighter,5288);
  if(history) for(u32 i=0;i<64;i++) {F(history,4+4*i)&=~0x700u;F(history,260+4*i)&=~0x700u;}
  F(fighter,5548)&=~0x700u;F(fighter,5552)&=~0x700u;
  F(fighter,6480)&=~0x700u;F(fighter,6484)&=~0x700u;
  return -1;
 }
 return ((Method)G(0x61A280))(self);
}
typedef void *(__thiscall *SetRef)(void *,void *);
static void drop_cursor(void) {
 if(second[0]) {
  ((Method)F(F(second[0],0),44))((void *)second[0]);
  ((SetRef)G(0x565670))(second,0);
 }
 second[1]=second[2]=0;
}
static void sync_cursor(void) {
 if(!eligible() || !U(0x86AC78) || !U(0x877640)) {drop_cursor();return;}
 if(second[0] && (F(second[0],264)!=U(0x86AC48) || F(second[0],144)==6)) drop_cursor();
 if(second[0]) return;
 void *memory=((void *(__cdecl *)(u32))G(0x419FF0))(504);
 if(!memory) return;
 u32 saved=U(0x86AC48);U(0x86AC48)=U(0x86AC54);
 void *object=((void *(__thiscall *)(void *,u32,u32))G(0x4CC310))(memory,saved,0);
 U(0x86AC48)=saved;
 object=((void *(__thiscall *)(void *,u32,void *))G(0x4AC3A0))((void *)U(0x877640),U(0x86AC78),object);
 ((SetRef)G(0x565670))(second,object);
}
/* 0 draw, 1 P1, 2 P2. Compare ratios by cross multiplication. */
static void health(void) {
 u32 a=U(0x86AC48),b=U(0x86AC54);
 if(a) {F(a,5652)=1;F(a,5656)=10000;}
 if(b) {F(b,5652)=1;F(b,5656)=10000;}
}
static void show(void) {
 u32 age=elapsed(),ticks=age>=180?age-180:0;
 u32 seconds=ticks>=5400 ? 0:90-ticks/60;
 if(!finished && age<240) {
  if(age<180) ((Format)G(ROUND_FORMAT_ADDR))(title,"BALDR SKY DiveX | VS P1 %u - %u P2 | %u | Move freely - attacks at GO",p1wins,p2wins,(180-age+59)/60);
  else ((Format)G(ROUND_FORMAT_ADDR))(title,"BALDR SKY DiveX | VS P1 %u - %u P2 | GO! | 01:30",p1wins,p2wins);
  ((WindowText)U(0x7D9328))(U(0x8775F8),title);return;
 }
 ((Format)G(ROUND_FORMAT_ADDR))(title,
  finished ? "BALDR SKY DiveX | VS P1 %u - %u P2 | P%u WINS MATCH | F1: new match" :
  "BALDR SKY DiveX | VS P1 %u - %u P2 | %02u:%02u | Last round: %s",
  p1wins,p2wins,finished?(p1wins==2?1:2):seconds/60,seconds%60,
  lastresult==1?"P1 wins":lastresult==2?"P2 wins":lastresult==3?"Draw":"--");
 ((WindowText)U(0x7D9328))(U(0x8775F8),title);
}
__declspec(dllexport) int __fastcall setup(void *self,void *unused) {
 drop_tracker();
 drop_cursor();
 int r=((Method)G(0x5FB8C0))(self);
 if(eligible()) {F((u32)self,256)=0;F((u32)self,212)=0;owner=(u32)self;p1wins=0;p2wins=0;finished=0;lastresult=0;
  health();start=lasttick=U(0x8738EC);prior1=U(0x86AC48);prior2=U(0x86AC54);announced=~0u;show();sync_tracker();}
 return r;
}
__declspec(dllexport) int __fastcall tick(void *self,void *unused) {
 if(trackerReady&&(!eligible()||trackerOwner!=U(0x86AC54)))drop_tracker();
 if(second[0] && (!eligible() || F(second[0],264)!=U(0x86AC48))) drop_cursor();
 u32 rewound=U(0x8738EC)<lasttick;
 int r=((Method)G(0x5F8CC0))(self);
 if(!training()) return r;
 /* A KO can clear the global pointer before this update. Never dereference
    retired fighter pointers or treat their disappearance as a roster change. */
 if(!eligible() && (owner!=(u32)self || !prior1 || !prior2)) return r;
 if(eligible() && (owner!=(u32)self || rewound || U(0x8738EC)<lasttick || prior1!=U(0x86AC48) || prior2!=U(0x86AC54))) {
  owner=(u32)self;p1wins=p2wins=finished=lastresult=0;
  health();start=U(0x8738EC);announced=~0u;}
 if(!finished && elapsed()>=180 && F((u32)self,256)==0 && !B(0x874B98)) {
  u32 a=U(0x86AC48),b=U(0x86AC54);
  int h1=a?(int)F(a,5656):0,h2=b?(int)F(b,5656):0;
  if(h1<=0 || h2<=0 || elapsed()>=5580) {
   int winner=round_winner(h1,10000,h2,10000);
   if(winner==1) ++p1wins; else if(winner==2) ++p2wins;
   lastresult=winner?winner:3;
   /* Call original directly: automatic rounds preserve the match score. */
   drop_cursor();drop_tracker();
   ((Method)G(0x5FB8C0))(self);health();start=U(0x8738EC);announced=~0u;
   if(p1wins==2 || p2wins==2) {
    finished=1;
    /* Native Training menu state, opened by its original update next tick. */
    F((u32)self,256)=1;F((u32)self,212)=0;
   }
  }
 }
 if(announced!=U(0x8738EC)/60 || finished) {announced=U(0x8738EC)/60;show();}
 if(finished && F((u32)self,256)==0) {F((u32)self,256)=1;F((u32)self,212)=0;}
 lasttick=U(0x8738EC);prior1=U(0x86AC48);prior2=U(0x86AC54);
 sync_cursor();
 sync_tracker();
 return r;
}
/* The secondary cursor owns its native range-marker animations and name. */
__declspec(dllexport) void __fastcall cursor(void *self,void *unused,int a2,int a3,int a4) {
 typedef void (__thiscall *Render)(void *,int,int,int);
 if((u32)self==second[0]) {
  if(!eligible()) return;
  u32 saved=U(0x86AC48);U(0x86AC48)=U(0x86AC54);
  ((Render)G(0x4CC050))(self,a2,a3,a4);
  U(0x86AC48)=saved;return;
 }
 ((Render)G(0xCBE411))(self,a2,a3,a4);
}
__declspec(dllexport) int __fastcall cursor_update(void *self,void *unused) {
 if((u32)self!=second[0]) return ((Method)G(0x4CBD50))(self);
 if(!eligible() || F((u32)self,264)!=U(0x86AC48)) {drop_cursor();return 0;}
 u32 saved=U(0x86AC48);U(0x86AC48)=U(0x86AC54);
 int r=((Method)G(0x4CBD50))(self);
 U(0x86AC48)=saved;return r;
}
/* Disable Training's recovery callback for both human-controlled fighters. */
__declspec(dllexport) void __stdcall recovery(u32 fighter) {
 if(eligible() && (fighter==U(0x86AC48)||fighter==U(0x86AC54))) return;
 ((void (__stdcall *)(u32))G(0x5F8D50))(fighter);
}


