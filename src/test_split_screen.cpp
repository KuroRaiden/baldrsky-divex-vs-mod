#include <cassert>
#include <climits>
#include <cstdio>
#include "split_screen_rules.h"
int main(){
 SplitScreenState s;
 s.update(0,0,653,333,true);assert(!s.active);
 s.update(0,0,654,0,true);assert(s.active);
 s.update(0,0,600,290,true);assert(s.active);
 s.update(0,0,580,280,true);assert(!s.active);
 s.update(0,0,0,-334,true);assert(s.active);
 s.update(0,0,0,0,false);assert(!s.active);
 s.update(INT_MIN,INT_MIN,INT_MAX,INT_MAX,true);assert(s.active);
 s.reset();assert(!s.active);
 s.update(900,0,0,0,true);assert(s.active&&s.leftPlayer==1);
 s.update(0,0,900,0,true);assert(s.active&&s.leftPlayer==0);
 s.update(0,0,0,400,true);assert(s.leftPlayer==0);
 s.update(10,0,0,400,true);assert(s.leftPlayer==0);
 s.update(17,0,0,400,true);assert(s.leftPlayer==1);
 s.update(-10,0,0,400,true);assert(s.leftPlayer==1);
 s.update(-17,0,0,400,true);assert(s.leftPlayer==0);
 s.update(0,0,0,0,false);assert(!s.active&&s.leftPlayer==0);
 s.update(INT_MAX,0,INT_MIN,0,true);assert(s.leftPlayer==1);
 s.update(INT_MIN,0,INT_MAX,0,true);assert(s.leftPlayer==0);
 SplitRect left={0,0,400,600},right={400,0,800,600};
 auto a=intersect_split_rect(left,{350,50,500,200});assert(a.left==350&&a.right==400);
 auto b=intersect_split_rect(right,{100,0,200,600});assert(split_rect_empty(b));
 SplitRect screen={0,0,800,600};SplitOrientation rotation;
 rotation.update(0,0,1000,0,true);
 assert(std::abs(rotation.nx()-1)<1e-9 && std::abs(rotation.ny())<1e-9);
 auto p1=split_anchor(screen,rotation.nx(),rotation.ny(),0),p2=split_anchor(screen,rotation.nx(),rotation.ny(),1);
 assert(std::abs(p1.x-200)<1e-9 && std::abs(p2.x-600)<1e-9 && std::abs(p1.y-300)<1e-9);
 rotation.update(0,0,0,1000,true);assert(rotation.angle>0&&rotation.angle<0.06);
 for(int i=0;i<40;i++)rotation.update(0,0,0,1000,true);
 p1=split_anchor(screen,rotation.nx(),rotation.ny(),0);p2=split_anchor(screen,rotation.nx(),rotation.ny(),1);
 assert(std::abs(p1.y-150)<1e-6&&std::abs(p2.y-450)<1e-6);
 assert(split_pixel_owner(screen,rotation.nx(),rotation.ny(),400,100)==0);
 assert(split_pixel_owner(screen,rotation.nx(),rotation.ny(),400,500)==1);
 for(int quadrant=0;quadrant<4;quadrant++){
  rotation.reset();rotation.update(0,0,quadrant&1?-1000:1000,quadrant&2?-700:700,true);
  p1=split_anchor(screen,rotation.nx(),rotation.ny(),0);p2=split_anchor(screen,rotation.nx(),rotation.ny(),1);
  assert(split_distance(screen,rotation.nx(),rotation.ny(),p1.x,p1.y)< -2);
  assert(split_distance(screen,rotation.nx(),rotation.ny(),p2.x,p2.y)>2);
  assert(std::abs(p1.x+p2.x-800)<1e-6&&std::abs(p1.y+p2.y-600)<1e-6);
  auto moved=split_anchor({20,30,820,630},rotation.nx(),rotation.ny(),0);
  assert(std::abs(moved.x-p1.x-20)<1e-6&&std::abs(moved.y-p1.y-30)<1e-6);
  unsigned owned[2]={},seam=0;
  for(int y=0;y<600;y++)for(int x=0;x<800;x++){
   int owner=split_pixel_owner(screen,rotation.nx(),rotation.ny(),x,y);
   if(owner<0)seam++;else owned[owner]++;
   assert(owner== -1||owner==0||owner==1);
  }
  assert(owned[0]==owned[1]&&owned[0]+owned[1]+seam==480000&&seam>0&&seam<5000);
 }
 rotation.reset();rotation.update(0,0,-1000,1,true);double before=rotation.angle;
 rotation.update(0,0,-1000,-1,true);assert(std::abs(std::remainder(rotation.angle-before,6.283185307179586))<0.01);
 rotation.update(INT_MIN,INT_MIN,INT_MAX,INT_MAX,true);assert(std::isfinite(rotation.angle));
 rotation.update(0,0,0,0,false);assert(!rotation.initialized);
 puts("PASS: split thresholds, clipping, rotating ownership, horizontal/vertical/diagonal anchors, complete pixel coverage and smooth angle wrap.");
}
