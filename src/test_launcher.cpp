#include <windows.h>
#include "launcher_protocol.h"
#include <cstdio>
#include <initializer_list>
int main() {
 VSConfig cfg={0x42565331,1,3,1,0,1,0,0};
 if(!valid_config(cfg))return 1;
 for(unsigned stage=0;stage<230;stage++) {
  cfg.stage=stage;
  if(valid_config(cfg)!=(stage<=220&&stage!=9))return 2;
 }
 unsigned count=0;for(unsigned i=0;i<103;i++)count+=valid_mecha(i);
 if(count!=99)return 11;
 for(unsigned id:{10u,20u,23u,49u})if(valid_mecha(id))return 12;
 cfg.stage=0;cfg.p1=103;if(valid_config(cfg))return 3;
 cfg.p1=0;cfg.p2=103;if(valid_config(cfg))return 4;
 cfg.p2=1;cfg.magic=0;if(valid_config(cfg))return 5;
 cfg.magic=0x42565331;cfg.version=2;if(valid_config(cfg))return 6;
 cfg.version=1;cfg.state=2;if(valid_config(cfg))return 7;
 cfg.state=1;
#ifdef VS_SPLIT_SCREEN
#ifdef VS_SPLIT_SCREEN_V2
 HMODULE dll=LoadLibraryExW(L"BaldrVSSetup.SplitScreen.v2.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
#else
 HMODULE dll=LoadLibraryExW(L"BaldrVSSetup.SplitScreen.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
#else
 HMODULE dll=LoadLibraryExW(L"BaldrVSSetup.dll",nullptr,LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
 if(!dll)return 8;
 auto init=(DWORD(WINAPI*)(void*))GetProcAddress(dll,"InitializeVS");
 auto exported=(VSConfig*)GetProcAddress(dll,"launcher_config");
 if(!init||!exported)return 9;
 if(init(nullptr)!=0 || init(&cfg)!=0)return 10;
 FreeLibrary(dll);
 puts("PASS: protocol bounds, exports, rejection of unrelated host; no game started.");
 return 0;
}
