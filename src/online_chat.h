#pragma once
#include "online_protocol.h"
#include <deque>
#include <mutex>
#include <memory>

namespace online {
constexpr size_t chat_max_bytes=512,chat_history_limit=100,chat_queue_limit=8;
// Accept printable UTF-8, rejecting truncated/overlong encodings, surrogate
// values and embedded control bytes. Chat is rendered as plain text only.
inline bool valid_chat_text(const std::string& text){
 if(text.empty()||text.size()>chat_max_bytes)return false;bool visible=false;
 for(size_t i=0;i<text.size();){
  uint8_t c=uint8_t(text[i++]);uint32_t cp=c;unsigned extra=0;
  if(c<0x80){if(c<32||c==127)return false;visible|=c!=' ';continue;}
  if(c>=0xc2&&c<=0xdf){cp=c&31;extra=1;}
  else if(c>=0xe0&&c<=0xef){cp=c&15;extra=2;}
  else if(c>=0xf0&&c<=0xf4){cp=c&7;extra=3;}
  else return false;
  if(i+extra>text.size())return false;
  for(unsigned j=0;j<extra;j++){uint8_t next=uint8_t(text[i++]);if((next&0xc0)!=0x80)return false;cp=(cp<<6)|(next&63);}
  if((extra==1&&cp<0x80)||(extra==2&&cp<0x800)||(extra==3&&cp<0x10000)
     ||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)||(cp>=0x80&&cp<0xa0))return false;
  visible=true;
 }return visible;
}
enum class ChatDelivery {Pending,Delivered,Failed};
struct ChatLine {uint32_t id;unsigned player;bool local;std::string text;ChatDelivery delivery;};
struct ChatSnapshot {bool connected=false;unsigned player=0;uint64_t revision=0;std::vector<ChatLine> lines;std::string localName,peerName;};
class ChatMailbox {
 struct Pending {uint32_t id;std::string text;uint64_t sent=0;bool attempted=false;};
 mutable std::mutex mutex_;bool connected_=false;unsigned player_;uint32_t next_=1,expected_=1;
 uint64_t revision_=0;std::deque<Pending> outgoing_;std::deque<ChatLine> history_;
 std::string localName_,peerName_;
 void trim(){while(history_.size()>chat_history_limit)history_.pop_front();}
public:
 explicit ChatMailbox(unsigned player=0,std::string name="Player"):player_(player),localName_(std::move(name)){}
 void peer_name(const std::string& name){std::lock_guard<std::mutex> lock(mutex_);if(peerName_!=name){peerName_=name;revision_++;}}
 void connected(bool connected){
  std::lock_guard<std::mutex> lock(mutex_);if(connected_==connected)return;
  connected_=connected;revision_++;
  if(!connected){for(auto& line:history_)if(line.local&&line.delivery==ChatDelivery::Pending)line.delivery=ChatDelivery::Failed;outgoing_.clear();}
 }
 bool send_text(const std::string& text){
  if(!valid_chat_text(text))return false;
  std::lock_guard<std::mutex> lock(mutex_);
  if(!connected_||outgoing_.size()>=chat_queue_limit||next_==UINT32_MAX)return false;
  uint32_t id=next_++;outgoing_.push_back({id,text});history_.push_back({id,player_,true,text,ChatDelivery::Pending});trim();revision_++;return true;
 }
 std::vector<uint8_t> next_packet(uint64_t now){
  std::lock_guard<std::mutex> lock(mutex_);if(!connected_||outgoing_.empty())return {};
  auto& p=outgoing_.front();if(p.attempted&&now-p.sent<250)return {};
  p.attempted=true;p.sent=now;std::vector<uint8_t> data;put32(data,p.id);data.insert(data.end(),p.text.begin(),p.text.end());return data;
 }
 // Return the sequence number to acknowledge. Repeated messages are ACKed
 // again, while their text appears only once and in order in the transcript.
 uint32_t receive(const uint8_t* data,size_t size){
  if(!data||size<5||size>4+chat_max_bytes)return 0;uint32_t id=get32(data);if(!id)return 0;
  std::string text((const char*)data+4,size-4);if(!valid_chat_text(text))return 0;
  std::lock_guard<std::mutex> lock(mutex_);if(id>expected_)return 0;if(id<expected_)return id;
  history_.push_back({id,1-player_,false,std::move(text),ChatDelivery::Delivered});trim();expected_++;revision_++;return id;
 }
 void acknowledge(const uint8_t* data,size_t size){
  if(!data||size!=4)return;uint32_t id=get32(data);std::lock_guard<std::mutex> lock(mutex_);
  if(outgoing_.empty()||outgoing_.front().id!=id||!outgoing_.front().attempted)return;
  outgoing_.pop_front();for(auto& line:history_)if(line.local&&line.id==id){line.delivery=ChatDelivery::Delivered;break;}revision_++;
 }
 ChatSnapshot snapshot()const{
  std::lock_guard<std::mutex> lock(mutex_);return {connected_,player_,revision_,{history_.begin(),history_.end()},localName_,peerName_};
 }
};
}
