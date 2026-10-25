#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "launcher_protocol.h"
#include "equipment_rules.h"

namespace online {
constexpr uint32_t wire_version=4, input_schema=1;
constexpr size_t max_payload=1152, header_size=28;
using Token=std::array<uint8_t,16>;
using Digest=std::array<uint8_t,32>;
enum class Kind:uint8_t {Hello=1,Ack=2,Data=3,Bye=4,Result=5,Chat=6,ChatAck=7,Ready=8,ReadyAck=9,Start=10,StartAck=11};
inline void put32(std::vector<uint8_t>& b,uint32_t n){for(unsigned i=0;i<4;i++)b.push_back(uint8_t(n>>(8*i)));}
inline uint32_t get32(const uint8_t* b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
struct Packet {Kind kind;unsigned role;const uint8_t* data;size_t size;};
inline std::vector<uint8_t> envelope(Kind kind,unsigned role,const Token& token,const uint8_t* data,size_t size){
 if(role>1||size>max_payload||(!data&&size))return {};
 std::vector<uint8_t> out={'B','V','O','N',uint8_t(wire_version),uint8_t(kind),uint8_t(role),0};
 out.insert(out.end(),token.begin(),token.end());put32(out,uint32_t(size));
 if(size)out.insert(out.end(),data,data+size);return out;
}
inline bool decode(const uint8_t* data,size_t size,const Token& token,Packet& out){
 if(!data||size<header_size||size>header_size+max_payload||memcmp(data,"BVON",4)||data[4]!=wire_version
    ||data[5]<1||data[5]>11||data[6]>1||data[7]||memcmp(data+8,token.data(),16))return false;
 uint32_t n=get32(data+24);if(n!=size-header_size)return false;
 out={Kind(data[5]),data[6],data+header_size,n};return true;
}
inline bool token_from_text(const std::string& text,Token& out){
 if(text.size()!=32)return false;Token result={};
 auto hex=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;};
 for(unsigned i=0;i<16;i++){int a=hex(text[2*i]),b=hex(text[2*i+1]);if(a<0||b<0)return false;result[i]=uint8_t(a*16+b);}
 bool nonzero=false;for(auto c:result)nonzero|=c!=0;if(!nonzero)return false;out=result;return true;
}
inline std::string token_text(const Token& token){std::string out;for(auto c:token){out+="0123456789abcdef"[c>>4];out+="0123456789abcdef"[c&15];}return out;}

struct Manifest {
 uint32_t p1=0,p2=1,stage=1,target=2,music=0xffffffffu,seed=0x42565331;
 EquipmentSlots equipment[2]={default_equipment(),default_equipment()};
 Digest build={},catalog={};
 bool valid()const{
  VSConfig c={0x42565331,1,p1,p2,stage,1,0,target};
  if(!valid_config(c)||!valid_music(music))return false;
  for(auto& slots:equipment){if(!equipment_unique(slots))return false;
   for(auto id:slots)if(id!=equipment_empty&&id>=500)return false; // No machine-local defaults online.
  }return true;
 }
 std::vector<uint8_t> bytes()const{
  std::vector<uint8_t> out;for(auto v:{input_schema,p1,p2,stage,target,music,seed})put32(out,v);
  for(auto& slots:equipment)for(auto id:slots)put32(out,id);
  out.insert(out.end(),build.begin(),build.end());out.insert(out.end(),catalog.begin(),catalog.end());return out;
 }
};
// Inputs are fixed-width button masks, never pointers or native object layouts.
struct Input {uint32_t held=0;};
constexpr uint32_t gameplay_buttons=0xfff;
inline bool valid_input(Input i){return !(i.held&~gameplay_buttons);}
// Explicit prerequisite: the shipping game does not yet have a proven snapshot
// and draw-free replay bridge. Connection success must never enable combat.
constexpr bool native_rollback_verified=false;
// The current probe does not simulate selected mechas/equipment. Keep its
// handshake independent of optional game files so players can connect/chat
// before the native game is installed. A future combat bridge needs its own
// validated match agreement; switching that on must revisit this gate.
inline Manifest connection_test_manifest(){
 static_assert(!native_rollback_verified,"Implement native match agreement before enabling online combat");
 Manifest m;for(auto& slots:m.equipment)slots.fill(equipment_empty);
 constexpr char build[]="BALDR-CONNECTION-PROBE-v4";
 memcpy(m.build.data(),build,sizeof(build)-1);return m;
}
}
