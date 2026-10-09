#pragma once
#include <cstring>
#include "split_screen_rules.h"
// Experimental: rendering only. Never advance combat, inputs, or the clock.
static SplitScreenState split_state;
static bool split_rendering=false;
static int split_x[2],split_y[2];
extern "C" __declspec(dllexport) volatile u32 split_screen_status[4]={0,0,0,0};
#ifdef VS_NATIVE_START
extern "C" u32 countdown(void);
static void native_countdown_overlay(u32 surface){
 u32 digit=countdown(),cursor=U(0x86AC78);
 if(!digit||digit>3||!cursor||*(BYTE*)0x874B98)return;
 u32 resource=U(cursor+68);if(!resource)return;
 typedef int (__thiscall *Resolve)(void*,int,int,int);
 typedef void (__thiscall *Sprite)(void*,u32,int,int,int,int,int);
 int index=((Resolve)0x419E30)((void*)resource,1,digit,0);
 if(index<0)return;
 u32 begin=U(resource+24),end=U(resource+28);
 if(!begin||(u32)index>=(end-begin)/88)return;
 int width=(int)U(begin+88*index+28),height=(int)U(begin+88*index+32);
 int x=((int)U(surface+32)+(int)U(surface+40)-width)/2;
 int y=(int)U(surface+36)+((int)U(surface+44)-(int)U(surface+36))/3-height/2;
 ((Sprite)0x70F800)((void*)resource,surface,index,x,y,0,144);
 int front=((Resolve)0x419E30)((void*)resource,2,digit,0);
 if(front>=0)((Sprite)0x70F800)((void*)resource,surface,front,x,y,0,256);
}
#endif
typedef int (__thiscall *PixelPosition)(void*);
typedef void (__thiscall *RenderScene)(void*,void*,int,int,int,int);
typedef void (__stdcall *RenderWorld)(u32,int,u32,int,int,int,int);
static void split_fighter_center(u32 fighter,int& x,int& y){
 x=((PixelPosition)U(U(fighter)+76))((void*)fighter);
 y=((PixelPosition)U(U(fighter)+80))((void*)fighter)
   -(int)U(fighter+168)-(int)U(fighter+5680)-(int)U(fighter+5684)/2;
}
static void split_counter(u32 address,int value){
 U(address+4)=0;U(address+8)=1;U(address+12)=1;U(address+20)=0;U(address+24)=value;
}
static void __fastcall split_scene(void* manager,void*,void* surface,int x,int y,int hud,int effects){
 u32 p1=U(0x86AC48),p2=U(0x86AC54),root=U(0x877674);
 bool eligible=U(0x86AA1C)==1 && U(0x86AA18)==2 && p1 && p2 && root
   && U(p2+6224)==0 && !U(0xAE45E8) && !U(0xAE45F4) && !U(0xAE45DC)
   && !*(volatile BYTE*)0x874B90 && !U(0x873464);
 if(eligible){split_fighter_center(p1,split_x[0],split_y[0]);split_fighter_center(p2,split_x[1],split_y[1]);}
 split_state.update(split_x[0],split_y[0],split_x[1],split_y[1],eligible);
 split_screen_status[0]=split_state.active;split_screen_status[1]++;
 if(!split_state.active){((RenderScene)0x740150)(manager,surface,x,y,hud,effects);return;}
 // Render at native scale to avoid the small shared zoom's repeating buffers.
 BYTE savedX[28],savedY[28];
 std::memcpy(savedX,(void*)(root+692),28);std::memcpy(savedY,(void*)(root+720),28);
 split_counter(root+692,256);split_counter(root+720,256);
 split_rendering=true;
 ((RenderScene)0x740150)(manager,surface,x,y,hud,effects);
 split_rendering=false;
 std::memcpy((void*)(root+692),savedX,28);std::memcpy((void*)(root+720),savedY,28);
}
static void split_set_clip(u32 surface,SplitRect r){
 typedef void (__thiscall *Clip)(void*,int,int,int,int);
 ((Clip)U(U(surface)+320))((void*)surface,r.left,r.top,r.right,r.bottom);
}
static void split_draw_world(u32 manager,int mask,u32 surface,int cameraX,int cameraY,int offsetX,int offsetY,SplitRect view){
 // Native render list (manager+56), sorted once by RenderScene. Each node's
 // draw method is vtable+4; vtable+0 belongs to the simulation update.
 u32 head=U(manager+76),node=head?U(head):0;
 unsigned visited=0;
 while(node && node!=head && visited++<100000){
  u32 next=U(node),object=U(node+8);
  if(object){
   typedef int (__thiscall *Mask)(void*);
   if(((Mask)U(U(object)+108))((void*)object)&mask){
    SplitRect own={};
    typedef void (__thiscall *ObjectClip)(void*,SplitRect*);
    ((ObjectClip)U(U(object)+164))((void*)object,&own);
    if(own.left||own.top||own.right||own.bottom){
     own.left+=(int)U(manager+164);own.right+=(int)U(manager+164);
     own.top+=(int)U(manager+168);own.bottom+=(int)U(manager+168);
     own=intersect_split_rect(view,own);
    }else own=view;
    int divisorX=(int)U(object+244),divisorY=(int)U(object+248);
    if(!split_rect_empty(own) && divisorX && divisorY){
     split_set_clip(surface,own);
     // Apply layer scroll ratios around the viewport origin, so slow scenery
     // on the right starts at that half's edge rather than the full screen's.
     int drawX=(int)U(manager+164)+view.left+(offsetX&(int)U(object+252))
       -(int)(((int64_t)cameraX+view.left)*(int)U(object+236)/divisorX)-(int)U(object+228);
     int drawY=(int)U(manager+168)+view.top+(offsetY&(int)U(object+252))
       -(int)(((int64_t)cameraY+view.top)*(int)U(object+240)/divisorY)-(int)U(object+232);
     typedef void (__thiscall *Draw)(void*,void*,int,int);
     ((Draw)U(U(object)+4))((void*)object,(void*)surface,drawX,drawY);
    }
   }
  }
  node=next;
 }
 split_set_clip(surface,view);
}
static void __stdcall split_world(u32 manager,int mask,u32 surface,int x,int y,int offsetX,int offsetY){
 if(!split_rendering){
  ((RenderWorld)0x73F6A0)(manager,mask,surface,x,y,offsetX,offsetY);
#ifdef VS_NATIVE_START
  native_countdown_overlay(surface);
#endif
  return;
 }
 SplitRect original={(int)U(surface+32),(int)U(surface+36),(int)U(surface+40),(int)U(surface+44)};
 int middle=original.left+(original.right-original.left)/2;
 SplitRect views[2]={{original.left,original.top,middle,original.bottom},{middle,original.top,original.right,original.bottom}};
 u32 root=U(0x877674);
 BYTE savedCameraX[28],savedCameraY[28];
 std::memcpy(savedCameraX,(void*)(root+992),28);
 std::memcpy(savedCameraY,(void*)(root+1020),28);
 for(unsigned side=0;side<2;side++){
  unsigned player=side==0?split_state.leftPlayer:1-split_state.leftPlayer;
  SplitRect view=views[side];if(split_rect_empty(view))continue;
  int centerX=view.left+(view.right-view.left)/2,centerY=view.top+(view.bottom-view.top)/2;
  int cameraX=split_x[player]-centerX+(int)U(manager+164);
  int cameraY=split_y[player]-centerY+(int)U(manager+168);
  // Tile culling and some scenery read the root camera directly rather than
  // their Draw arguments. Give them this pass's camera; simulation never runs
  // here, and the complete counter records are restored before HUD rendering.
  split_counter(root+992,cameraX);split_counter(root+1020,cameraY);
  split_set_clip(surface,view);
  split_draw_world(manager,mask,surface,cameraX,cameraY,offsetX,offsetY,view);
  split_screen_status[player+2]++;
 }
 std::memcpy((void*)(root+992),savedCameraX,28);
 std::memcpy((void*)(root+1020),savedCameraY,28);
 split_set_clip(surface,original);
 // Make the active split obvious even when both cameras show similar terrain.
 typedef void (__thiscall *FillRect)(void*,int,int,int,int,u32,u32,int);
 ((FillRect)U(U(surface)+120))((void*)surface,middle-2,original.top,middle+2,original.bottom,0,0,255);
#ifdef VS_NATIVE_START
 native_countdown_overlay(surface);
#endif
}
static bool split_hook_matches(u32 call,u32 original){
 return *(BYTE*)call==0xE8 && (u32)(call+5+*(LONG*)(call+1))==original;
}
static bool split_install_call(u32 call,void* wrapper){
 DWORD previous=0;if(!VirtualProtect((void*)call,5,PAGE_EXECUTE_READWRITE,&previous))return false;
 *(LONG*)(call+1)=(LONG)((BYTE*)wrapper-(BYTE*)(call+5));
 FlushInstructionCache(GetCurrentProcess(),(void*)call,5);
 DWORD ignored=0;VirtualProtect((void*)call,5,previous,&ignored);return true;
}
static bool split_install(){
 if(!split_hook_matches(0x787937,0x740150)||!split_hook_matches(0x740E8F,0x73F6A0))return false;
 return split_install_call(0x787937,(void*)split_scene) && split_install_call(0x740E8F,(void*)split_world);
}
