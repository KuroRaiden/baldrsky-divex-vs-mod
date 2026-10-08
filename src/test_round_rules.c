#include "round_rules.h"
__declspec(dllimport) void __stdcall ExitProcess(unsigned int);
void entry(void) {
 int fails=0;
 fails+=round_winner(0,10000,10000,10000)!=2;
 fails+=round_winner(10000,10000,0,10000)!=1;
 fails+=round_winner(0,10000,0,10000)!=0;
 fails+=round_winner(5000,10000,5000,10000)!=0;
 fails+=round_winner(8000,10000,5000,10000)!=1;
 fails+=round_winner(5000,10000,8000,10000)!=2;
 fails+=round_winner(3000,4000,6000,10000)!=1;
 fails+=round_winner(2000,4000,5000,10000)!=0;
 fails+=round_winner(10000,10000,9999,10000)!=1;
 fails+=round_winner(1,0,1,10000)!=0;
 ExitProcess(fails);
}
