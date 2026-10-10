#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <tlhelp32.h>
#include "launcher_protocol.h"
#include "launcher_native_hash.h"
#include "equipment_profiles.h"
struct Choice { unsigned id; std::string label; };
static std::wstring directory;
static std::vector<Choice> mechas,stages,music;
static bool native_set_ended=false;
static HANDLE battle=nullptr;
static ULONGLONG launched=0;
static std::string status="Choose both mechas and a stage.";
static EquipmentCatalog equipment_catalog;
static EquipmentProfiles equipment_profiles;
static bool equipment_scan=false;
static void load_equipment(){
 std::ifstream catalog(directory+L"equipment.tsv");EquipmentCatalog loaded;
 if(read_equipment_catalog(catalog,loaded))equipment_catalog=std::move(loaded);
 if(equipment_profiles.active.empty()&&equipment_profiles.presets.empty()){
  std::ifstream profiles(directory+L"BaldrVS-loadouts.tsv");equipment_profiles.read(profiles,equipment_catalog);
 }
}
static bool save_equipment_profiles(){
 std::ofstream file(directory+L"BaldrVS-loadouts.tsv.tmp",std::ios::binary);equipment_profiles.write(file);file.close();
 if(!file)return false;
 return MoveFileExW((directory+L"BaldrVS-loadouts.tsv.tmp").c_str(),(directory+L"BaldrVS-loadouts.tsv").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
}
static void load_choices(const wchar_t *file,std::vector<Choice>& choices) {
 std::ifstream in(directory+file);std::string line;
 while(std::getline(in,line)) {
  auto tab=line.find('\t');if(tab==std::string::npos)continue;
  try { unsigned id=std::stoul(line.substr(0,tab));std::string name=line.substr(tab+1);
   if(!name.empty()&&name.back()=='\r')name.pop_back();
   if(name.empty())continue;
   choices.push_back({id,name+"  ["+std::to_string(id)+"]"});
  }catch(...){choices.clear();return;}
 }
}
static bool verified_file(const std::wstring& path,const char* expected) {
 HCRYPTPROV provider=0;HCRYPTHASH hash=0;bool good=false;
 if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return false;
 if(CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) {
  std::ifstream in(path,std::ios::binary);char buffer[65536];bool readOK=in.good();
  while(in){in.read(buffer,sizeof(buffer));auto n=in.gcount();if(n&&!CryptHashData(hash,(BYTE*)buffer,(DWORD)n,0)){readOK=false;break;}}
  BYTE digest[32];DWORD length=32;
  if(readOK&&!in.bad()&&CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0)){
   const char *digits="0123456789ABCDEF";std::string hex;
   for(BYTE b:digest){hex+=digits[b>>4];hex+=digits[b&15];}
   good=hex==expected;
  }
  CryptDestroyHash(hash);
 }
 CryptReleaseContext(provider,0);return good;
}
static bool existing_game() {
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)return true;
 PROCESSENTRY32W entry={sizeof(entry)};bool found=false;
 if(Process32FirstW(snap,&entry))do{if(!_wcsnicmp(entry.szExeFile,L"BaldrSky",8)){found=true;break;}}while(Process32NextW(snap,&entry));
 CloseHandle(snap);return found;
}

static void start_battle(unsigned p1,unsigned p2,unsigned map,unsigned setWins=2,unsigned track=0xffffffffu,const VSEquipmentConfig* equipment=nullptr) {
 if(battle)return;
 VSNativeConfig cfg={{0x42565331,1,p1,p2,map,1,0,setWins},track};
 if(!valid_config(cfg.battle)||!valid_music(track)){status="Invalid selection.";return;}
 native_set_ended=false;
 if(existing_game()){status="Close the current game before starting another battle.";return;}
 std::wstring path=directory+L"BaldrSky.VSNative.exe";
 if(!verified_file(path,VS_NATIVE_GAME_SHA256)||!verified_file(directory+L"BaldrVSPlugin.dll",VS_NATIVE_PLUGIN_SHA256)){
  status="Native VS game/plugin pair missing, changed or blocked by Windows.";return;
 }
 HANDLE file=CreateFileW((directory+L"BaldrVS-start.bin").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 DWORD written=0;bool ok=file!=INVALID_HANDLE_VALUE;
 if(ok){ok=WriteFile(file,&cfg,sizeof(cfg),&written,nullptr)&&written==sizeof(cfg);CloseHandle(file);}
 if(!ok){status="Could not save battle selection.";return;}
 VSEquipmentConfig loadout;if(equipment)loadout=*equipment;else{loadout.p1=p1;loadout.p2=p2;}
 equipment_scan=loadout.catalogOnly!=0;
 if(!equipment_shape_valid(loadout,p1,p2)){status="Invalid equipment selection.";return;}
 file=CreateFileW((directory+L"BaldrVS-equipment.bin").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 ok=file!=INVALID_HANDLE_VALUE;written=0;
 if(ok){ok=WriteFile(file,&loadout,sizeof(loadout),&written,nullptr)&&written==sizeof(loadout);CloseHandle(file);}
 if(!ok){status="Could not save equipment selection.";return;}
 DeleteFileW((directory+L"BaldrVS-status.bin").c_str());
 STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWNORMAL;
 PROCESS_INFORMATION pi={};std::wstring command=L"\""+path+L"\" -u";
 if(!CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&si,&pi)){
  status="Could not start game (Windows error "+std::to_string(GetLastError())+").";return;
 }
 CloseHandle(pi.hThread);battle=pi.hProcess;launched=GetTickCount64();status=equipment_scan?"Reading the game's equipment catalog...":"Starting VS battle...";
}
static void poll_battle(){
 if(!battle)return;DWORD code=0;
 if(GetExitCodeProcess(battle,&code)&&code!=STILL_ACTIVE){
  VSConfig finalStatus={};std::ifstream finalFile(directory+L"BaldrVS-status.bin",std::ios::binary);
  if(finalFile.read((char*)&finalStatus,sizeof(finalStatus))&&finalStatus.magic==0x42565331&&finalStatus.state==4)native_set_ended=true;
  CloseHandle(battle);battle=nullptr;
  load_equipment();
  status=equipment_scan?(equipment_catalog.empty()?"Equipment scan failed. Check the game files.":"Equipment catalog ready. Choose Equipment for either player."):
    native_set_ended?"Set finished. Choose your next battle.":"Game closed (exit "+std::to_string(code)+").";
  equipment_scan=false;return;
 }
 VSConfig cfg={};std::ifstream input(directory+L"BaldrVS-status.bin",std::ios::binary);
 if(input.read((char*)&cfg,sizeof(cfg))&&cfg.magic==0x42565331&&cfg.version==1){
  if(cfg.state==3)status="VS battle ready. Use your two controllers.";
  else if(cfg.state==4){native_set_ended=true;status="Set finished. Closing game...";}
  else if(cfg.state==9)status="VS plugin startup failed at step "+std::to_string(cfg.step)+".";
 }
 if(GetTickCount64()-launched>90000&&status=="Starting VS battle...")status="Check the game window: VS startup is taking longer than expected.";
}
static void close_game_from_launcher(){
 if(!battle){status="No game is running from this launcher.";return;}
 // This handle belongs to the child started by the launcher. Terminating that
 // child directly avoids the game's normal close-confirmation dialog.
 TerminateProcess(battle,0);
 WaitForSingleObject(battle,3000);
 CloseHandle(battle);battle=nullptr;native_set_ended=false;
 status="Game closed.";
}
static void choice_combo(const char* id,int& selected,const std::vector<Choice>& choices) {
 if(choices.empty()){ImGui::BeginDisabled();ImGui::Button("Roster unavailable");ImGui::EndDisabled();return;}
 if(selected<0||selected>=(int)choices.size())selected=0;
 if(ImGui::BeginCombo(id,choices[selected].label.c_str())){
  for(int i=0;i<(int)choices.size();i++){
   bool current=i==selected;if(ImGui::Selectable(choices[i].label.c_str(),current))selected=i;
   if(current)ImGui::SetItemDefaultFocus();
  }ImGui::EndCombo();
 }
}


