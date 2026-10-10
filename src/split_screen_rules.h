#pragma once
#include <stdint.h>
#include <cmath>
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

// The normal points from P1 to P2; the divider is perpendicular to it.
// Track player identity through rotation, rather than swapping two fixed sides.
struct SplitOrientation {
 double angle=0;bool initialized=false;
 void reset(){angle=0;initialized=false;}
 void update(int x1,int y1,int x2,int y2,bool active){
  if(!active){reset();return;}
  double dx=(double)x2-x1,dy=(double)y2-y1;
  if(dx==0 && dy==0)return;
  double target=std::atan2(dy,dx);
  if(!initialized){angle=target;initialized=true;return;}
  const double pi=3.14159265358979323846,step=pi/60;
  double delta=std::remainder(target-angle,2*pi);
  angle=std::remainder(angle+(delta>step?step:delta< -step?-step:delta),2*pi);
 }
 double nx()const{return std::cos(angle);}
 double ny()const{return std::sin(angle);}
};
struct SplitPoint {double x,y;};
inline double split_distance(SplitRect view,double nx,double ny,double x,double y){
 return nx*(x-((double)view.left+view.right)/2)+ny*(y-((double)view.top+view.bottom)/2);
}
inline SplitPoint split_anchor(SplitRect view,double nx,double ny,unsigned player){
 SplitPoint input[4]={{(double)view.left,(double)view.top},{(double)view.right,(double)view.top},
                     {(double)view.right,(double)view.bottom},{(double)view.left,(double)view.bottom}},out[8];
 unsigned count=0;double sign=player?1:-1;
 for(unsigned i=0;i<4;i++){
  SplitPoint a=input[i],b=input[(i+1)%4];
  double da=sign*split_distance(view,nx,ny,a.x,a.y),db=sign*split_distance(view,nx,ny,b.x,b.y);
  if(da>=0)out[count++]=a;
  if((da>=0)!=(db>=0)){double t=da/(da-db);out[count++]={a.x+t*(b.x-a.x),a.y+t*(b.y-a.y)};}
 }
 double area=0,x=0,y=0;
 for(unsigned i=0;i<count;i++){
  SplitPoint a=out[i],b=out[(i+1)%count];double cross=a.x*b.y-b.x*a.y;
  area+=cross;x+=(a.x+b.x)*cross;y+=(a.y+b.y)*cross;
 }
 if(std::abs(area)<1e-8)return {((double)view.left+view.right)/2,((double)view.top+view.bottom)/2};
 return {x/(3*area),y/(3*area)};
}
inline int split_pixel_owner(SplitRect view,double nx,double ny,int x,int y){
 double distance=split_distance(view,nx,ny,x+0.5,y+0.5);
 return std::abs(distance)<2?-1:distance<0?0:1;
}
