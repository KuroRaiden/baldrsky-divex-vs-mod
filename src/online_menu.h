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
static char online_name[online::name_max_bytes+1]="Player";
static bool online_name_loaded=false;
static std::string online_name_notice;
static void load_online_name(){
 if(online_name_loaded)return;online_name_loaded=true;
 std::ifstream input(directory+L"BaldrVS-player-name.txt",std::ios::binary);std::string name;
 if(online::read_player_name(input,name))strcpy_s(online_name,name.c_str());
}
static bool save_online_name(){
 std::string name=online::trim_name(online_name);
 if(!online::valid_player_name(name)){online_name_notice="Enter a name of 1 to 24 characters.";return false;}
 strcpy_s(online_name,name.c_str());
 std::ofstream file(directory+L"BaldrVS-player-name.txt.tmp",std::ios::binary);
 bool good=online::write_player_name(file,name);file.close();
 if(!good||!file||!MoveFileExW((directory+L"BaldrVS-player-name.txt.tmp").c_str(),(directory+L"BaldrVS-player-name.txt").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
  online_name_notice="Your name can be used now, but could not be saved here.";return false;
 }
 online_name_notice.clear();return true;
}
static std::future<std::string> online_code_task;
static char online_link[97]={};
static void online_panel(float scale){
 load_online_name();
 ImGui::SetCursorPos(ImVec2(42*scale,(std::max)(435*scale,ImGui::GetCursorPosY()+8*scale)));
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
  options.playerName=online::trim_name(online_name);
  if(!online::valid_player_name(options.playerName)){
   std::lock_guard<std::mutex> lock(online_mutex);online_progress.message="Enter your player name before connecting.";return;
  }
  save_online_name();
  online_chat=std::make_shared<online::ChatMailbox>(role,options.playerName);auto chat=online_chat;
  online_cancel=false;online_task=std::async(std::launch::async,[options,chat]{return online::run_probe(options,online_cancel,[](const online::ProbeProgress& p){std::lock_guard<std::mutex> lock(online_mutex);online_progress=p;},chat);});
 };
 if(online_code_task.valid()&&online_code_task.wait_for(std::chrono::seconds(0))==std::future_status::ready){
  auto code=online_code_task.get();busy=online_task.valid();
  if(code.empty()){std::lock_guard<std::mutex> lock(online_mutex);online_progress.message="Could not discover an Internet IPv4 address. Use an address override under Advanced.";}
  else {strcpy_s(online_link,code.c_str());if(!online_cancel)begin(0);}
 }
 busy=online_task.valid()||online_code_task.valid();
 ImGui::BeginDisabled(busy);
 ImGui::TextUnformatted("PLAYER NAME");ImGui::SetNextItemWidth(280*scale);
 ImGui::InputText("##player-name",online_name,sizeof(online_name));
 if(ImGui::IsItemDeactivatedAfterEdit())save_online_name();
 if(!online_name_notice.empty())ImGui::TextWrapped("%s",online_name_notice.c_str());
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
 ImGui::TextWrapped("Connection test only: native online combat is still in development. You can connect and chat without installing game files. Share the link code with the other player.");ImGui::PopTextWrapPos();
}
static void online_chat_panel(float scale,float left,float width){
 ImGui::SetNextWindowPos(ImVec2(left,0));ImGui::SetNextWindowSize(ImVec2(width,ImGui::GetIO().DisplaySize.y));
 ImGui::Begin("Player chat",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
 auto snapshot=online_chat->snapshot();static char text[online::chat_max_bytes+1]={};
 static std::weak_ptr<online::ChatMailbox> room;static uint64_t seen=0;static std::string notice;static bool focus=false;
 if(room.lock()!=online_chat){room=online_chat;seen=0;text[0]=0;notice.clear();focus=false;}
 if(snapshot.connected)ImGui::TextWrapped("Connected to %s (P%u)",snapshot.peerName.c_str(),2-snapshot.player);
 else ImGui::TextUnformatted("Not connected");
 bool ready=snapshot.ready.localReady;
 ImGui::BeginDisabled(!snapshot.connected||snapshot.ready.pending||snapshot.ready.starting||snapshot.ready.started);
 if(ImGui::Checkbox("Ready (you)",&ready))online_chat->set_ready(ready);ImGui::EndDisabled();
 bool peerReady=snapshot.ready.peerReady;ImGui::BeginDisabled();ImGui::Checkbox("Peer ready",&peerReady);ImGui::EndDisabled();
 if(snapshot.ready.started)ImGui::TextWrapped("Netplay test started.");
 else if(snapshot.ready.starting)ImGui::TextWrapped("Both ready. Confirming start...");
 else if(snapshot.ready.pending)ImGui::TextWrapped("Sending ready status...");
 else ImGui::TextWrapped("Both players must check Ready to start the netplay test.");
 bool changed=snapshot.revision!=seen;seen=snapshot.revision;
 ImGui::BeginChild("##chat-history",ImVec2(0,ImGui::GetContentRegionAvail().y-100*scale),ImGuiChildFlags_Borders);
 bool nearBottom=ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-10*scale;
 if(snapshot.lines.empty())ImGui::TextWrapped("Connect with a link code to chat with the other player.");
 for(auto& line:snapshot.lines){
  if(line.system){ImGui::PushTextWrapPos(0);ImGui::TextColored(ImVec4(.3f,.35f,.4f,1),"%s",line.text.c_str());ImGui::PopTextWrapPos();ImGui::Spacing();continue;}
  ImGui::PushID(int(line.id));ImGui::PushID(line.local?1:0);
  const char* delivery=line.delivery==online::ChatDelivery::Pending?" - sending...":line.delivery==online::ChatDelivery::Failed?" - not delivered":"";
  const std::string& name=line.local?snapshot.localName:snapshot.peerName;
  ImGui::PushTextWrapPos(0);ImGui::TextColored(line.local?ImVec4(.15f,.35f,.7f,1):ImVec4(.65f,.25f,.2f,1),"%s%s (P%u)%s",name.c_str(),line.local?" (you)":"",line.player+1,delivery);ImGui::PopTextWrapPos();
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
