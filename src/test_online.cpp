#include "online_probe.h"
#include "online_link_code.h"
#include <cassert>
#include <cstdio>
#include <random>

int main(int argc,char** argv){
 using namespace online;
 if(argc==4&&!strcmp(argv[1],"--make-code")){Token key;if(!token_from_text(argv[3],key))return 2;auto code=make_link_code(argv[2],key);if(code.empty())return 2;puts(code.c_str());return 0;}
 if(argc>=5){
  ProbeOptions options;options.role=std::stoul(argv[1]);options.port=std::stoul(argv[2]);options.peer=argv[3];options.frames=180;
  if(options.peer.rfind("BALDR1",0)==0){if(!parse_link_code(argv[3],options.peer,options.token))return 2;}
  else if(!token_from_text(argv[4],options.token))return 2;
  bool chatTest=false;for(int i=5;i<argc;i++){
   if(!strcmp(argv[i],"--chat-test")){chatTest=true;options.keepChat=true;}
   else if(!strcmp(argv[i],"--name")&&i+1<argc){options.playerName=argv[++i];}
   else{options.punch=true;std::string path=argv[i];options.punchPath.assign(path.begin(),path.end());}
  }
  options.match=connection_test_manifest();
  auto chat=chatTest?std::make_shared<ChatMailbox>(options.role,options.playerName):nullptr;
  std::atomic_bool cancel=false;bool early=false,late=false;uint64_t completeAt=0;
  auto verifyChat=[&](){
   if(!chat)return false;auto snapshot=chat->snapshot();if(snapshot.localName!=options.playerName||snapshot.peerName!=(options.role?"Test Host":"Test Guest"))return false;unsigned delivered=0,received=0;
   for(auto& line:snapshot.lines){
    if(line.local){if(line.delivery==ChatDelivery::Delivered)delivered++;}
    else{
     unsigned remote=1-options.role;
     bool expected=line.text=="Hello from P"+std::to_string(remote+1)
       ||line.text==std::string(512,char('a'+remote))
       ||line.text=="After the test: 100% ### from P"+std::to_string(remote+1);
     if(!expected)return false;received++;
    }
   }return delivered==3&&received==3&&snapshot.lines.size()==6;
  };
  auto update=[&](const ProbeProgress& progress){
   if(!chat)return;
   if(chat->snapshot().connected&&!early){early=true;chat->send_text("Hello from P"+std::to_string(options.role+1));chat->send_text(std::string(512,char('a'+options.role)));}
   if(progress.message=="Connection test passed. Player chat connected."&&!late){late=true;chat->send_text("After the test: 100% ### from P"+std::to_string(options.role+1));}
   if(late&&verifyChat()){if(!completeAt)completeAt=GetTickCount64();else if(GetTickCount64()-completeAt>1000)cancel=true;}
  };
  auto result=run_probe(options,cancel,update,chat);
  if(chatTest){bool good=late&&verifyChat();printf("CHAT %s: three messages each direction, including a 512-byte message and post-test chat.\n",good?"PASS":"FAIL");result.success&=good;}
  printf("%s frames=%u rollbacks=%u ping=%.1f\n",result.message.c_str(),result.frame,result.rollbacks,result.ping);return result.success?0:1;
 }
 Token token;assert(token_from_text("0123456789abcdef0123456789abcdef",token));
 assert(!token_from_text("00000000000000000000000000000000",token));
 assert(!token_from_text("abc",token));assert(!token_from_text("0123456789abcdef0123456789abcdeg",token));
 std::string endpoint;Token parsed={};auto code=make_link_code("192.168.1.24:14764",token);
 assert(!code.empty()&&parse_link_code(code,endpoint,parsed)&&endpoint=="192.168.1.24:14764"&&parsed==token);
 auto typo=code;typo.back()=typo.back()=='0'?'1':'0';assert(!parse_link_code(typo,endpoint,parsed));
 assert(!parse_link_code("BALDR2-"+code.substr(7),endpoint,parsed));
 for(size_t n=0;n<code.size()-1;n++)assert(!parse_link_code(code.substr(0,n),endpoint,parsed));
 assert(make_link_code("999.1.1.1:1",token).empty());
 uint8_t payload[]={1,2,3};auto data=envelope(Kind::Data,0,token,payload,sizeof(payload));Packet p={};
 assert(decode(data.data(),data.size(),token,p)&&p.size==3&&p.role==0);
 for(size_t n=0;n<data.size();n++)assert(!decode(data.data(),n,token,p));
 auto bad=data;bad[24]++;assert(!decode(bad.data(),bad.size(),token,p));
 bad=data;bad[8]++;assert(!decode(bad.data(),bad.size(),token,p));
 bad=data;bad[4]++;assert(!decode(bad.data(),bad.size(),token,p));
 bad=data;bad[6]=2;assert(!decode(bad.data(),bad.size(),token,p));
 assert(envelope(Kind::Data,0,token,payload,max_payload+1).empty());
 Manifest m;assert(!m.valid());for(auto& slots:m.equipment)slots.fill(equipment_empty);assert(m.valid());
 auto original=m.bytes();m.equipment[1][3]=22;assert(original!=m.bytes());m.equipment[1][4]=22;assert(!m.valid());
 sockaddr_in addr={};assert(parse_address("127.0.0.1:14764",addr));
 for(auto s:{"127.0.0.1:0","127.0.0.1:65536","0.0.0.0:1","255.255.255.255:1","224.0.0.1:1","localhost:1","127.0.0.1:-2","127.0.0.1:1x"})assert(!parse_address(s,addr));
 std::mt19937 random(1);for(unsigned i=0;i<20000;i++){std::vector<uint8_t> bytes(random()%1300);for(auto& b:bytes)b=uint8_t(random());decode(bytes.data(),bytes.size(),token,p);}
 assert(valid_input({0xfff})&&!valid_input({0x1000}));assert(!native_rollback_verified);
 // Exercise the actual SDK's repeated save/load/resimulation requests.
 GekkoSession* session=nullptr;assert(gekko_create(&session,GekkoStressSession));
 GekkoConfig config={};config.num_players=2;config.input_size=sizeof(Input);config.state_size=sizeof(ProbeState);config.check_distance=8;
 gekko_start(session,&config);assert(gekko_add_actor(session,GekkoLocalPlayer,nullptr)==0);assert(gekko_add_actor(session,GekkoLocalPlayer,nullptr)==1);
 ProbeState state,expected;unsigned loads=0;
 for(unsigned f=0;f<600;f++){
  Input in[2]={probe_input(0,f),probe_input(1,f)};for(unsigned p=0;p<2;p++)gekko_add_local_input(session,p,&in[p]);
  int count=0;auto events=gekko_update_session(session,&count);
  for(int i=0;i<count;i++){
   auto e=events[i];if(e->type==GekkoSaveEvent){*e->data.save.state_len=sizeof(state);*e->data.save.checksum=state.checksum();memcpy(e->data.save.state,&state,sizeof(state));}
   else if(e->type==GekkoLoadEvent){assert(e->data.load.state_len==sizeof(state));memcpy(&state,e->data.load.state,sizeof(state));loads++;}
   else if(e->type==GekkoAdvanceEvent){Input inputs[2];assert(e->data.adv.input_len==sizeof(inputs));memcpy(inputs,e->data.adv.inputs,sizeof(inputs));assert(state.frame==unsigned(e->data.adv.frame));state.advance(inputs);}
  }
  auto notices=gekko_session_events(session,&count);for(int i=0;i<count;i++)assert(notices[i]->type!=GekkoDesyncDetected);
  expected.advance(in);assert(state.checksum()==expected.checksum());
 }
 assert(loads>500);gekko_destroy(&session);
 printf("PASS: packet bounds/fuzz, room/version rejection, match fingerprint, endpoints, native-combat gate; 600 Gekko stress frames, %u loads.\n",loads);
}
