
#include <stdint.h>


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
void div_blk( uint32_t *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=polylen-1;i>=si_h;i--) {
      poly[(i-deg_diff)*4]   ^= poly[i*4];
      poly[(i-deg_diff)*4+1] ^= poly[i*4+1];
      poly[(i-deg_diff)*4+2] ^= poly[i*4+2];
      poly[(i-deg_diff)*4+3] ^= poly[i*4+3];
  }
}

static
void rep_in_si( uint32_t *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=polyloglen_blk-1;i>=si;i--) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) div_blk( data+j*4 , si_h, si_l, polylen );
  }
}

static
void cvt( uint32_t *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);
  rep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );

  cvt( data , datalen , logsize_blk , si );
  cvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
}


////////

static inline
void idiv_blk( uint32_t *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=si_h;i<polylen;i++) {
      poly[(i-deg_diff)*4]   ^= poly[i*4];
      poly[(i-deg_diff)*4+1] ^= poly[i*4+1];
      poly[(i-deg_diff)*4+2] ^= poly[i*4+2];
      poly[(i-deg_diff)*4+3] ^= poly[i*4+3];
  }
}

static
void irep_in_si( uint32_t *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=si;i<polyloglen_blk;i++) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) idiv_blk( data+j*4 , si_h, si_l, polylen );
  }
}

static
void icvt( uint32_t *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);

  icvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
  icvt( data , datalen , logsize_blk , si );

  irep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );
}


#include "bc_128.h"

void bc_128( void * poly, unsigned n_128 )
{
    cvt( (uint32_t *)poly , n_128 , 0 , __builtin_ctz(n_128)  );
}

void ibc_128( void * poly, unsigned n_128 )
{
    icvt( (uint32_t *)poly , n_128 , 0 , __builtin_ctz(n_128)  );
}



