/* Run beside the freshly compiled rounds.dll. Own-code mock, no game needed. */
typedef unsigned int u32;
#define G(a) ((a)+0x20000000u)
#define U(a) (*(volatile u32 *)G(a))
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,u32,u32,u32);
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(void *,const char *);
__declspec(dllimport) void __stdcall ExitProcess(u32);
static int resets,regen;
static int __fastcall reset(void *self,void *unused){++resets;U(0x8738EC)=0;return 123;}
static int __fastcall update(void *self,void *unused){++U(0x8738EC);return -1;}
static void __stdcall recover(u32 fighter){++regen;}
static int __cdecl format(char *buffer,const char *fmt,...){buffer[0]=0;return 0;}
static int __stdcall title(u32 hwnd,const char *text){return 1;}
static void jump(u32 addr,void *fn){*(unsigned char *)G(addr)=0xE9;U(addr+1)=(u32)fn-G(addr)-5;}
typedef int (__fastcall *Hook)(void *,void *);
typedef u32 (__cdecl *Snap)(void);
void entry(void) {
 int fails=0;
 /* Use a formatter shim outside Windows' low-address allocations. */
 static u32 pages[]={0x5F0000,0x670000,0x7D0000,0x860000,0x870000,0x900000};
 for(int i=0;i<6;i++) if(!VirtualAlloc((void *)G(pages[i]),0x10000,0x3000,0x40)) ExitProcess(110+i);
 jump(0x5FB8C0,reset);jump(0x5F8CC0,update);jump(0x5F8D50,recover);jump(0x676305,format);
 U(0x7D9328)=(u32)title;
 U(0x86AA1C)=1;U(0x86AA18)=2;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);
 void *dll=LoadLibraryA("rounds.dll");if(!dll) ExitProcess(101);
 Hook setup=(Hook)GetProcAddress(dll,"@setup@8");
 Hook tick=(Hook)GetProcAddress(dll,"@tick@8");
 Snap snap=(Snap)GetProcAddress(dll,"snapshot");
 void (__stdcall *recovery)(u32)=(void *)GetProcAddress(dll,"_recovery@4");
 if(!setup||!tick||!snap||!recovery) ExitProcess(102);
 void *self=(void *)G(0x906000);
 fails+=setup(self,0)!=123;fails+=snap()!=0;fails+=U(0x901618)!=10000;fails+=U(0x904618)!=10000;
 recovery(G(0x900000));fails+=regen!=0;
 U(0x904618)=0;tick(self,0);fails+=(snap()&255)!=1;fails+=resets!=2;
 U(0x8738EC)=5400;tick(self,0);fails+=(snap()&255)!=1;fails+=(snap()>>12)!=3;
 U(0x901618)=0;U(0x904618)=0;tick(self,0);fails+=(snap()&255)!=1;fails+=(snap()>>12)!=3;
 U(0x901618)=0;tick(self,0);fails+=(snap()&255)!=17;
 U(0x901618)=6000;U(0x904618)=4000;U(0x8738EC)=5400;tick(self,0);
 fails+=(snap()&255)!=18;fails+=(snap()&256)==0;fails+=U(0x906100)!=1;
 setup(self,0);fails+=snap()!=0;
 /* Manual clock rewind aborts the old match without awarding a win. */
 U(0x904618)=0;tick(self,0);tick(self,0);U(0x8738EC)=0;tick(self,0);fails+=snap()!=0;
 /* A roster replacement starts a fresh match. */
 U(0x904618)=0;tick(self,0);U(0x86AC54)=G(0x909000);tick(self,0);fails+=snap()!=0;
 U(0x86AA1C)=3;recovery(G(0x900000));fails+=regen!=1;
 ExitProcess(fails);
}


