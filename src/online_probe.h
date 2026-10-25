#pragma once
#include "online_transport.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace online {
// This small, fully serializable simulation tests the networking integration.
// It is deliberately separate from BALDR SKY's unsolved native snapshot bridge.
struct ProbeState {
 uint32_t frame=0,rng=0x42565331,x[2]={100,700},hp[2]={10000,10000};
 void advance(const Input* in){
  for(unsigned p=0;p<2;p++){
   x[p]+=(in[p].held&1)?3:0;x[p]-=(in[p].held&2)?3:0;
   rng=rng*1664525u+1013904223u;
   if(in[p].held&0x100)hp[1-p]-=rng%7;
  }frame++;
 }
 uint32_t checksum()const{
  uint32_t hash=2166136261u;for(uint32_t n:{frame,rng,x[0],x[1],hp[0],hp[1]})for(unsigned i=0;i<4;i++){hash^=uint8_t(n>>(8*i));hash*=16777619u;}return hash;
 }
};
inline Input probe_input(unsigned role,unsigned frame){return {((frame/17+role)%2?1u:2u)|((frame+role*7)%13==0?0x100u:0u)};}
struct ProbeProgress {std::string message="Idle";unsigned frame=0,rollbacks=0;float ping=0;bool success=false,done=false;};
struct ProbeOptions {unsigned role=0,port=14764,frames=600;std::string peer;Token token={};Manifest match;bool punch=false;std::wstring punchPath;};
inline std::atomic_bool punch_active=false;
inline bool initialize_punch(const std::wstring& path,std::string& error){
 // Our build keeps DllMain inert and starts AutoPunch explicitly, outside the
 // loader lock. Retain it until process exit; callbacks belong to this process.
 static HMODULE module=nullptr;
 if(punch_active)return true;
 module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!module){error="AutoPunch helper is missing or could not load.";return false;}
 auto start=(BOOL(WINAPI*)())GetProcAddress(module,"StartAutoPunch");
 if(!start||!start()){error="AutoPunch could not initialize.";return false;}punch_active=true;return true;
}
inline ProbeProgress run_probe(const ProbeOptions& options,std::atomic_bool& cancel,const std::function<void(const ProbeProgress&)>& update={}){
 ProbeProgress result;auto publish=[&](const std::string& s){result.message=s;if(update)update(result);};
 if(options.punch&&!initialize_punch(options.punchPath,result.message)){result.done=true;return result;}
 Transport transport;
 if(!transport.open(options.role,options.port,options.peer,options.token,options.match)){result.message=transport.error();result.done=true;return result;}
 publish(options.role?"Connecting to host...":"Hosting: waiting for the peer...");
 uint64_t begin=GetTickCount64();
 while(!cancel&&!transport.ready()&&GetTickCount64()-begin<15000){
  transport.poll();transport.discard_results();if(!transport.error().empty()){result.message=transport.error();result.done=true;return result;}Sleep(2);
 }
 if(cancel||!transport.ready()){result.message=cancel?"Connection cancelled.":"No matching peer replied. Check address, port and room key.";result.done=true;return result;}
 GekkoSession* session=nullptr;
 if(!gekko_create(&session,GekkoGameSession)){result.message="Could not create the GekkoNet session.";result.done=true;return result;}
 struct Cleanup{GekkoSession** s;~Cleanup(){gekko_destroy(s);}} cleanup{&session};
 GekkoConfig config={};config.num_players=2;config.input_size=sizeof(Input);config.state_size=sizeof(ProbeState);
 config.max_input_prediction_window=8;config.desync_detection=true;
 gekko_start(session,&config);gekko_net_adapter_set(session,transport.adapter());gekko_set_disconnect_timeout(session,10000);
 auto address=transport.peer_address();
 for(unsigned p=0;p<2;p++)if(gekko_add_actor(session,p==options.role?GekkoLocalPlayer:GekkoRemotePlayer,p==options.role?nullptr:&address)!=int(p)){
  result.message="GekkoNet could not assign both player slots.";result.done=true;return result;
 }
 gekko_set_local_delay(session,options.role,2);
 ProbeState state,checkpoint;state.rng=options.match.seed;unsigned nextFrame=0;bool failed=false,hasCheckpoint=false;
 publish("Peers matched. Running the rollback connection test...");begin=GetTickCount64();
 auto nextTick=std::chrono::steady_clock::now();
 while(!cancel&&nextFrame<options.frames+16&&GetTickCount64()-begin<35000){
  auto input=nextFrame+2<options.frames?probe_input(options.role,nextFrame):Input{};gekko_add_local_input(session,options.role,&input);
  int n=0;auto events=gekko_update_session(session,&n);
  bool advanced=false;
  for(int i=0;i<n;i++){
   auto e=events[i];
   if(e->type==GekkoSaveEvent){*e->data.save.state_len=sizeof(state);*e->data.save.checksum=state.checksum();if(state.frame==options.frames){checkpoint=state;hasCheckpoint=true;}memcpy(e->data.save.state,&state,sizeof(state));}
   else if(e->type==GekkoLoadEvent){if(e->data.load.state_len!=sizeof(state)){failed=true;break;}memcpy(&state,e->data.load.state,sizeof(state));}
   else if(e->type==GekkoAdvanceEvent){
    if(e->data.adv.input_len!=2*sizeof(Input)||e->data.adv.frame<0||unsigned(e->data.adv.frame)!=state.frame){failed=true;break;}
    Input in[2];memcpy(in,e->data.adv.inputs,sizeof(in));if(!valid_input(in[0])||!valid_input(in[1])){failed=true;break;}
    state.advance(in);if(!e->data.adv.rolling_back&&!e->data.adv.running_ahead){nextFrame=unsigned(e->data.adv.frame)+1;advanced=true;}
   }
  }
  auto notifications=gekko_session_events(session,&n);
  for(int i=0;i<n;i++)if(notifications[i]->type==GekkoDesyncDetected||notifications[i]->type==GekkoPlayerDisconnected)failed=true;
  if(failed||transport.disconnected()||!transport.error().empty()){failed=true;break;}
  GekkoRollbackStats rollback={};gekko_rollback_stats(session,&rollback);result.rollbacks=rollback.total_count;
  GekkoNetworkStats stats={};gekko_network_stats(session,1-options.role,&stats);result.ping=stats.avg_ping;result.frame=nextFrame;
  if(update)update(result);
  if(advanced){nextTick+=std::chrono::microseconds(16667);std::this_thread::sleep_until(nextTick);}
  else {nextTick=std::chrono::steady_clock::now();Sleep(2);}
 }
 // Stop submitting test buttons before the finish so both simulations converge
 // beyond the prediction window. Confirm the exact same final frame and checksum.
 ProbeState expected;expected.rng=options.match.seed;
 for(unsigned f=0;f<options.frames;f++){Input in[2]={f<2?Input{}:probe_input(0,f-2),f<2?Input{}:probe_input(1,f-2)};expected.advance(in);}
 if(!failed&&!cancel&&nextFrame==options.frames+16&&hasCheckpoint){
  // Sixteen neutral frames beyond the checkpoint exceed the eight-frame
  // prediction window. A rollback which touches it replaces the checkpoint.
  uint64_t settle=GetTickCount64();
  while(!cancel&&GetTickCount64()-settle<2000){gekko_network_poll(session);Sleep(2);}
  transport.report(checkpoint.frame,checkpoint.checksum());
  settle=GetTickCount64();
  // Keep replying for the full grace period even after receiving our peer's
  // checkpoint. Otherwise one lost final datagram lets one side report success
  // and exit while the other side is still waiting for that result.
  while(!cancel&&GetTickCount64()-settle<3000){gekko_network_poll(session);transport.report(checkpoint.frame,checkpoint.checksum());Sleep(20);}
  result.success=checkpoint.frame==options.frames&&checkpoint.checksum()==expected.checksum()&&transport.hasResult
   &&transport.remoteFrame==checkpoint.frame&&transport.remoteChecksum==checkpoint.checksum();
 }
 result.done=true;publish(cancel?"Connection test cancelled.":result.success?"Connection test passed. Native game rollback is still in development.":"Connection test failed or timed out; combat was not started.");
 transport.send(Kind::Bye);return result;
}
}
