#pragma once
#include <stdint.h>
struct SplitScreenState {
 bool active=false;
 unsigned leftPlayer=0;
 void reset(){active=false;leftPlayer=0;}
 void update(int x1,int y1,int x2,int y2,bool eligible){
  if(!eligible){reset();return;}
  int64_t dx=(int64_t)x1-x2,dy=(int64_t)y1-y2;
  if(dx<0)dx=-dx;if(dy<0)dy=-dy;
  // V6 fits dx+200 in 640 and dy+200 in 400, with zoom >=192/256.
  // Enter once the requested shared zoom would fall below that floor.
  if(!active){
   active=(dx>653 || dy>333);
   if(active)leftPlayer=x1>x2?1:0;
  }
  else if(dx<=580 && dy<=280)active=false;
  else {
   // Keep the current order at near-equal X to avoid flicker during crossings.
   int64_t horizontal=(int64_t)x1-x2;
   if(horizontal>16)leftPlayer=1;
   else if(horizontal< -16)leftPlayer=0;
  }
 }
};
struct SplitRect {int left,top,right,bottom;};
inline SplitRect intersect_split_rect(SplitRect a,SplitRect b){
 return {a.left>b.left?a.left:b.left,a.top>b.top?a.top:b.top,
         a.right<b.right?a.right:b.right,a.bottom<b.bottom?a.bottom:b.bottom};
}
inline bool split_rect_empty(SplitRect r){return r.left>=r.right || r.top>=r.bottom;}
