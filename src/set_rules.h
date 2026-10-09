#pragma once
#include <stdint.h>
inline bool valid_set_preset(uint32_t wins){return wins==0||wins==2||wins==3||wins==5;}
inline bool set_complete(uint32_t p1,uint32_t p2,uint32_t wins){return wins!=0&&(p1>=wins||p2>=wins);}
