
#include "bc_8.h"




static inline
int choose_si( int loglen )
{
  int si = 1<<0;
  for( int i=1; (1<<i) < loglen ; i++ ) {
    si = 1<<i;
  }
  return si;
}

/////////////////

static inline
void div_blk( uint8_t *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=polylen-1;i>=si_h;i--) {  poly[(i-deg_diff)] ^= poly[i]; }
}

static
void rep_in_si( uint8_t *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=polyloglen_blk-1;i>=si;i--) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) div_blk( data+j , si_h, si_l, polylen );
  }
}

static
void cvt( uint8_t *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);
  rep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );

  cvt( data , datalen , logsize_blk , si );
  cvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
}


static inline void bc_8_128( uint8_t * poly )
{
  // poly has 16 terms
  // div s2^2 = ( x^8 - x^2 )
  div_blk( poly , 8 , 2 , 16 );
  // 2 x div s2 = x^4 - x
  div_blk( poly+8 , 4 , 1 , 8 );
  div_blk( poly , 4 , 1 , 8 );
  // div s1 = x^2 - x , each coef has 4 terms
  div_blk( poly , 2*4 , 1*4 , 4*4 );
  // 4 x div s1 = x^2 - x
  div_blk( poly+12 , 2 , 1 , 4 );
  div_blk( poly+8 , 2 , 1 , 4 );
  div_blk( poly+4 , 2 , 1 , 4 );
  div_blk( poly , 2 , 1 , 4 );
}

// s4 = x^16 - x
// div   s4^i , s4^(i-1) , ... , s4^1
static inline
void repr_s4( uint8_t * poly , unsigned n_8 )
{
  unsigned log_n = __builtin_ctz( n_8 );
  for( int i=log_n-4-1;i>=0;i--) {
    unsigned s_h = 1<<(i+4);
    unsigned s_l = 1<<i;
    unsigned len = 1<<(i+5);
    // div X^{16*s_ht} - X^{s_tt}
    for(unsigned j=0;j<n_8;j+=len) { div_blk( poly+j , s_h , s_l , len ); }
  }
}

#include "bc_128.h"

void bc_8( uint8_t * poly , unsigned n_8 )
{
  if(32>=n_8) { cvt( poly , n_8 , 0 , __builtin_ctz(n_8) ); return; }
  repr_s4( poly , n_8 );
  bc_128( poly , n_8>>4 );
  for(unsigned i=0;i<n_8;i+=16) { bc_8_128( poly + i ); }
}

////////

static inline
void idiv_blk( uint8_t *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=si_h;i<polylen;i++) { poly[(i-deg_diff)] ^= poly[i]; }
}

static
void irep_in_si( uint8_t *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=si;i<polyloglen_blk;i++) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) idiv_blk( data+j , si_h, si_l, polylen );
  }
}

static
void icvt( uint8_t *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);

  icvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
  icvt( data , datalen , logsize_blk , si );

  irep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );
}

void ibc_8( uint8_t * poly , unsigned n_8 )
{
  icvt( poly , n_8 , 0 , __builtin_ctz(n_8) );
}


