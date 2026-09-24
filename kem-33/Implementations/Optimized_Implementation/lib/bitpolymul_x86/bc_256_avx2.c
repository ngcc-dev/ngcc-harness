
#include "bc_256.h"


#include <emmintrin.h>
#include <immintrin.h>




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
void div_blk( __m256i *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=polylen-1;i>=si_h;i--) {
      __m256i d = _mm256_loadu_si256(poly+i-deg_diff);
      __m256i p = _mm256_loadu_si256(poly+i);
      d ^= p;
      _mm256_storeu_si256( poly+(i-deg_diff) , d );
      //poly[i-deg_diff] ^= poly[i];
  }
}

static
void rep_in_si( __m256i *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=polyloglen_blk-1;i>=si;i--) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) div_blk( data+j , si_h, si_l, polylen );
  }
}

static
void cvt( __m256i *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);
  rep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );

  cvt( data , datalen , logsize_blk , si );
  cvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
}




void bc_256( void * poly, unsigned n_256 ) {  cvt( (__m256i*) poly , n_256 , 0 , __builtin_ctz(n_256)  ); }


////////

static inline
void idiv_blk( __m256i *poly, int si_h, int si_l, int polylen )
{
  int deg_diff = si_h-si_l;
  for(int i=si_h;i<polylen;i++) {
      __m256i d = _mm256_loadu_si256(poly+i-deg_diff);
      __m256i p = _mm256_loadu_si256(poly+i);
      d ^= p;
      _mm256_storeu_si256( poly+(i-deg_diff) , d );
      //poly[i-deg_diff] ^= poly[i];
  }
}

static
void irep_in_si( __m256i *data, int datalen, int logsize_blk, int polyloglen_blk,  int si  )
{
  for(int i=si;i<polyloglen_blk;i++) {
    int polylen = (1<<(i+logsize_blk+1));
    int si_h = (1<<(i+logsize_blk));
    int si_l = (1<<(i+logsize_blk-si));
    for(int j=0;j<datalen;j+=polylen) idiv_blk( data+j , si_h, si_l, polylen );
  }
}

static
void icvt( __m256i *data, int datalen, int logsize_blk, int polyloglen_blk )
{
  if( 1 >= polyloglen_blk ) return;
  int si = choose_si(polyloglen_blk);

  icvt( data , datalen , logsize_blk+si , polyloglen_blk-si );
  icvt( data , datalen , logsize_blk , si );

  irep_in_si( data, datalen , logsize_blk , polyloglen_blk , si );
}



void ibc_256( void * poly, unsigned n_256 ) { icvt( (__m256i*) poly , n_256 , 0 , __builtin_ctz(n_256)  ); }



