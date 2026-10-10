#pragma once
#include <stdint.h>
#include <array>
constexpr uint32_t equipment_empty=0xffffffffu;
constexpr uint32_t equipment_default=0xfffffffeu;
constexpr unsigned equipment_slots=14;
using EquipmentSlots=std::array<uint32_t,equipment_slots>;
inline EquipmentSlots default_equipment(){EquipmentSlots s;s.fill(equipment_default);return s;}
// Native save order interleaves A/B/C; the human selector groups four ranges.
inline unsigned equipment_save_slot(unsigned slot){return slot<12?(slot%4)*3+slot/4:slot;}
inline bool equipment_type_allowed(unsigned slot,unsigned type){return slot<12?type==0:(type==1||type==2);}
inline bool equipment_unique(const EquipmentSlots& s){
 for(unsigned i=0;i<14;i++)if(s[i]<equipment_default)
  for(unsigned j=i+1;j<14;j++)if(s[i]==s[j])return false;
 return true;
}
struct VSEquipmentConfig {
 uint32_t magic=0x42564531,version=1,p1=0,p2=1,catalogOnly=0;
 EquipmentSlots players[2]={default_equipment(),default_equipment()};
};
static_assert(sizeof(VSEquipmentConfig)==132,"Equipment settings size");
inline bool equipment_shape_valid(const VSEquipmentConfig& c,unsigned p1,unsigned p2){
 if(c.magic!=0x42564531||c.version!=1||c.p1!=p1||c.p2!=p2||c.catalogOnly>1)return false;
 for(auto& s:c.players){if(!equipment_unique(s))return false;for(auto id:s)if(id>=500&&id<equipment_default)return false;}
 return true;
}
