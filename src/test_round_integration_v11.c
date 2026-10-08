/* Run beside the freshly compiled rounds.dll. Own-code mock, no game needed. */
typedef unsigned int u32;
#define G(a) ((a)+0x20000000u)
#define U(a) (*(volatile u32 *)G(a))
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,u32,u32,u32);
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(void *,const char *);
__declspec(dllimport) void __stdcall ExitProcess(u32);
static int resets,regen,renders,strings,allocs,drops,attackCalls;
static u32 constructObserver,updateObserver,rangeState;
static u32 rendered[2],observers[2];
static int __fastcall reset(void *self,void *unused){++resets;U(0x8738EC)=0;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);return 123;}
static int __fastcall update(void *self,void *unused){++U(0x8738EC);return -1;}
static void __stdcall recover(u32 fighter){++regen;}
static int __cdecl format(char *buffer,const char *fmt,...){buffer[0]=0;return 0;}
static int __stdcall title(u32 hwnd,const char *text){return 1;}
static int __fastcall xy(void *self,void *unused){return 100;}
static void __fastcall name(void *self,void *unused,u32 *out){for(int i=0;i<7;i++)out[i]=0;out[5]=3;out[6]=15;out[1]=0x454E4F;}
static int __fastcall destroy(void *self,void *unused){++strings;return 0;}
static void *__cdecl allocate(u32 size){if(size!=504)ExitProcess(60);++allocs;return(void *)G(0x90B000);}
static void *__fastcall construct(void *self,void *unused,u32 fighter,u32 other){constructObserver=U(0x86AC48);for(int i=0;i<126;i++)((u32 *)self)[i]=0;((u32 *)self)[0]=G(0x90E800);((u32 *)self)[66]=fighter;((u32 *)self)[36]=3;return self;}
static void *__fastcall register_cursor(void *self,void *unused,u32 parent,void *object){return object;}
static void *__fastcall setref(u32 *ref,void *unused,void *object){ref[0]=(u32)object;ref[1]=ref[2]=0;return ref;}
static int __fastcall dispose(void *self,void *unused){++drops;((u32 *)self)[36]=6;return 0;}
static int __fastcall animate(void *self,void *unused){updateObserver=U(0x86AC48);rangeState=U(updateObserver==G(0x903000)?0x904500:0x901500);return 7;}
static void __fastcall render(void *self,void *unused,int a,int b,int c){if(renders<2){rendered[renders]=*(u32 *)((u32)self+264);observers[renders]=U(0x86AC48);}++renders;}
static void jump(u32 addr,void *fn){*(unsigned char *)G(addr)=0xE9;U(addr+1)=(u32)fn-G(addr)-5;}
typedef int (__fastcall *Hook)(void *,void *);
typedef u32 (__cdecl *Snap)(void);
static Hook rawtick;
static int __fastcall battleTick(void *self,void *unused){if(U(0x8738EC)<180)U(0x8738EC)=180;return rawtick(self,unused);}
static int __fastcall attack(void *self,void *unused){++attackCalls;return 42;}
void entry(void) {
 int fails=0;
 /* Use a formatter shim outside Windows' low-address allocations. */
 static u32 pages[]={0x5F0000,0x670000,0x7D0000,0x860000,0x870000,0x900000,0x410000,0x4A0000,0x4C0000,0xCB0000,0x560000,0x610000};
 for(int i=0;i<12;i++) if(!VirtualAlloc((void *)G(pages[i]),0x10000,0x3000,0x40)) ExitProcess(110+i);
 jump(0x5FB8C0,reset);jump(0x5F8CC0,update);jump(0x5F8D50,recover);jump(0x676305,format);
 jump(0x4AC310,name);jump(0x41A300,destroy);jump(0xCBE411,render);jump(0x4CC050,render);
 jump(0x419FF0,allocate);jump(0x4CC310,construct);jump(0x4AC3A0,register_cursor);jump(0x565670,setref);jump(0x4CBD50,animate);
 jump(0x61A280,attack);U(0x7D9328)=(u32)title;
 U(0x86AA1C)=1;U(0x86AA18)=2;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);
 void *dll=LoadLibraryA("rounds.dll");if(!dll) ExitProcess(101);
 Hook setup=(Hook)GetProcAddress(dll,"@setup@8");
 rawtick=(Hook)GetProcAddress(dll,"@tick@8");Hook tick=battleTick;
 Snap snap=(Snap)GetProcAddress(dll,"snapshot");
 void (__stdcall *recovery)(u32)=(void *)GetProcAddress(dll,"_recovery@4");
 if(!setup||!tick||!snap||!recovery) ExitProcess(102);
 void (__fastcall *cursor)(void *,void *,int,int,int)=(void *)GetProcAddress(dll,"@cursor@20");if(!cursor)ExitProcess(103);
 Snap secondary=(Snap)GetProcAddress(dll,"second_cursor");Hook cursor_update=(Hook)GetProcAddress(dll,"@cursor_update@8");if(!secondary||!cursor_update)ExitProcess(104);
 void *self=(void *)G(0x906000);
 if(setup(self,0)!=123) ExitProcess(1);if(snap()!=0) ExitProcess(2);if(U(0x901618)!=10000) ExitProcess(3);if(U(0x904618)!=10000) ExitProcess(4);
 Hook select=(Hook)GetProcAddress(dll,"@attack_select@8");Snap countdown=(Snap)GetProcAddress(dll,"countdown");if(!select||!countdown)ExitProcess(70);
if(countdown()!=3)ExitProcess(71);
U(0x9014A8)=G(0x90F000);U(0x9044A8)=G(0x90F400);
for(int i=0;i<64;i++){U(0x90F004+4*i)=0xF55;U(0x90F104+4*i)=0xF55;U(0x90F404+4*i)=0xF55;U(0x90F504+4*i)=0xF55;}
U(0x9015AC)=0xF55;U(0x9015B0)=0xF55;U(0x9045AC)=0xF55;U(0x9045B0)=0xF55;
if(select((void *)G(0x900000),0)!=-1||select((void *)G(0x903000),0)!=-1||attackCalls)ExitProcess(72);
if(U(0x9015AC)!=0x855||U(0x9045B0)!=0x855)ExitProcess(73);
for(int i=0;i<64;i++)if(U(0x90F004+4*i)!=0x855||U(0x90F504+4*i)!=0x855)ExitProcess(74);
/* The selector is called after movement. Position and direction/dash bits survive. */
U(0x9000A0)=1234;U(0x9000A4)=5678;select((void *)G(0x900000),0);if(U(0x9000A0)!=1234||U(0x9000A4)!=5678||U(0x9015B0)!=0x855)ExitProcess(75);
if(select((void *)G(0x909000),0)!=42||attackCalls!=1)ExitProcess(76);
U(0x8738EC)=60;if(countdown()!=2)ExitProcess(77);U(0x8738EC)=120;if(countdown()!=1)ExitProcess(78);
U(0x8738EC)=179;if(select((void *)G(0x900000),0)!=-1)ExitProcess(79);
U(0x8738EC)=180;if(countdown()!=0||select((void *)G(0x900000),0)!=42||select((void *)G(0x903000),0)!=42)ExitProcess(80);
setup(self,0);U(0x904618)=0;rawtick(self,0);if(snap()!=0||countdown()!=3||resets!=2)ExitProcess(81);
U(0x904618)=10000;
recovery(G(0x900000));if(regen!=0) ExitProcess(5);
 U(0x874B98)=0x01000001;U(0x904618)=0;tick(self,0);if(snap()!=0) ExitProcess(40);U(0x874B98)=0x01000000;U(0x904618)=0;tick(self,0);if((snap()&255)!=1) ExitProcess(6);if(resets!=3) ExitProcess(7);if(countdown()!=3)ExitProcess(82);
 U(0x8738EC)=5580;tick(self,0);if((snap()&255)!=1) ExitProcess(8);if((snap()>>12)!=3) ExitProcess(9);
 U(0x901618)=0;U(0x874B98)=0x01000000;U(0x904618)=0;tick(self,0);if((snap()&255)!=1) ExitProcess(10);if((snap()>>12)!=3) ExitProcess(11);
 U(0x901618)=0;tick(self,0);if((snap()&255)!=17) ExitProcess(12);
 U(0x901618)=6000;U(0x904618)=4000;U(0x8738EC)=5580;tick(self,0);
 if((snap()&255)!=18) ExitProcess(13);if((snap()&256)==0) ExitProcess(14);if(U(0x906100)!=1) ExitProcess(15);
 setup(self,0);if(snap()!=0||countdown()!=3) ExitProcess(16);
U(0x901618)=6000;U(0x904618)=4000;U(0x8738EC)=5578;rawtick(self,0);if(snap()!=0)ExitProcess(83);
rawtick(self,0);if((snap()&255)!=1||countdown()!=3)ExitProcess(84);
setup(self,0);
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
 U(0x877640)=G(0x90A000);U(0x90E82C)=(u32)dispose;
 tick(self,0);if(!secondary()||allocs!=1)ExitProcess(28);if(constructObserver!=G(0x903000))ExitProcess(29);
 cursor((void *)G(0x90C000),0,1,2,3);
 cursor((void *)secondary(),0,1,2,3);
 if(renders!=2) ExitProcess(61);if(rendered[0]!=G(0x903000)) ExitProcess(30);if(rendered[1]!=G(0x900000)) ExitProcess(31);
 if(observers[1]!=G(0x903000)) ExitProcess(32);if(U(0x86AC48)!=G(0x900000)) ExitProcess(33);
 U(0x901500)=0;U(0x904500)=1;
 if(cursor_update((void *)secondary(),0)!=7||updateObserver!=G(0x903000)||rangeState!=1)ExitProcess(34);
 U(0x901500)=1;U(0x904500)=0;
 cursor_update((void *)secondary(),0);if(rangeState!=0||U(0x86AC48)!=G(0x900000))ExitProcess(35);
 cursor_update((void *)G(0x90C000),0);if(updateObserver!=G(0x900000)||rangeState!=1)ExitProcess(36);
 tick(self,0);if(allocs!=1)ExitProcess(37);
 setup(self,0);if(secondary()||drops!=1)ExitProcess(38);
 tick(self,0);if(!secondary()||allocs!=2)ExitProcess(39);
 U(0x904618)=0;tick(self,0);if(!secondary()||drops!=2||allocs!=3)ExitProcess(41);
 U(0x86AA1C)=3;cursor_update((void *)secondary(),0);if(secondary()||drops!=3)ExitProcess(42);
 ExitProcess(fails);
}







