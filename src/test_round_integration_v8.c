/* Run beside the freshly compiled rounds.dll. Own-code mock, no game needed. */
typedef unsigned int u32;
#define G(a) ((a)+0x20000000u)
#define U(a) (*(volatile u32 *)G(a))
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,u32,u32,u32);
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(void *,const char *);
__declspec(dllimport) void __stdcall ExitProcess(u32);
static int resets,regen,renders,strings;
static u32 rendered[2],observers[2];
static int __fastcall reset(void *self,void *unused){++resets;U(0x8738EC)=0;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);return 123;}
static int __fastcall update(void *self,void *unused){++U(0x8738EC);return -1;}
static void __stdcall recover(u32 fighter){++regen;}
static int __cdecl format(char *buffer,const char *fmt,...){buffer[0]=0;return 0;}
static int __stdcall title(u32 hwnd,const char *text){return 1;}
static int __fastcall xy(void *self,void *unused){return 100;}
static void __fastcall name(void *self,void *unused,u32 *out){for(int i=0;i<7;i++)out[i]=0;out[5]=3;out[6]=15;out[1]=0x454E4F;}
static int __fastcall destroy(void *self,void *unused){++strings;return 0;}
static void __fastcall render(void *self,void *unused,int a,int b,int c){if(renders<2){rendered[renders]=*(u32 *)((u32)self+264);observers[renders]=U(0x86AC48);}++renders;}
static void jump(u32 addr,void *fn){*(unsigned char *)G(addr)=0xE9;U(addr+1)=(u32)fn-G(addr)-5;}
typedef int (__fastcall *Hook)(void *,void *);
typedef u32 (__cdecl *Snap)(void);
void entry(void) {
 int fails=0;
 /* Use a formatter shim outside Windows' low-address allocations. */
 static u32 pages[]={0x5F0000,0x670000,0x7D0000,0x860000,0x870000,0x900000,0x410000,0x4A0000,0x4C0000,0xCB0000};
 for(int i=0;i<10;i++) if(!VirtualAlloc((void *)G(pages[i]),0x10000,0x3000,0x40)) ExitProcess(110+i);
 jump(0x5FB8C0,reset);jump(0x5F8CC0,update);jump(0x5F8D50,recover);jump(0x676305,format);
 jump(0x4AC310,name);jump(0x41A300,destroy);jump(0xCBE411,render);jump(0x4CC050,render);
 U(0x7D9328)=(u32)title;
 U(0x86AA1C)=1;U(0x86AA18)=2;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);
 void *dll=LoadLibraryA("rounds.dll");if(!dll) ExitProcess(101);
 Hook setup=(Hook)GetProcAddress(dll,"@setup@8");
 Hook tick=(Hook)GetProcAddress(dll,"@tick@8");
 Snap snap=(Snap)GetProcAddress(dll,"snapshot");
 void (__stdcall *recovery)(u32)=(void *)GetProcAddress(dll,"_recovery@4");
 if(!setup||!tick||!snap||!recovery) ExitProcess(102);
 void (__fastcall *cursor)(void *,void *,int,int,int)=(void *)GetProcAddress(dll,"@cursor@20");if(!cursor)ExitProcess(103);
 void *self=(void *)G(0x906000);
 if(setup(self,0)!=123) ExitProcess(1);if(snap()!=0) ExitProcess(2);if(U(0x901618)!=10000) ExitProcess(3);if(U(0x904618)!=10000) ExitProcess(4);
 recovery(G(0x900000));if(regen!=0) ExitProcess(5);
 U(0x904618)=0;tick(self,0);if((snap()&255)!=1) ExitProcess(6);if(resets!=2) ExitProcess(7);
 U(0x8738EC)=5400;tick(self,0);if((snap()&255)!=1) ExitProcess(8);if((snap()>>12)!=3) ExitProcess(9);
 U(0x901618)=0;U(0x904618)=0;tick(self,0);if((snap()&255)!=1) ExitProcess(10);if((snap()>>12)!=3) ExitProcess(11);
 U(0x901618)=0;tick(self,0);if((snap()&255)!=17) ExitProcess(12);
 U(0x901618)=6000;U(0x904618)=4000;U(0x8738EC)=5400;tick(self,0);
 if((snap()&255)!=18) ExitProcess(13);if((snap()&256)==0) ExitProcess(14);if(U(0x906100)!=1) ExitProcess(15);
 setup(self,0);if(snap()!=0) ExitProcess(16);
 /* Manual clock rewind aborts the old match without awarding a win. */
 U(0x904618)=0;tick(self,0);tick(self,0);U(0x8738EC)=0;tick(self,0);if(snap()!=0) ExitProcess(17);
 /* A roster replacement starts a fresh match. */
 U(0x904618)=0;tick(self,0);U(0x86AC54)=G(0x909000);tick(self,0);if(snap()!=0) ExitProcess(18);
 U(0x86AA1C)=3;recovery(G(0x900000));if(regen!=1) ExitProcess(19);
 U(0x86AA1C)=1;setup(self,0);
 U(0x86AC54)=0;tick(self,0);if((snap()&255)!=1) ExitProcess(20);if(U(0x86AC54)!=G(0x903000)) ExitProcess(21);
 U(0x86AC48)=0;tick(self,0);if((snap()&255)!=17) ExitProcess(22);if(U(0x86AC48)!=G(0x900000)) ExitProcess(23);
 U(0x86AC48)=0;U(0x86AC54)=0;tick(self,0);if((snap()&255)!=17) ExitProcess(24);if((snap()>>12)!=3) ExitProcess(25);
 if(U(0x901614)!=1) ExitProcess(26);if(U(0x904614)!=1) ExitProcess(27);
 setup(self,0);
 U(0x86AC78)=G(0x90D000);U(0x90D1D4)=G(0x90C000);U(0x90C108)=G(0x903000);
 U(0x900000)=G(0x90F000);U(0x903000)=G(0x90F000);U(0x90F04C)=(u32)xy;U(0x90F050)=(u32)xy;
 U(0x901834)=G(0x90E000);U(0x904834)=G(0x90E100);
 cursor((void *)G(0x90C000),0,1,2,3);
 if(renders!=2) ExitProcess(28);if(strings!=1) ExitProcess(29);if(rendered[0]!=G(0x903000)) ExitProcess(30);if(rendered[1]!=G(0x900000)) ExitProcess(31);
 if(observers[1]!=G(0x903000)) ExitProcess(32);if(U(0x86AC48)!=G(0x900000)) ExitProcess(33);
 ExitProcess(fails);
}



