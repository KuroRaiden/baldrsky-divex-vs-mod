#include <windows.h>
#include <cstdio>
#include "launcher_protocol.h"
typedef unsigned int u32;
#define U(a) (*(volatile u32 *)(a))
#ifdef VS_SPLIT_SCREEN
#include "vs_split_screen.h"
#endif
typedef int (__stdcall *Input)(u32);
typedef int (__thiscall *Mode)(void *,int,int);
typedef int (__thiscall *Close)(void *,int);
typedef int (__thiscall *ResetDisplay)(void *);
typedef int (__thiscall *ClearScene)(void *,int,int,int,int,int);
// The reveal routine forwards ECX to its scene manager cleanup, in addition
// to consuming seven stack arguments. IDA's inferred stdcall omits ECX.
typedef int (__thiscall *Reveal)(void*,int,int,int,int,int,int,int);
typedef int (__cdecl *Battle)(int,int);
typedef int (__cdecl *Mecha)(int);
typedef int (__cdecl *Prepare)(void);
typedef int (__thiscall *LoadMecha)(void *,u32,int,int,int);
/* Written only by the launcher into its newly created child process. */
extern "C" __declspec(dllexport) volatile u32 launcher_config[8]={0x42565331,1,0,1,0,0,0,0};
static LONG CALLBACK startup_exception(EXCEPTION_POINTERS* info) {
 if(launcher_config[5]!=2 || info->ExceptionRecord->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION)return EXCEPTION_CONTINUE_SEARCH;
 char report[1024]; auto c=info->ContextRecord;
 int length=sprintf_s(report,sizeof(report),"step=%u code=%08X EIP=%08X ESP=%08X ECX=%08X EAX=%08X\r\n",launcher_config[6],info->ExceptionRecord->ExceptionCode,c->Eip,c->Esp,c->Ecx,c->Eax);
 for(unsigned i=0;i<16;i++)length+=sprintf_s(report+length,sizeof(report)-length,"%08X ",((u32*)c->Esp)[i]);
 HANDLE log=CreateFileW(L"vs-startup-crash.txt",GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(log!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(log,report,length,&written,nullptr);CloseHandle(log);}
 return EXCEPTION_CONTINUE_SEARCH;
}
static int __stdcall launcher_input(u32 manager) {
 // Complete the native Survival scene reset and reveal before handing control to VS.
 if(launcher_config[5]==1 && U(0x86AAC4) && U(0x86AAA8) && U(0x877640)
    && U(0x873A40) && U(0x873AD0) && U(0x873B60)) {
  u32 p1=launcher_config[2],p2=launcher_config[3],map=launcher_config[4];
  if(!valid_mecha(p1) || !valid_mecha(p2) || map>220 || map==9){launcher_config[5]=9;return 0;}
  launcher_config[5]=2;
  launcher_config[6]=10;
  ((Close)0x792170)((void *)manager,0);
  launcher_config[6]=11;
  ((ClearScene)0x74C770)((void*)manager,1,0,1,0,0);
  launcher_config[6]=12;
  ((ResetDisplay)0x743F10)((void*)manager);
  launcher_config[6]=1;
  ((Mode)0x673E60)((void *)U(0x86AAA8),1,1);
  launcher_config[6]=2;
  /* Fresh startup has no previous mech to unload. Use the initial load
     path, rather than the native roster-change routine's release path. */
  U(0x86AA14)=p1;U(0x872B88)=p1;
  ((Prepare)0x5DD3B0)();
  ((LoadMecha)0x671FC0)((void *)U(0x86AAA8),p1,127,-3,-2);
  U(0x8734E8)=p2;U(0x8734EC)=map;
  launcher_config[6]=3;
  ((Battle)0x5E1E10)(2,-1);
  launcher_config[6]=13;
  ((Reveal)0x791BD0)((void*)manager,0,0,100,100,400,300,0);
  *(volatile BYTE*)0x874B98=0;
  launcher_config[6]=4;
  launcher_config[5]=(U(0x86AC48)&&U(0x86AC54)&&U(0x86AA18)==2)?3:9;
  return 0;
 }
 return ((Input)0x79A550)(manager);
}
static void __stdcall skip_startup_movie(int,int,int) {}

// The launcher pauses its child main thread while installing these calls.
// Install only in the known 32-bit V11c image. No file or save writes.
extern "C" __declspec(dllexport) DWORD WINAPI InitializeVS(void* settings) {
 if(!settings || !valid_config(*(VSConfig*)settings))return 0;
 BYTE* image=(BYTE*)GetModuleHandleW(nullptr);
 if(image!=(BYTE*)0x400000)return 0;
 auto dos=(IMAGE_DOS_HEADER*)image;
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096)return 0;
 auto pe=(IMAGE_NT_HEADERS32*)(image+dos->e_lfanew);
 if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386
    || pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC
    || pe->OptionalHeader.SizeOfImage!=0x8C3000)return 0;
 BYTE expected[5]={0xE8,0x20,0x05,0xFF,0xFF};
 BYTE* call=(BYTE*)0x7AA02B;
 for(unsigned i=0;i<5;i++)if(call[i]!=expected[i])return 0;
 BYTE movieExpected[5]={0xE8,0x0D,0x47,0xFA,0xFF};
 BYTE* movieCall=(BYTE*)0x78251E;
 for(unsigned i=0;i<5;i++)if(movieCall[i]!=movieExpected[i])return 0;
 // A video already owns the window: reject this late injection safely.
 if(U(0xAE3E14))return 0;
#ifdef VS_SPLIT_SCREEN
 if(!split_hook_matches(0x787937,0x740150)||!split_hook_matches(0x740E8F,0x73F6A0))return 0;
#endif
 for(unsigned i=0;i<8;i++)launcher_config[i]=((u32*)settings)[i];
 AddVectoredExceptionHandler(1,startup_exception);
 DWORD old=0;
 if(!VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&old))return 0;
 // Displacement address is 4-byte aligned. Both old and new calls are valid.
 InterlockedExchange((volatile LONG*)(call+1),(LONG)((BYTE*)&launcher_input-(call+5)));
 FlushInstructionCache(GetCurrentProcess(),call,5);
 DWORD ignored=0;VirtualProtect(call,5,old,&ignored);
 if(!VirtualProtect(movieCall,5,PAGE_EXECUTE_READWRITE,&old))return 0;
 *(LONG*)(movieCall+1)=(LONG)((BYTE*)&skip_startup_movie-(movieCall+5));
 FlushInstructionCache(GetCurrentProcess(),movieCall,5);
 VirtualProtect(movieCall,5,old,&ignored);
#ifdef VS_SPLIT_SCREEN
 if(!split_install())return 0;
#endif
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
 if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);
 return TRUE;
}
