#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <tlhelp32.h>
#include "launcher_protocol.h"
#include "launcher_helper_hash.h"
struct Choice { unsigned id; std::string label; };
static std::wstring directory;
static std::vector<Choice> mechas,stages;
static HANDLE battle=nullptr;
static uintptr_t remote_config=0;
static ULONGLONG launched=0;
static std::string status="Choose both mechas and a stage.";
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
static uintptr_t find_module(DWORD pid,const wchar_t* name) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);
 if(snapshot==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W item={sizeof(item)};uintptr_t result=0;
 if(Module32FirstW(snapshot,&item))do{
  if(!_wcsicmp(item.szModule,name)){result=(uintptr_t)item.modBaseAddr;break;}
 }while(Module32NextW(snapshot,&item));
 CloseHandle(snapshot);return result;
}
static bool remote_call(HANDLE process,uintptr_t proc,void* arg,DWORD& result) {
 HANDLE thread=CreateRemoteThread(process,nullptr,0,(LPTHREAD_START_ROUTINE)proc,arg,0,nullptr);
 if(!thread)return false;
 bool done=WaitForSingleObject(thread,30000)==WAIT_OBJECT_0;
 bool ok=done&&GetExitCodeThread(thread,&result);CloseHandle(thread);return ok;
}
static bool existing_game() {
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)return true;
 PROCESSENTRY32W entry={sizeof(entry)};bool found=false;
 if(Process32FirstW(snap,&entry))do{if(!_wcsnicmp(entry.szExeFile,L"BaldrSky",8)){found=true;break;}}while(Process32NextW(snap,&entry));
 CloseHandle(snap);return found;
}
static void start_battle(unsigned p1,unsigned p2,unsigned map) {
 if(battle)return;
 VSConfig cfg={0x42565331,1,p1,p2,map,1,0,0};
 if(!valid_config(cfg)){status="Invalid selection.";return;}
 // Game is beside this program, in the user's own installation.
 std::wstring path=directory+L"BaldrSky.VSPrototype.v11c.exe";
 std::wstring dll=directory+L"BaldrVSSetup.dll";
 if(!verified_file(path,"42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460")){status="Place the launcher beside the verified V11c game build.";return;}
 if(!verified_file(dll,VS_HELPER_SHA256)){status="VS helper missing, changed or blocked by Windows.";return;}
 HMODULE local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
 if(!local){status="VS helper is missing or Windows blocked it.";return;}
 uintptr_t init=(uintptr_t)GetProcAddress(local,"InitializeVS");
 uintptr_t settings=(uintptr_t)GetProcAddress(local,"launcher_config");
 if(!init||!settings){FreeLibrary(local);status="Invalid VS helper.";return;}
 init-=(uintptr_t)local;settings-=(uintptr_t)local;FreeLibrary(local);
 STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWNORMAL;
 PROCESS_INFORMATION pi={};std::wstring command=L"\""+path+L"\" -u";
 if(!CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&si,&pi)){
  status="Could not start game (Windows error "+std::to_string(GetLastError())+").";return;
 }
 SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t),written=0;
 void* memory=VirtualAllocEx(pi.hProcess,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 DWORD result=0;bool ok=memory&&WriteProcessMemory(pi.hProcess,memory,dll.c_str(),bytes,&written)&&written==bytes;
 // Same-bitness local and child processes; resolve the system function by module RVA.
 HMODULE kernel=GetModuleHandleW(L"kernel32.dll");
 uintptr_t load=(uintptr_t)GetProcAddress(kernel,"LoadLibraryW");
 HMODULE owner=nullptr;
 GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)load,&owner);
 wchar_t ownerPath[MAX_PATH]={};GetModuleFileNameW(owner,ownerPath,MAX_PATH);
 const wchar_t* base=wcsrchr(ownerPath,L'\\');base=base?base+1:ownerPath;
 uintptr_t remoteOwner=0;
 for(int i=0;i<100&&!remoteOwner;i++){remoteOwner=find_module(pi.dwProcessId,base);if(!remoteOwner)Sleep(20);}
 ok=ok&&remoteOwner&&remote_call(pi.hProcess,remoteOwner+load-(uintptr_t)owner,memory,result)&&result;
 // If a remote thread timed out, its argument must remain allocated until the process exits.
 // On any setup failure close only the game instance this launcher created.
 uintptr_t remote=ok?find_module(pi.dwProcessId,L"BaldrVSSetup.dll"):0;
 // The helper changes two native calls. Pause only the child main thread
 // after DLL loading has completed, then resume it after initialization.
 bool paused=ok&&remote&&SuspendThread(pi.hThread)!=DWORD(-1);
 ok=ok&&paused;
 if(ok&&remote){
  VirtualFreeEx(pi.hProcess,memory,0,MEM_RELEASE);
  memory=VirtualAllocEx(pi.hProcess,nullptr,sizeof(cfg),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
  ok=memory&&WriteProcessMemory(pi.hProcess,memory,&cfg,sizeof(cfg),&written)&&written==sizeof(cfg);
  result=0;ok=ok&&remote_call(pi.hProcess,remote+init,memory,result)&&result==1;
 }else ok=false;
 if(paused)ResumeThread(pi.hThread);
 CloseHandle(pi.hThread);
 if(!ok){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);CloseHandle(pi.hProcess);
  status="VS setup failed. Check Windows protection history; the game file is unchanged.";return;}
 VirtualFreeEx(pi.hProcess,memory,0,MEM_RELEASE);
 remote_config=remote+settings;battle=pi.hProcess;launched=GetTickCount64();status="Starting VS battle...";
}
static void poll_battle() {
 if(!battle)return;DWORD code=0;
 if(GetExitCodeProcess(battle,&code)&&code!=STILL_ACTIVE){
  CloseHandle(battle);battle=nullptr;status="Game closed (exit "+std::to_string(code)+").";return;
 }
 VSConfig config={};SIZE_T read=0;
 if(ReadProcessMemory(battle,(void*)remote_config,&config,sizeof(config),&read)&&read==sizeof(config)){
  if(config.state==3)status="VS battle ready. Use your two controllers.";
  else if(config.state==9)status="VS startup failed at step "+std::to_string(config.step)+".";
  else if(GetTickCount64()-launched>90000)status="Check the game window: VS startup is taking longer than expected.";
 }
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
