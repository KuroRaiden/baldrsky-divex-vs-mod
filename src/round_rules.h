/* Integration bounds each health maximum to 10000, preventing overflow. */
static int round_winner(int h1,int m1,int h2,int m2) {
 if(h1<=0 || h2<=0) return h1<=0 ? (h2<=0?0:2):1;
 if(m1<=0 || m2<=0) return 0;
 if(h1*m2==h2*m1) return 0;
 return h1*m2>h2*m1 ? 1:2;
}
