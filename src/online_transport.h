#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdlib>
#include <functional>
#include "online_protocol.h"
#include "online_chat.h"
#include "online_identity.h"
#include <gekkonet.h>

namespace online {
inline bool parse_address(const std::string& text,sockaddr_in& out){
 auto colon=text.find(':');if(colon==std::string::npos||colon!=text.rfind(':'))return false;
 auto host=text.substr(0,colon),port=text.substr(colon+1);if(port.empty()||port.size()>5)return false;
 unsigned n=0;for(char c:port){if(c<'0'||c>'9')return false;n=n*10+c-'0';}if(!n||n>65535)return false;
 sockaddr_in a={};a.sin_family=AF_INET;a.sin_port=htons(u_short(n));
 if(InetPtonA(AF_INET,host.c_str(),&a.sin_addr)!=1||!a.sin_addr.s_addr||a.sin_addr.s_addr==INADDR_BROADCAST
    ||(ntohl(a.sin_addr.s_addr)>>28)==14)return false;
 out=a;return true;
}
inline bool same_address(const sockaddr_in& a,const sockaddr_in& b){return a.sin_addr.s_addr==b.sin_addr.s_addr&&a.sin_port==b.sin_port;}

class Transport {
 SOCKET socket_=INVALID_SOCKET;bool wsa_=false,pinned_=false,hello_=false,ack_=false;
 sockaddr_in peer_={};unsigned role_=0;Token token_={};std::vector<uint8_t> manifest_;
 uint64_t lastHello_=0,lastReceive_=0;std::vector<GekkoNetResult*> received_;
 std::string error_;bool left_=false,libraryOwnsResults_=false;
 std::shared_ptr<ChatMailbox> chat_;
 std::string localName_,peerName_;
 std::vector<uint8_t> hello()const{auto bytes=manifest_;bytes.insert(bytes.end(),localName_.begin(),localName_.end());return bytes;}
 inline static thread_local Transport* active_=nullptr;
 GekkoNetAdapter adapter_={send_callback,receive_callback,free_callback};
 static void send_callback(GekkoNetAddress*,const char* data,int length){if(active_&&length>0)active_->send(Kind::Data,(const uint8_t*)data,size_t(length));}
 static GekkoNetResult** receive_callback(int* count){
  if(!active_){*count=0;return nullptr;}active_->poll();*count=int(active_->received_.size());active_->libraryOwnsResults_=true;return active_->received_.data();
 }
 static void free_callback(void* p){free(p);}
 void release_results(){if(!libraryOwnsResults_)for(auto* p:received_){free(p->addr.data);free(p->data);free(p);}received_.clear();libraryOwnsResults_=false;}
public:
 Transport()=default;Transport(const Transport&)=delete;Transport& operator=(const Transport&)=delete;
 ~Transport(){close();}
 bool open(unsigned role,unsigned port,const std::string& peer,const Token& token,const Manifest& manifest,const std::string& name="Player"){
  close();error_.clear();left_=false;hello_=ack_=false;lastHello_=0;
  if(role>1||port>65535||!port||!manifest.valid()||!valid_player_name(name)){error_="Invalid online settings or player name.";return false;}
  localName_=name;peerName_.clear();hasResult=false;remoteFrame=remoteChecksum=0;
  if(role==1&&!parse_address(peer,peer_)){error_="Enter an IPv4 address and port, such as 127.0.0.1:14764.";return false;}
  pinned_=role==1;role_=role;token_=token;manifest_=manifest.bytes();
  WSADATA ws={};if(WSAStartup(MAKEWORD(2,2),&ws)){error_="Windows networking could not start.";return false;}wsa_=true;
  socket_=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(socket_==INVALID_SOCKET){error_="Could not create the UDP socket.";close();return false;}
  sockaddr_in local={};local.sin_family=AF_INET;local.sin_port=htons(u_short(port));local.sin_addr.s_addr=INADDR_ANY;
  if(bind(socket_,(sockaddr*)&local,sizeof(local))){error_="The local port is already in use or unavailable.";close();return false;}
  u_long nonblocking=1;if(ioctlsocket(socket_,FIONBIO,&nonblocking)){error_="Could not configure the UDP socket.";close();return false;}
  lastReceive_=GetTickCount64();return true;
 }
 void close(){if(chat_)chat_->connected(false);release_results();if(socket_!=INVALID_SOCKET){closesocket(socket_);socket_=INVALID_SOCKET;}if(wsa_){WSACleanup();wsa_=false;}if(active_==this)active_=nullptr;}
 void chat(std::shared_ptr<ChatMailbox> mailbox){chat_=std::move(mailbox);}
 bool send(Kind kind,const uint8_t* bytes=nullptr,size_t n=0){
  if(socket_==INVALID_SOCKET||!pinned_)return false;auto packet=envelope(kind,role_,token_,bytes,n);if(packet.empty())return false;
  return sendto(socket_,(const char*)packet.data(),int(packet.size()),0,(sockaddr*)&peer_,sizeof(peer_))==int(packet.size());
 }
 void poll(){
  // Gekko frees each returned result. Only the vector of pointers is ours.
  // Control-only polls call discard_results() below before polling again.
  release_results();
  uint64_t now=GetTickCount64();
  if(!lastHello_||now-lastHello_>=250){auto data=hello();send(Kind::Hello,data.data(),data.size());lastHello_=now;}
  for(unsigned i=0;i<64;i++){
   uint8_t bytes[header_size+max_payload+1];sockaddr_in from={};int length=sizeof(from);
   int n=recvfrom(socket_,(char*)bytes,sizeof(bytes),0,(sockaddr*)&from,&length);
   if(n==SOCKET_ERROR){int e=WSAGetLastError();if(e==WSAEWOULDBLOCK)break;if(e==WSAEMSGSIZE||e==WSAECONNRESET)continue;error_="UDP receive failed.";break;}
   Packet packet={};if(!decode(bytes,size_t(n),token_,packet)||packet.role==role_||(pinned_&&!same_address(from,peer_)))continue;
   if(packet.kind==Kind::Hello||packet.kind==Kind::Ack){
    if(packet.size<=manifest_.size()||packet.size>manifest_.size()+name_max_bytes||memcmp(packet.data,manifest_.data(),manifest_.size())){error_="Peer connection-test build differs.";continue;}
    std::string name((const char*)packet.data+manifest_.size(),packet.size-manifest_.size());
    if(!valid_player_name(name)){error_="The peer sent an invalid player name.";continue;}
    if(!peerName_.empty()&&name!=peerName_){error_="The peer identity changed during this connection.";continue;}
    peerName_=name;if(chat_)chat_->peer_name(name);
    if(!pinned_){peer_=from;pinned_=true;}lastReceive_=now;
    if(packet.kind==Kind::Hello){hello_=true;auto data=hello();send(Kind::Ack,data.data(),data.size());}
    else ack_=true;continue;
   }
   if(!ready())continue;lastReceive_=now;
   if(packet.kind==Kind::Bye){left_=true;continue;}
   if(packet.kind==Kind::Ready||packet.kind==Kind::ReadyAck||packet.kind==Kind::Start||packet.kind==Kind::StartAck){
    if(chat_){auto reply=chat_->receive_lobby(packet.kind,packet.data,packet.size);if(reply)send(reply->kind,reply->bytes.data(),reply->bytes.size());}continue;
   }
   if(packet.kind==Kind::Chat){
    if(chat_){auto id=chat_->receive(packet.data,packet.size);if(id){std::vector<uint8_t> ack;put32(ack,id);send(Kind::ChatAck,ack.data(),ack.size());}}continue;
   }
   if(packet.kind==Kind::ChatAck){if(chat_)chat_->acknowledge(packet.data,packet.size);continue;}
   if(packet.kind==Kind::Result){if(packet.size==8){remoteFrame=get32(packet.data);remoteChecksum=get32(packet.data+4);hasResult=true;}continue;}
   if(packet.kind!=Kind::Data||!packet.size)continue;
   auto result=(GekkoNetResult*)calloc(1,sizeof(GekkoNetResult));if(!result){error_="Network buffer allocation failed.";break;}
   result->addr.data=malloc(sizeof(sockaddr_in));result->data=malloc(packet.size);
   if(!result->addr.data||!result->data){free(result->addr.data);free(result->data);free(result);error_="Network buffer allocation failed.";break;}
   memcpy(result->addr.data,&peer_,sizeof(peer_));memcpy(result->data,packet.data,packet.size);
   result->addr.size=sizeof(peer_);result->data_len=unsigned(packet.size);received_.push_back(result);
  }
  if(chat_){
   chat_->connected(ready()&&!disconnected()&&error_.empty());
   auto control=chat_->next_lobby_packet(now);if(control)send(control->kind,control->bytes.data(),control->bytes.size());
   auto data=chat_->next_packet(now);if(!data.empty())send(Kind::Chat,data.data(),data.size());
  }
 }
 bool hasResult=false;uint32_t remoteFrame=0,remoteChecksum=0;
 void discard_results(){release_results();}
 bool ready()const{return hello_&&ack_;}
 bool disconnected()const{return left_||GetTickCount64()-lastReceive_>10000;}
 const std::string& error()const{return error_;}
 const std::string& peer_name()const{return peerName_;}
 GekkoNetAddress peer_address(){return {(void*)&peer_,sizeof(peer_)};}
 GekkoNetAdapter* adapter(){active_=this;return &adapter_;}
 void report(uint32_t frame,uint32_t checksum){std::vector<uint8_t> b;put32(b,frame);put32(b,checksum);send(Kind::Result,b.data(),b.size());}
};
}
