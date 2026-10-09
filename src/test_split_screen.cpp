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
 SplitRect left={0,0,400,600},right={400,0,800,600};
 auto a=intersect_split_rect(left,{350,50,500,200});assert(a.left==350&&a.right==400);
 auto b=intersect_split_rect(right,{100,0,200,600});assert(split_rect_empty(b));
 puts("PASS: split/merge thresholds, hysteresis, reset, extreme positions and viewport clipping.");
}
