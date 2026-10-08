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
static u32 trackerRefs[45],managerObserved,playerObserved,rowObserved,xObserved;
static int trackerBuilds,trackerDrops,vectorDrops,fighterUpdates;
static int __fastcall tracker_init(u32 *self,void *unused){for(int i=0;i<13;i++)self[i]=0;return 0;}
static int __fastcall tracker_build(u32 *self,void *unused){++trackerBuilds;int rows=U(0x8734C4)==1?15:1;self[4]=(u32)trackerRefs;self[5]=self[4]+12*rows;
 for(int i=0;i<rows;i++){u32 row=G(0x910000)+428*i;for(int j=0;j<107;j++)*(u32 *)(row+4*j)=0;*(u32 *)row=G(0x91E000);*(u32 *)(row+160)=self[7];*(u32 *)(row+280)=*(u32 *)(U(0x86AC48)+340);trackerRefs[3*i]=row;trackerRefs[3*i+1]=trackerRefs[3*i+2]=0;}return 0;}
static int __fastcall tracker_dispose(void *self,void *unused){++trackerDrops;for(int i=0;i<15;i++)if(trackerRefs[3*i]==(u32)self)trackerRefs[3*i]=0;return 0;}
static int __fastcall vector_destroy(void *self,void *unused){++vectorDrops;return 0;}
static int __fastcall fighter_native(void *self,void *unused,unsigned char *state){++fighterUpdates;managerObserved=U(0x86AAB0);playerObserved=U(0x86AC48);return 9;}
static void __fastcall tracker_native_render(void *self,void *unused,int surface,int x,int y){managerObserved=U(0x86AAB0);playerObserved=U(0x86AC48);rowObserved=*(u32 *)((u32)self+280);xObserved=*(u32 *)((u32)self+160);}
static u32 rendered[2],observers[2];
static int __fastcall reset(void *self,void *unused){++resets;U(0x8738EC)=0;U(0x86AC48)=G(0x900000);U(0x86AC54)=G(0x903000);return 123;}
static int __fastcall update(void *self,void *unused){++U(0x8738EC);return -1;}
static void __stdcall recover(u32 fighter){++regen;}
static int __cdecl format(char *buffer,const char *fmt,...){buffer[0]=0;return 0;}
static int __stdcall title(u32 hwnd,const char *text){return 1;}
static int __fastcall xy(void *self,void *unused){return 100;}
static void __fastcall name(void *self,void *unused,u32 *out){for(int i=0;i<7;i++)out[i]=0;out[5]=3;out[6]=15;out[1]=0x454E4F;}
static int __fastcall destroy(void *self,void *unused){++strings;return 0;}
static int labelAllocs,labelDisposals,menuCommits;
static u32 chosen1,chosen2,positionX[2],positionY[2];
static void *__cdecl allocate(u32 size){if(size==0x67C)return(void *)(G(0x930000)+0x800*labelAllocs++);if(size!=504)ExitProcess(60);++allocs;return(void *)G(0x90B000);}
static int __fastcall label_dispose(void *self,void *unused){++labelDisposals;return 0;}
static void *__fastcall label_construct(void *self,void *unused,u32 parent,u32 layer,void *font,u32 *text,int x,int y,u32 color,int align,int spacing,int alpha,int a,int b,int c,int d){
 if(parent!=G(0x920000)||layer!=169230||font!=(void *)G(0xAE3640)||align!=1||spacing!=10||alpha!=256||(text[5]!=2&&text[5]!=8)||text[6]!=15)ExitProcess(150);
 for(int i=0;i<415;i++)((u32 *)self)[i]=0;*(u32 *)self=G(0x93F000);U(0x93F02C)=(u32)label_dispose;return self;
}
static void *__fastcall label_register(void *self,void *unused,void *object){return object;}
static int __fastcall label_position(void *self,void *unused,int a,int b,int x,int y){if(a||b)ExitProcess(151);*(u32 *)((u32)self+160)=x;*(u32 *)((u32)self+164)=y;return 0;}
static int __fastcall menu_native(void *self,void *unused){return 77;}
static void __stdcall dispatch_native(u32 manager,u32 id){++menuCommits;chosen1=U(0x920110);chosen2=U(0x8734E8);if(manager!=G(0x92F000))ExitProcess(152);}
static void *__fastcall construct(void *self,void *unused,u32 fighter,u32 other){constructObserver=U(0x86AC48);for(int i=0;i<156;i++)((u32 *)self)[i]=0;((u32 *)self)[0]=G(0x90E800);((u32 *)self)[66]=fighter;((u32 *)self)[36]=3;return self;}
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
 static u32 pages[]={0x5F0000,0x670000,0x7D0000,0x860000,0x870000,0x900000,0x410000,0x4A0000,0x4C0000,0xCB0000,0x560000,0x610000,0x5E0000,0x480000,0x910000,0xAE0000,0x540000,0x420000,0x730000,0x7A0000,0x920000,0x930000};
 for(int i=0;i<22;i++) if(!VirtualAlloc((void *)G(pages[i]),0x10000,0x3000,0x40)) ExitProcess(110+i);
 jump(0x5FB8C0,reset);jump(0x5F8CC0,update);jump(0x5F8D50,recover);jump(0x676305,format);
 jump(0x4AC310,name);jump(0x41A300,destroy);jump(0xCBE411,render);jump(0x4CC050,render);
 jump(0x419FF0,allocate);jump(0x4CC310,construct);jump(0x4AC3A0,register_cursor);jump(0x565670,setref);jump(0x4CBD50,animate);
 jump(0x5E5540,tracker_init);jump(0x5E5E00,tracker_build);jump(0x481D20,vector_destroy);jump(0x61E390,fighter_native);jump(0x5E7430,tracker_native_render);U(0x91E02C)=(u32)tracker_dispose;
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
 /* Independent native P2 tracker, scoped update/render and reset lifecycle. */
U(0x86AA1C)=1;U(0x8734C4)=1;U(0x86AAB0)=G(0x90D800);U(0x900154)=G(0x90A100);U(0x903154)=G(0x90A500);
Snap tracker=(Snap)GetProcAddress(dll,"second_tracker");
int (__fastcall *fighter)(void *,void *,unsigned char *)=(void *)GetProcAddress(dll,"@fighter_update@12");
void (__fastcall *tracker_render)(void *,void *,int,int,int)=(void *)GetProcAddress(dll,"@tracker_render@20");if(!tracker||!fighter||!tracker_render)ExitProcess(90);
setup(self,0);if(!tracker()||trackerBuilds!=1)ExitProcess(91);
if(*(u32 *)(tracker()+28)!=570||*(u32 *)(tracker()+20)-*(u32 *)(tracker()+16)!=180)ExitProcess(92);
for(int i=0;i<15;i++)if(*(u32 *)(trackerRefs[3*i]+280)!=G(0x90A500))ExitProcess(93);
unsigned char state=0;if(fighter((void *)G(0x903000),0,&state)!=9||managerObserved!=tracker()||playerObserved!=G(0x900000)||U(0x86AAB0)!=G(0x90D800))ExitProcess(94);
fighter((void *)G(0x900000),0,&state);if(managerObserved!=G(0x90D800)||fighterUpdates!=2)ExitProcess(95);
tracker_render((void *)trackerRefs[0],0,1,0,0);if(managerObserved!=tracker()||playerObserved!=G(0x903000)||rowObserved!=G(0x90A500)||xObserved!=570||U(0x86AC48)!=G(0x900000)||U(0x86AAB0)!=G(0x90D800))ExitProcess(96);
tracker_render((void *)G(0x90C000),0,1,0,0);if(playerObserved!=G(0x900000)||managerObserved!=G(0x90D800))ExitProcess(97);
tick(self,0);if(trackerBuilds!=1)ExitProcess(98);
U(0x904618)=0;tick(self,0);if(trackerBuilds!=2||trackerDrops!=15||vectorDrops!=1||countdown()!=3)ExitProcess(99);
U(0x8734C4)=2;tick(self,0);if(trackerBuilds!=3||trackerDrops!=30||vectorDrops!=2)ExitProcess(100);
U(0x86AA1C)=3;tick(self,0);if(tracker()||trackerDrops!=31||vectorDrops!=3)ExitProcess(105);
/* Shared native roster: independent cursors, both confirmation orders,
   wrap, ready freeze, cancel/re-entry and delegation outside Training. */
jump(0x54BE10,menu_native);jump(0x54C110,menu_native);jump(0x7A3D80,dispatch_native);
jump(0x736080,label_construct);jump(0x428360,label_register);jump(0x731530,label_position);
Hook menu_update=(Hook)GetProcAddress(dll,"@menu_update@8"),menu_close=(Hook)GetProcAddress(dll,"@menu_close@8");
void (__stdcall *menu_dispatch)(u32,u32)=(void *)GetProcAddress(dll,"_menu_dispatch@8");Snap selection=(Snap)GetProcAddress(dll,"selection_state");
if(!menu_update||!menu_close||!menu_dispatch||!selection)ExitProcess(153);
U(0x86AA1C)=1;U(0xAE3B4C)=G(0x920000);U(0x920090)=2;U(0x920160)=G(0x921000);U(0x920164)=G(0x921000)+70*12;
U(0xAED7A4)=G(0xAE8000);
for(int i=0;i<70;i++){U(0x921000+12*i)=1000+3*i;U(0x921008+12*i)=2000+i;u32 tile=G(0x922000)+440*i;U(0xAE8000+12*(697+i))=tile;*(u32 *)(tile+160)=310+50*(i%10);*(u32 *)(tile+164)=65+53*(i/10);}
U(0x86AA14)=1006;U(0x8734E8)=1009;U(0x920110)=1006;U(0x874BD8)=0;
void *menu=(void *)G(0x920000);menu_update(menu,0);if(selection()!=((2<<8)|(3<<16))||labelAllocs!=2)ExitProcess(154);
menu_dispatch(G(0x92F000),699);if(menuCommits||!(selection()&1))ExitProcess(155);
U(0x874BD8)=0x40;menu_update(menu,0);if((selection()>>16)!=4||U(0x8734E8)!=1009)ExitProcess(156);
U(0x874BD8)=0x100;menu_update(menu,0);if(menuCommits!=1||chosen1!=1006||chosen2!=1012)ExitProcess(157);
menu_update(menu,0);if(menuCommits!=1)ExitProcess(158);
menu_close(menu,0);if(labelDisposals!=4)ExitProcess(159);
U(0x874BD8)=0;U(0x86AA14)=1000;U(0x8734E8)=1000;U(0x920110)=1000;menu_update(menu,0);
U(0x874BD8)=5;menu_update(menu,0);if((selection()>>16)!=69)ExitProcess(160);
U(0x874BD8)=0x100;menu_update(menu,0);if(menuCommits!=1||selection()%256!=2)ExitProcess(161);
U(0x874BD8)=0x40;menu_update(menu,0);if((selection()>>16)!=69)ExitProcess(162);
menu_dispatch(G(0x92F000),697);U(0x874BD8)=0;menu_update(menu,0);if(menuCommits!=2||chosen1!=1000||chosen2!=1207)ExitProcess(163);
menu_close(menu,0);menu_update(menu,0);U(0x874BD8)=0x100;menu_update(menu,0);U(0x874BD8)=0x200;menu_update(menu,0);if(selection()%256)ExitProcess(164);
menu_close(menu,0);U(0x86AA1C)=3;menu_dispatch(G(0x92F000),699);if(menuCommits!=3)ExitProcess(165);
ExitProcess(fails);
}







