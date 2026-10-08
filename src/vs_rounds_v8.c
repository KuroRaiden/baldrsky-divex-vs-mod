/* Freestanding x86 helper for the exact V6 image. No CRT or new imports. */
#include "round_rules.h"
typedef unsigned int u32;
#ifndef ROUND_ADDRESS_BIAS
#define ROUND_ADDRESS_BIAS 0
#endif
#define G(a) ((a)+ROUND_ADDRESS_BIAS)
#define U(a) (*(volatile u32 *)G(a))
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
static char title[192];
__declspec(dllexport) u32 snapshot(void) {return p1wins|(p2wins<<4)|(finished<<8)|(lastresult<<12);}
static int eligible(void) {
 return U(0x86AA1C)==1 && U(0x86AA18)==2 && U(0x86AC48) && U(0x86AC54)
  && F(U(0x86AC54),0x1850)==0;
}
static int training(void) {return U(0x86AA1C)==1 && U(0x86AA18)==2;}
/* 0 draw, 1 P1, 2 P2. Compare ratios by cross multiplication. */
static void health(void) {
 u32 a=U(0x86AC48),b=U(0x86AC54);
 if(a) {F(a,5652)=1;F(a,5656)=10000;}
 if(b) {F(b,5652)=1;F(b,5656)=10000;}
}
static void show(void) {
 u32 ticks=U(0x8738EC)-start;
 u32 seconds=ticks>=5400 ? 0:90-ticks/60;
 ((Format)G(ROUND_FORMAT_ADDR))(title,
  finished ? "BALDR SKY DiveX | VS P1 %u - %u P2 | P%u WINS MATCH | F1: new match" :
  "BALDR SKY DiveX | VS P1 %u - %u P2 | %02u:%02u | Last round: %s",
  p1wins,p2wins,finished?(p1wins==2?1:2):seconds/60,seconds%60,
  lastresult==1?"P1 wins":lastresult==2?"P2 wins":lastresult==3?"Draw":"--");
 ((WindowText)U(0x7D9328))(U(0x8775F8),title);
}
__declspec(dllexport) int __fastcall setup(void *self,void *unused) {
 int r=((Method)G(0x5FB8C0))(self);
 if(eligible()) {F((u32)self,256)=0;F((u32)self,212)=0;owner=(u32)self;p1wins=0;p2wins=0;finished=0;lastresult=0;
  health();start=lasttick=U(0x8738EC);prior1=U(0x86AC48);prior2=U(0x86AC54);announced=~0u;show();}
 return r;
}
__declspec(dllexport) int __fastcall tick(void *self,void *unused) {
 u32 rewound=U(0x8738EC)<lasttick;
 int r=((Method)G(0x5F8CC0))(self);
 if(!training()) return r;
 /* A KO can clear the global pointer before this update. Never dereference
    retired fighter pointers or treat their disappearance as a roster change. */
 if(!eligible() && (owner!=(u32)self || !prior1 || !prior2)) return r;
 if(eligible() && (owner!=(u32)self || rewound || U(0x8738EC)<lasttick || prior1!=U(0x86AC48) || prior2!=U(0x86AC54))) {
  owner=(u32)self;p1wins=p2wins=finished=lastresult=0;
  health();start=U(0x8738EC);announced=~0u;}
 if(!finished && F((u32)self,256)==0 && !U(0x874B98)) {
  u32 a=U(0x86AC48),b=U(0x86AC54);
  int h1=a?(int)F(a,5656):0,h2=b?(int)F(b,5656):0;
  if(h1<=0 || h2<=0 || U(0x8738EC)-start>=5400) {
   int winner=round_winner(h1,10000,h2,10000);
   if(winner==1) ++p1wins; else if(winner==2) ++p2wins;
   lastresult=winner?winner:3;
   /* Call original directly: automatic rounds preserve the match score. */
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
 return r;
}
/* Borrow the current cursor's font/opacity on a stack copy. The second label
   owns a freshly initialized string and never takes fighter references. */
__declspec(dllexport) void __fastcall cursor(void *self,void *unused,int a2,int a3,int a4) {
 typedef void (__thiscall *Render)(void *,int,int,int);
 ((Render)G(0xCBE411))(self,a2,a3,a4);
 if(!eligible() || !U(0x86AC78) || F(U(0x86AC78),468)!=(u32)self) return;
 u32 first=F((u32)self,264),p1=U(0x86AC48),p2=U(0x86AC54);
 if(first!=p1 && first!=p2) return;
 u32 target=first==p2?p1:p2,observer=first==p2?p2:p1;
 u32 copy[126];
 for(u32 i=0;i<126;i++) ((volatile u32 *)copy)[i]=((volatile u32 *)self)[i];
 for(u32 i=117;i<124;i++) copy[i]=0;
 ((void (__thiscall *)(void *,void *))G(0x4AC310))((void *)F(target,0x1834),&copy[117]);
 copy[66]=target;
 copy[40]=((Method)F(F(target,0),76))((void *)target);
 copy[41]=((Method)F(F(target,0),80))((void *)target)-F(target,168)-F(target,5640)/2;
 copy[46]=copy[122]*8;
 u32 saved=U(0x86AC48);U(0x86AC48)=observer;
 ((Render)G(0x4CC050))(copy,a2,a3,a4);
 U(0x86AC48)=saved;
 ((Method)G(0x41A300))(&copy[117]);
}
/* Disable Training's recovery callback for both human-controlled fighters. */
__declspec(dllexport) void __stdcall recovery(u32 fighter) {
 if(eligible() && (fighter==U(0x86AC48)||fighter==U(0x86AC54))) return;
 ((void (__stdcall *)(u32))G(0x5F8D50))(fighter);
}

