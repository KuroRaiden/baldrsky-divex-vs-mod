#pragma once
#include "online_probe.h"
#include "online_link_code.h"
#include <mutex>
#include <algorithm>

static std::future<online::ProbeProgress> online_task;
static std::atomic_bool online_cancel=false;
static std::mutex online_mutex;
static online::ProbeProgress online_progress;
static bool online_mode=true;
static std::shared_ptr<online::ChatMailbox> online_chat=std::make_shared<online::ChatMailbox>();
static online::Digest online_hash(const std::wstring& path){
 online::Digest digest={};HCRYPTPROV p=0;HCRYPTHASH h=0;
 if(!CryptAcquireContextW(&p,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return digest;
 if(CryptCreateHash(p,CALG_SHA_256,0,0,&h)){
  std::ifstream f(path,std::ios::binary);char b[8192];bool good=f.good();
  while(f){f.read(b,sizeof(b));if(f.gcount()&&!CryptHashData(h,(BYTE*)b,DWORD(f.gcount()),0)){good=false;break;}}
  DWORD n=32;if(!good||f.bad()||!CryptGetHashParam(h,HP_HASHVAL,digest.data(),&n,0))digest={};CryptDestroyHash(h);
 }CryptReleaseContext(p,0);return digest;
}
static online::Digest online_catalog_hash(){
 // Training defaults and translated labels are machine-local. Compatibility
 // depends on mecha/move IDs and slot types; explicit loadouts are matched too.
 std::vector<uint8_t> bytes;
 for(auto& entry:equipment_catalog){
  online::put32(bytes,entry.first);std::vector<std::pair<unsigned,unsigned>> moves;
  for(auto& move:entry.second.moves)moves.push_back({move.id,move.type});std::sort(moves.begin(),moves.end());
  online::put32(bytes,unsigned(moves.size()));for(auto move:moves){online::put32(bytes,move.first);online::put32(bytes,move.second);}
 }
 online::Digest result={};HCRYPTPROV provider=0;HCRYPTHASH hash=0;
 if(bytes.empty()||!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return result;
 if(CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){
  DWORD size=32;if(!CryptHashData(hash,bytes.data(),DWORD(bytes.size()),0)||!CryptGetHashParam(hash,HP_HASHVAL,result.data(),&size,0))result={};CryptDestroyHash(hash);
 }CryptReleaseContext(provider,0);return result;
}
static std::future<std::string> online_code_task;
static char online_link[97]={};
static void online_panel(float scale,unsigned p1,unsigned p2,unsigned map,unsigned wins,unsigned song){
 ImGui::SetCursorPos(ImVec2(42*scale,435*scale));
 ImGui::SeparatorText("ONLINE VS - DEVELOPMENT");
 ImGui::BeginDisabled(online_task.valid()||online_code_task.valid());ImGui::Checkbox("Online setup",&online_mode);ImGui::EndDisabled();
 if(!online_mode)return;
 static int port=14764;static bool punch=true;static char advertise[64]="";
 bool busy=online_task.valid()||online_code_task.valid();
 if(online_task.valid()&&online_task.wait_for(std::chrono::seconds(0))==std::future_status::ready){auto r=online_task.get();std::lock_guard<std::mutex> lock(online_mutex);online_progress=r;busy=online_code_task.valid();}
 auto begin=[&](unsigned role){
  online::ProbeOptions options;options.role=role;options.port=unsigned(port);options.punch=punch;
  options.keepChat=true;
  options.punchPath=directory+L"BaldrAutoPunch.dll";
  if(!online::parse_link_code(online_link,options.peer,options.token)){std::lock_guard<std::mutex> lock(online_mutex);online_progress.message="Paste a valid BALDR1 link code.";return;}
  if(role==0){sockaddr_in address={};online::parse_address(options.peer,address);options.port=ntohs(address.sin_port);}
  auto& m=options.match;m.p1=p1;m.p2=p2;m.stage=map;m.target=wins;m.music=song;
  m.equipment[0]=equipment_profiles.get(0,p1,equipment_catalog);m.equipment[1]=equipment_profiles.get(1,p2,equipment_catalog);
  m.build=online_hash(directory+L"BaldrVSPlugin.dll");m.catalog=online_catalog_hash();
  if(!m.valid()||m.build==online::Digest{}||m.catalog==online::Digest{}){
   std::lock_guard<std::mutex> lock(online_mutex);online_progress.message="Load the equipment catalog and resolve both loadouts before connecting.";return;
  }
  online_chat=std::make_shared<online::ChatMailbox>(role);auto chat=online_chat;
  online_cancel=false;online_task=std::async(std::launch::async,[options,chat]{return online::run_probe(options,online_cancel,[](const online::ProbeProgress& p){std::lock_guard<std::mutex> lock(online_mutex);online_progress=p;},chat);});
 };
 if(online_code_task.valid()&&online_code_task.wait_for(std::chrono::seconds(0))==std::future_status::ready){
  auto code=online_code_task.get();busy=online_task.valid();
  if(code.empty()){std::lock_guard<std::mutex> lock(online_mutex);online_progress.message="Could not discover an Internet IPv4 address. Use an address override under Advanced.";}
  else {strcpy_s(online_link,code.c_str());if(!online_cancel)begin(0);}
 }
 ImGui::BeginDisabled(busy);
 ImGui::TextUnformatted("LINK CODE");ImGui::SetNextItemWidth(560*scale);ImGui::InputText("##link",online_link,sizeof(online_link));
 if(ImGui::Button("Create link code / Host")){
  if(port>0&&port<=65535){
   online::Token token={};HCRYPTPROV provider=0;
   if(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)){
    bool random=CryptGenRandom(provider,DWORD(token.size()),token.data())!=FALSE;CryptReleaseContext(provider,0);
    if(random){bool discover=punch&&!*advertise;std::string ip=advertise;unsigned localPort=unsigned(port);online_cancel=false;
     online_code_task=std::async(std::launch::async,[discover,ip,token,localPort]{auto host=discover?online::public_ipv4():ip;return online::make_link_code(host+":"+std::to_string(localPort),token);});
    }
   }
  }
 }
 ImGui::SameLine();if(ImGui::Button("Join with link code"))begin(1);
 ImGui::EndDisabled();
 ImGui::SameLine();if(ImGui::Button("Copy code"))ImGui::SetClipboardText(online_link);
 ImGui::SameLine();if(ImGui::Button("Disconnect"))online_cancel=true;
 ImGui::BeginDisabled(busy);ImGui::BeginDisabled(online::punch_active);if(online::punch_active)punch=true;
 ImGui::Checkbox(online::punch_active?"AutoPunch active":"Use AutoPunch (Internet)",&punch);ImGui::EndDisabled();
 if(ImGui::CollapsingHeader("Advanced")){
  ImGui::SetNextItemWidth(180*scale);ImGui::InputInt("Local UDP port",&port);
  ImGui::SetNextItemWidth(210*scale);ImGui::InputText("Advertise IPv4 (blank = Internet auto)",advertise,sizeof(advertise));
  ImGui::TextWrapped("For two launchers on one PC, use a different local port when joining. Use your LAN address for LAN-only codes.");
 }
 ImGui::EndDisabled();
 online::ProbeProgress progress;{std::lock_guard<std::mutex> lock(online_mutex);progress=online_progress;}
 ImGui::PushTextWrapPos(620*scale);ImGui::TextUnformatted(online_code_task.valid()?"Creating your link code...":progress.message.c_str());
 if(online_task.valid())ImGui::Text("Frame %u | %.0f ms | %u rollbacks",progress.frame,progress.ping,progress.rollbacks);
 ImGui::TextWrapped("Connection test only: native online combat is still in development. Both peers must select the same settings and loadouts. Share the link code with the other player.");ImGui::PopTextWrapPos();
}
static void online_chat_panel(float scale,float left,float width){
 ImGui::SetNextWindowPos(ImVec2(left,0));ImGui::SetNextWindowSize(ImVec2(width,ImGui::GetIO().DisplaySize.y));
 ImGui::Begin("Player chat",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
 auto snapshot=online_chat->snapshot();static char text[online::chat_max_bytes+1]={};
 static std::weak_ptr<online::ChatMailbox> room;static uint64_t seen=0;static std::string notice;static bool focus=false;
 if(room.lock()!=online_chat){room=online_chat;seen=0;text[0]=0;notice.clear();focus=false;}
 ImGui::TextUnformatted(snapshot.connected?"Connected":"Not connected");
 bool changed=snapshot.revision!=seen;seen=snapshot.revision;
 ImGui::BeginChild("##chat-history",ImVec2(0,ImGui::GetContentRegionAvail().y-100*scale),ImGuiChildFlags_Borders);
 bool nearBottom=ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-10*scale;
 if(snapshot.lines.empty())ImGui::TextWrapped("Connect with a link code to chat with the other player.");
 for(auto& line:snapshot.lines){
  ImGui::PushID(int(line.id));ImGui::PushID(line.local?1:0);
  const char* delivery=line.delivery==online::ChatDelivery::Pending?" - sending...":line.delivery==online::ChatDelivery::Failed?" - not delivered":"";
  ImGui::TextColored(line.local?ImVec4(.15f,.35f,.7f,1):ImVec4(.65f,.25f,.2f,1),"%s (P%u)%s",line.local?"You":"Player",line.player+1,delivery);
  ImGui::PushTextWrapPos(0);ImGui::TextUnformatted(line.text.c_str());ImGui::PopTextWrapPos();ImGui::Spacing();
  ImGui::PopID();ImGui::PopID();
 }
 if(changed&&nearBottom)ImGui::SetScrollHereY(1);ImGui::EndChild();
 ImGui::BeginDisabled(!snapshot.connected);
 ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x-65*scale);
 if(focus){ImGui::SetKeyboardFocusHere();focus=false;}
 bool send=ImGui::InputTextWithHint("##chat-input","Type a message...",text,sizeof(text),ImGuiInputTextFlags_EnterReturnsTrue);
 ImGui::SameLine();send=ImGui::Button("Send",ImVec2(58*scale,0))||send;
 if(send&&text[0]){
  if(online_chat->send_text(text)){text[0]=0;notice.clear();focus=true;}
  else notice="Message not queued. Check its length or wait for pending messages.";
 }
 ImGui::EndDisabled();ImGui::TextDisabled("Enter to send | %u/%u bytes",unsigned(strlen(text)),unsigned(online::chat_max_bytes));
 if(!notice.empty())ImGui::TextWrapped("%s",notice.c_str());
 ImGui::End();
}
