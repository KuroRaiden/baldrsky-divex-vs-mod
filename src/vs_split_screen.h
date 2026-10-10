#pragma once
#include <cstring>
#include <vector>
#include "split_screen_rules.h"
// Experimental: rendering only. Never advance combat, inputs, or the clock.
static SplitScreenState split_state;
static SplitOrientation split_orientation;
static bool split_rendering=false;
static int split_x[2],split_y[2];
extern "C" __declspec(dllexport) volatile u32 split_screen_status[4]={0,0,0,0};
#ifdef VS_NATIVE_START
extern "C" u32 countdown(void);
extern "C" u32 round_seconds(void);
typedef int (__thiscall *HudResolve)(void*,int,int,int);
static u32 hud_frame(u32 resource,int index){
 u32 begin=U(resource+24),end=U(resource+28);
 return begin && end>=begin && index>=0 && (u32)index<(end-begin)/88?begin+88*index:0;
}
// Keep the original atlas, transparency and per-chunk flips. The native image
// renderer accepts a larger destination rectangle, avoiding new font assets.
static void hud_sprite(u32 resource,u32 surface,int index,int x,int y,int scale,int alpha){
 u32 frame=hud_frame(resource,index);if(!frame)return;
 u32 chunks=U(frame+16),end=U(frame+20),count=U(frame);
 if(!chunks||end<chunks||count>(end-chunks)/56||count>256)return;
 u32 flip=U(frame+52)&0x30000000;
 for(u32 i=0;i<count;i++){
  u32 c=chunks+56*i,texture=U(c),textures=U(resource+72),textureEnd=U(resource+76);
  if(!textures||textureEnd<textures||texture>=(textureEnd-textures)/40)continue;
  typedef void (__thiscall *LoadTexture)(void*,unsigned);
  ((LoadTexture)0x430B80)((void*)resource,texture);
  u32 image=U(textures+40*texture+28);if(!image)continue;
  int left=(int)U(c+4),top=(int)U(c+8),right=(int)U(c+12),bottom=(int)U(c+16);
  int dx=x+scale*((flip&0x10000000)?-right:left);
  int dy=y+scale*((flip&0x20000000)?-bottom:top);
  SplitRect dst={dx,dy,dx+scale*(right-left),dy+scale*(bottom-top)};
  SplitRect src={(int)U(c+20),(int)U(c+24),(int)U(c+28),(int)U(c+32)};
  u32 flags=U(c+44)^flip;
  typedef void (__thiscall *Blit)(void*,u32,SplitRect*,SplitRect*,u32,int);
  ((Blit)U(U(image)+76))((void*)image,surface,&dst,&src,flags,alpha*(int)U(c+48)/256);
 }
}
static void native_round_overlay(u32 surface){
 u32 seconds=round_seconds(),cursor=U(0x86AC78);
 if(seconds==~0u||!cursor||*(BYTE*)0x874B98||U(0xAE45E8)||U(0xAE45F4)||U(0xAE45DC))return;
 u32 resource=U(cursor+68);if(!resource)return;
 HudResolve resolve=(HudResolve)0x419E30;
 u32 digit=countdown();
 if(digit&&digit<=3){
  int shadow=resolve((void*)resource,1,digit,0),front=resolve((void*)resource,2,digit,0);
  u32 frame=hud_frame(resource,shadow);
  if(frame){
   int width=3*(int)U(frame+28),height=3*(int)U(frame+32);
   int x=((int)U(surface+32)+(int)U(surface+40)-width)/2;
   int y=(int)U(surface+36)+((int)U(surface+44)-(int)U(surface+36))/3-height/2;
   hud_sprite(resource,surface,shadow,x,y,3,144);
   hud_sprite(resource,surface,front,x,y,3,256);
  }
 }
 // CComboCommonValue's damage digits: bank 3 shadow, bank 4 gold face.
 // Keep two digits (90 ... 00), centered as a single row, above the bottom edge.
 int ids[2][2],widths[2]={},height=0,total=0;
 for(unsigned i=0;i<2;i++){
  unsigned n=i?seconds%10:seconds/10;
  ids[i][0]=resolve((void*)resource,3,n,0);ids[i][1]=resolve((void*)resource,4,n,0);
  u32 frame=hud_frame(resource,ids[i][0]);if(!frame)return;
  widths[i]=(int)U(frame+28);int h=(int)U(frame+32);if(h>height)height=h;
  total+=widths[i];
 }
 int x=((int)U(surface+32)+(int)U(surface+40)-total)/2;
 int y=(int)U(surface+44)-height-12;
 for(unsigned i=0;i<2;i++){
  hud_sprite(resource,surface,ids[i][0],x,y,1,144);
  hud_sprite(resource,surface,ids[i][1],x,y,1,256);x+=widths[i];
 }
#ifdef VS_NATIVE_EQUIPMENT_AUTOTEST
 static u32 lastDigit=~0u;
 if(digit!=lastDigit){FILE* out=nullptr;if(_wfopen_s(&out,L"equipment-runtime-test.txt",L"ab")==0&&out){
  fprintf(out,"HUD countdown=%u scale=3 timer=%u width=%d height=%d PASS\n",digit,seconds,total,height);fclose(out);
 }lastDigit=digit;}
#endif
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
#ifdef VS_ROTATED_SPLIT_AUTOTEST
 // Temporary renderer test: place the cached draw positions in known layouts.
 // This macro is absent from the release and never changes native physics.
 if(eligible){
  static int baseX=0,baseY=0;if(!baseX){baseX=U(p1+160);baseY=U(p1+164);}
  u32 age=U(0x8738EC);double angle=age<600?0.60:age<900?1.5707963267948966:2.60;
  double radius=age<250||age>1200?80:500;
  int dx=(int)(radius*std::cos(angle)),dy=(int)(radius*std::sin(angle));
  U(p1+160)=baseX-dx;U(p2+160)=baseX+dx;U(p1+164)=baseY-dy;U(p2+164)=baseY+dy;
 }
#endif
 if(eligible){split_fighter_center(p1,split_x[0],split_y[0]);split_fighter_center(p2,split_x[1],split_y[1]);}
 split_state.update(split_x[0],split_y[0],split_x[1],split_y[1],eligible);
 split_orientation.update(split_x[0],split_y[0],split_x[1],split_y[1],split_state.active);
 split_screen_status[0]=split_state.active;split_screen_status[1]++;
 if(!split_state.active){((RenderScene)0x740150)(manager,surface,x,y,hud,effects);
#ifdef VS_NATIVE_START
 if(eligible)native_round_overlay((u32)surface);
#endif
 return;}
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
static bool split_rotated_world(u32 manager,int mask,u32 surface,int offsetX,int offsetY,SplitRect view){
 // v1.00's software surface stores 32-bit pixels at +28, with its pitch in
 // pixels at +52. Confirm the concrete FillRect implementation before using
 // that layout; other surfaces retain the rectangular fallback below.
 if(U(U(surface)+120)!=0x705030)return false;
 u32* pixels=(u32*)U(surface+28);int pitch=(int)U(surface+52),height=(int)U(surface+56);
 if(!pixels || pitch<=0 || pitch>8192 || height<=0 || height>8192
    || view.left<0 || view.top<0 || view.right>pitch || view.bottom>height || split_rect_empty(view))return false;
 static std::vector<u32> base,first;
 size_t width=view.right-view.left,rows=view.bottom-view.top,count=width*rows;
 try{base.resize(count);first.resize(count);}catch(...){return false;}
 for(size_t row=0;row<rows;row++)std::memcpy(base.data()+row*width,pixels+(view.top+row)*pitch+view.left,width*4);
 u32 root=U(0x877674);BYTE savedX[28],savedY[28];
 std::memcpy(savedX,(void*)(root+992),28);std::memcpy(savedY,(void*)(root+1020),28);
 double nx=split_orientation.nx(),ny=split_orientation.ny();
 for(unsigned player=0;player<2;player++){
  if(player)for(size_t row=0;row<rows;row++)std::memcpy(pixels+(view.top+row)*pitch+view.left,base.data()+row*width,width*4);
  SplitPoint anchor=split_anchor(view,nx,ny,player);
  int cameraX=split_x[player]-(int)std::lround(anchor.x)+(int)U(manager+164);
  int cameraY=split_y[player]-(int)std::lround(anchor.y)+(int)U(manager+168);
  split_counter(root+992,cameraX);split_counter(root+1020,cameraY);
  split_set_clip(surface,view);
  split_draw_world(manager,mask,surface,cameraX,cameraY,offsetX,offsetY,view);
  if(!player)for(size_t row=0;row<rows;row++)std::memcpy(first.data()+row*width,pixels+(view.top+row)*pitch+view.left,width*4);
  split_screen_status[player+2]++;
 }
 // Composite after two draw-only passes. No object update, combat or input
 // runs here. The four-pixel seam rotates with the half-plane mask.
 for(int row=view.top;row<view.bottom;row++){
  u32* destination=pixels+row*pitch+view.left;
  const u32* source=first.data()+(row-view.top)*width;
  double distance=split_distance(view,nx,ny,view.left+0.5,row+0.5);
  for(size_t column=0;column<width;column++,distance+=nx){
   if(std::abs(distance)<2)destination[column]=0;
   else if(distance<0)destination[column]=source[column];
  }
 }
 std::memcpy((void*)(root+992),savedX,28);std::memcpy((void*)(root+1020),savedY,28);
#ifdef VS_ROTATED_SPLIT_AUTOTEST
 static int lastLayout=-1;u32 age=U(0x8738EC);int layout=age<600?0:age<900?1:2;
 if(layout!=lastLayout){FILE* log=fopen("split-runtime-test.txt","a");if(log){
  bool restored=std::memcmp((void*)(root+992),savedX,28)==0&&std::memcmp((void*)(root+1020),savedY,28)==0;
  unsigned terrain=0;for(size_t i=0;i<count;i+=1000)if(first[i]!=base[i])terrain++;
  fprintf(log,"COMPOSITE layout=%d restored=%s terrain=%u width=%u height=%u\n",layout,restored?"PASS":"FAIL",terrain,(unsigned)width,(unsigned)rows);fclose(log);
 }lastLayout=layout;}
#endif
 split_set_clip(surface,view);return true;
}
static void __stdcall split_world(u32 manager,int mask,u32 surface,int x,int y,int offsetX,int offsetY){
 if(!split_rendering){
  ((RenderWorld)0x73F6A0)(manager,mask,surface,x,y,offsetX,offsetY);
  return;
 }
 SplitRect original={(int)U(surface+32),(int)U(surface+36),(int)U(surface+40),(int)U(surface+44)};
 if(split_rotated_world(manager,mask,surface,offsetX,offsetY,original)){
  return;
 }
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
