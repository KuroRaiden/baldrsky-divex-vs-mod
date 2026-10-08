/* One native 10x7 grid, independently retained choices. No save/asset edits. */
typedef void (__stdcall *MenuDispatch)(u32,u32);
static u32 selectOwner,selectManager,selectIndex[2],selectReady,selectCommitted;
static u32 selectLabels[2],selectLabelReady[2];
__declspec(dllexport) u32 selection_state(void){return selectReady|(selectIndex[0]<<8)|(selectIndex[1]<<16);}
static int shared_menu(u32 menu) {
 return training()&&menu&&menu==U(0xAE3B4C)&&F(menu,144)==2
  &&F(menu,352)&&F(menu,356)>=F(menu,352)+70*12;
}
static u32 mecha(u32 menu,u32 index){return F(F(menu,352)+12*index,0);}
static void clear_labels(void){
 for(u32 p=0;p<2;p++){if(selectLabels[p])((Method)F(F(selectLabels[p],0),44))((void *)selectLabels[p]);selectLabels[p]=0;}
}
static u32 index_of(u32 menu,u32 id){for(u32 i=0;i<70;i++)if(mecha(menu,i)==id)return i;return 0;}
static void begin_selection(u32 menu){
 /* Old labels belong to the old parent and have already been disposed. */
 selectLabels[0]=selectLabels[1]=0;selectOwner=menu;selectReady=selectManager=selectCommitted=0;
 selectIndex[0]=index_of(menu,U(0x86AA14));selectIndex[1]=index_of(menu,U(0x8734E8));
}
static void selection_labels(u32 menu){
 typedef void *(__cdecl *Allocate)(u32);
 typedef void *(__thiscall *Text)(void *,u32,u32,void *,void *,int,int,u32,int,int,int,int,int,int,int);
 typedef void *(__thiscall *Register)(void *,void *);
 typedef int (__thiscall *Position)(void *,int,int,int,int);
 for(u32 p=0;p<2;p++){
  u32 ready=(selectReady>>p)&1;
  if(selectLabels[p]&&selectLabelReady[p]!=ready){((Method)F(F(selectLabels[p],0),44))((void *)selectLabels[p]);selectLabels[p]=0;}
  u32 tile=F(U(0xAED7A4)+12*(697+selectIndex[p]),0);if(!tile)continue;
  int x=(int)F(tile,160)-18,y=(int)F(tile,164)+10+(p?14:0);
  if(!selectLabels[p]){
   /* Native short-string layout, kept inline: no allocation or destructor. */
   volatile u32 string[7];for(u32 i=0;i<7;i++)string[i]=0;
   string[1]=p?0x005032:0x005031;string[5]=2;string[6]=15;
   if(ready){string[1]=p?0x52205032:0x52205031;string[2]=0x59444145;string[3]=0;string[5]=8;}
   void *memory=((Allocate)G(0x419FF0))(0x67C);if(!memory)continue;
   void *label=((Text)G(0x736080))(memory,menu,169230,(void *)G(0xAE3640),(void *)string,x,y,p?0xFF70D0:0x50CFFF,1,10,256,0,0,0,0);
   selectLabels[p]=(u32)((Register)G(0x428360))((void *)menu,label);selectLabelReady[p]=ready;
  }
  if(selectLabels[p])((Position)G(0x731530))((void *)selectLabels[p],0,0,x,y);
 }
}
__declspec(dllexport) void __stdcall menu_dispatch(u32 manager,u32 id){
 u32 menu=U(0xAE3B4C);
 if(shared_menu(menu)&&id>=697&&id<767){
  if(selectOwner!=menu)begin_selection(menu);
  if(!(selectReady&1)){selectIndex[0]=id-697;selectReady|=1;selectManager=manager;}
  return;
 }
 ((MenuDispatch)G(0x7A3D80))(manager,id);
}
__declspec(dllexport) int __fastcall menu_close(void *self,void *unused){
 if(selectOwner==(u32)self){clear_labels();selectOwner=selectReady=selectManager=0;}
 return ((Method)G(0x54C110))(self);
}
__declspec(dllexport) int __fastcall menu_update(void *self,void *unused){
 u32 menu=(u32)self;
 if(!shared_menu(menu))return ((Method)G(0x54BE10))(self);
 int result=((Method)G(0x54BE10))(self);
 if(selectOwner!=menu)begin_selection(menu);
 if(!(selectReady&1))selectIndex[0]=index_of(menu,F(menu,272));
 u32 edge=U(0x874BD8);
 if(edge&0x200)selectReady&=~2u;
 if(!(selectReady&2)){
  int x=selectIndex[1]%10,y=selectIndex[1]/10;
  if(edge&1)y=(y+6)%7;else if(edge&0x10)y=(y+1)%7;
  if(edge&4)x=(x+9)%10;else if(edge&0x40)x=(x+1)%10;
  selectIndex[1]=y*10+x;
  if(edge&0x100)selectReady|=2;
 }
 selection_labels(menu);
 if(selectReady==3&&selectManager&&!selectCommitted){
  selectCommitted=1;
  /* Native commit handles P1's equipment/controller and menu transition.
     P2 reads the very same roster IDs at the next Training setup. */
  U(0x8734E8)=mecha(menu,selectIndex[1]);F(menu,272)=mecha(menu,selectIndex[0]);
  ((MenuDispatch)G(0x7A3D80))(selectManager,697+selectIndex[0]);
 }
 return result;
}

