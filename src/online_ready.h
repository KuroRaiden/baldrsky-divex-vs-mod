#pragma once
#include "online_protocol.h"
#include <optional>

namespace online {
struct ControlPacket {Kind kind;std::vector<uint8_t> bytes;};
struct ReadySnapshot {
 bool connected=false,localReady=false,peerReady=false,pending=false,starting=false,started=false;
 uint32_t localVersion=0,peerVersion=0;
};
// Runs under the mailbox mutex. Only one readiness change may be in flight.
// Start is bound to the exact two acknowledged ready revisions, so an old
// ready/start datagram cannot start a lobby which has since been unchecked.
class ReadyCheck {
 unsigned role_;ReadySnapshot state_;uint32_t acknowledged_=0;
 uint64_t revision_=0,lastReady_=0,lastStart_=0;bool sentReady_=false,sentStart_=false;
 std::vector<uint8_t> start_;
 static ControlPacket ack(Kind kind,uint32_t value){ControlPacket out{kind,{}};put32(out.bytes,value);return out;}
 std::vector<uint8_t> versions()const{
  std::vector<uint8_t> out;put32(out,role_?state_.peerVersion:state_.localVersion);put32(out,role_?state_.localVersion:state_.peerVersion);return out;
 }
public:
 explicit ReadyCheck(unsigned role):role_(role){}
 ReadySnapshot snapshot()const{return state_;}
 uint64_t revision()const{return revision_;}
 void connected(bool value){
  if(state_.connected==value)return;state_={};state_.connected=value;acknowledged_=0;
  sentReady_=sentStart_=false;start_.clear();revision_++;
 }
 bool set_ready(bool value){
  if(!state_.connected||state_.starting||state_.started||state_.pending||state_.localReady==value||state_.localVersion==UINT32_MAX)return false;
  state_.localReady=value;state_.localVersion++;state_.pending=true;sentReady_=false;revision_++;return true;
 }
 std::optional<ControlPacket> receive(Kind kind,const uint8_t* data,size_t size){
  if(!state_.connected||!data)return {};
  if(kind==Kind::Ready){
   if(size!=8)return {};uint32_t version=get32(data),value=get32(data+4);
   if(!version||value>1)return {};
   if(version==state_.peerVersion&&bool(value)!=state_.peerReady)return {};
   if(version>state_.peerVersion){
    if(state_.started||state_.peerVersion==UINT32_MAX||version!=state_.peerVersion+1)return {};
    state_.peerVersion=version;state_.peerReady=value!=0;
    // The guest may have unchecked before receiving our start proposal.
    state_.starting=false;sentStart_=false;start_.clear();revision_++;
   }
   return ack(Kind::ReadyAck,version);
  }
  if(kind==Kind::ReadyAck){
   if(size==4&&sentReady_&&get32(data)==state_.localVersion&&acknowledged_!=state_.localVersion){
    acknowledged_=state_.localVersion;state_.pending=false;revision_++;
   }return {};
  }
  if(size!=8||!state_.localReady||!state_.peerReady)return {};
  auto expected=versions();if(memcmp(data,expected.data(),8))return {};
  if(kind==Kind::Start&&role_==1){
   if(!state_.started){
    // A matching host start also proves receipt of our current ready packet.
    acknowledged_=state_.localVersion;state_.pending=false;state_.started=true;revision_++;
   }
   return ControlPacket{Kind::StartAck,std::move(expected)};
  }
  if(kind==Kind::StartAck&&role_==0&&state_.starting){state_.starting=false;state_.started=true;revision_++;}
  return {};
 }
 std::optional<ControlPacket> next_packet(uint64_t now){
  if(!state_.connected||state_.started)return {};
  if(role_==0&&!state_.starting&&state_.localReady&&state_.peerReady&&!state_.pending){
   state_.starting=true;start_=versions();sentStart_=false;revision_++;
  }
  if(state_.starting){
   if(sentStart_&&now-lastStart_<250)return {};sentStart_=true;lastStart_=now;return ControlPacket{Kind::Start,start_};
  }
  if(state_.pending){
   if(sentReady_&&now-lastReady_<250)return {};sentReady_=true;lastReady_=now;
   ControlPacket out{Kind::Ready,{}};put32(out.bytes,state_.localVersion);put32(out.bytes,state_.localReady?1:0);return out;
  }return {};
 }
};
}
