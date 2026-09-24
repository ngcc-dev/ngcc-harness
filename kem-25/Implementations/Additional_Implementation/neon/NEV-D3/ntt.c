#include "avx2_to_neon.h"
#include "ntt.h"
#include "params.h"
#include "align.h"

#if PARAM_Q == 641

ALIGN(32) int16_t zetas_avx[176] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,-16,-16,-16,-16,-16,-16,-16,-16,100,100,100,100,100,100,100,100,250,250,250,250,250,250,250,250,40,40,40,40,40,40,40,40,-10,-10,-10,-10,-258,-258,-258,-258,-4,-4,-4,-4,25,25,25,25,-282,-282,-282,-282,160,160,160,160,-241,-241,-241,-241,64,64,64,64,-32,-32,200,200,-141,-141,80,80,-5,-5,-129,-129,-2,-2,-308,-308,77,77,320,320,159,159,128,128,-8,-8,50,50,125,125,20,20,29,-21,268,248,305,177,122,199,-210,-290,-84,-116,-153,155,67,62,-31,-287,244,-243,-105,-145,-42,-58,-306,310,134,124,-168,-232,61,-221 };

ALIGN(32) int16_t zetas_inv_avx[176] = { 221,-61,232,168,-124,-134,-310,306,58,42,145,105,243,-244,287,31,-62,-67,-155,153,116,84,290,210,-199,-122,-177,-305,-248,-268,21,-29,-20,-20,-125,-125,-50,-50,8,8,-128,-128,-159,-159,-320,-320,-77,-77,308,308,2,2,129,129,5,5,-80,-80,141,141,-200,-200,32,32,-64,-64,-64,-64,241,241,241,241,-160,-160,-160,-160,282,282,282,282,-25,-25,-25,-25,4,4,4,4,258,258,258,258,10,10,10,10,-40,-40,-40,-40,-40,-40,-40,-40,-250,-250,-250,-250,-250,-250,-250,-250,-100,-100,-100,-100,-100,-100,-100,-100,16,16,16,16,16,16,16,16,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-258,-258,-258,-258,-258,-258,-258,-258, -258,-258,-258,-258,-258,-258,-258,-258 };


void ntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i t[8];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i tzeta[2];



	//level 5
	//mul
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[0]);


	t[4] = _mm256_mullo_epi16(t[2], tzeta[0]);
	t[2] = _mm256_mulhi_epi16(t[2], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[0]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[0]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[2], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);


	//butterfly
	t[2] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[1], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[1] = _mm256_add_epi16(t[1], t[5]);

	//level 4
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[1]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[2]);

	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[4] = _mm256_add_epi16(t[0], t[4]);
	t[5] = _mm256_add_epi16(t[2], t[5]);


	//level 3
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[3]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[4]);

	t[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	t[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[4] = _mm256_add_epi16(t[0], t[4]);
	t[5] = _mm256_add_epi16(t[2], t[5]);


	//level 2   
	t[4] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[5], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xcc);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xcc);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xcc);


	t[0] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[2] = _mm256_permute4x64_epi64(t[2], 0xb1);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[5]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[6]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly        
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//level 1  
	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xaa);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xaa);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xaa);

	t[0] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[2] = _mm256_permutevar8x32_epi32(t[2], idx);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[7]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[8]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//level 0
	t[4] = _mm256_srli_epi32(t[0], 16);
	t[5] = _mm256_srli_epi32(t[2], 16);
	t[6] = _mm256_slli_epi32(t[1], 16);
	t[7] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[6], 0xaa);
	t[1] = _mm256_blend_epi16(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[7], 0xaa);
	t[3] = _mm256_blend_epi16(t[5], t[3], 0xaa);

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[9]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[10]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	// reduce to (-Q,Q)

	t[4] = _mm256_mulhi_epi16(t[1], c16x);
	t[5] = _mm256_mulhi_epi16(t[3], c16x);

	t[4] = _mm256_add_epi16(t[4], one16x);
	t[5] = _mm256_add_epi16(t[5], one16x);

	t[4] = _mm256_srai_epi16(t[4], 1);
	t[5] = _mm256_srai_epi16(t[5], 1);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);


	t[4] = _mm256_mulhi_epi16(t[0], c16x);
	t[5] = _mm256_mulhi_epi16(t[2], c16x);

	t[4] = _mm256_add_epi16(t[4], one16x);
	t[5] = _mm256_add_epi16(t[5], one16x);

	t[4] = _mm256_srai_epi16(t[4], 1);
	t[5] = _mm256_srai_epi16(t[5], 1);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[0] = _mm256_sub_epi16(t[0], t[4]);
	t[2] = _mm256_sub_epi16(t[2], t[5]);

	//store   
	t[4] = _mm256_unpacklo_epi16(t[0], t[1]);
	t[1] = _mm256_unpackhi_epi16(t[0], t[1]);
	t[5] = _mm256_unpacklo_epi16(t[2], t[3]);
	t[3] = _mm256_unpackhi_epi16(t[2], t[3]);

	p256a[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	p256a[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	p256a[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	p256a[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);
}
void invntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_inv_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
	// __m256i invn16x = _mm256_set1_epi16(1024); //invn*2^16 mod q
	__m256i invn16x = _mm256_set1_epi16(10);
	__m256i t[10];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i mask = _mm256_set1_epi32(0xFFFF);



	//level 0   
	//pack the data
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);


	t[4] = _mm256_permute2x128_si256(t[0], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[0], t[1], 0x31);
	t[5] = _mm256_permute2x128_si256(t[2], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[2], t[3], 0x31);


	t[6] = _mm256_and_si256(t[4], mask);
	t[7] = _mm256_and_si256(t[5], mask);
	t[8] = _mm256_and_si256(t[1], mask);
	t[9] = _mm256_and_si256(t[3], mask);
	t[0] = _mm256_packus_epi32(t[6], t[8]);
	t[2] = _mm256_packus_epi32(t[7], t[9]);


	t[4] = _mm256_srli_epi32(t[4], 16);
	t[1] = _mm256_srli_epi32(t[1], 16);
	t[5] = _mm256_srli_epi32(t[5], 16);
	t[3] = _mm256_srli_epi32(t[3], 16);

	t[4] = _mm256_packus_epi32(t[4], t[1]);
	t[5] = _mm256_packus_epi32(t[5], t[3]);

	//butterfly 

	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 1  
	t[4] = _mm256_srli_epi32(t[0], 16);
	t[5] = _mm256_srli_epi32(t[2], 16);
	t[6] = _mm256_slli_epi32(t[1], 16);
	t[7] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[6], 0xaa);
	t[1] = _mm256_blend_epi16(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[7], 0xaa);
	t[3] = _mm256_blend_epi16(t[5], t[3], 0xaa);

	//butterfly 

	t[4] = _mm256_sub_epi16(t[0], t[1]);
	t[5] = _mm256_sub_epi16(t[2], t[3]);

	t[0] = _mm256_add_epi16(t[0], t[1]);
	t[2] = _mm256_add_epi16(t[2], t[3]);

	//mul
	t[1] = _mm256_mulhi_epi16(t[4], p256zeta[2]);
	t[4] = _mm256_mullo_epi16(t[4], p256zeta[2]);
	t[3] = _mm256_mulhi_epi16(t[5], p256zeta[3]);
	t[5] = _mm256_mullo_epi16(t[5], p256zeta[3]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);


	//level 2

	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xaa);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xaa);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xaa);

	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);


	//butterfly 
	t[0] = _mm256_add_epi16(t[4], t[1]);
	t[2] = _mm256_add_epi16(t[5], t[3]);

	t[1] = _mm256_sub_epi16(t[4], t[1]);
	t[3] = _mm256_sub_epi16(t[5], t[3]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[4]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[4]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[5]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[5]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 3 
	t[4] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[2], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xcc);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xcc);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xcc);

	t[4] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[2], 0xb1);

	//butterfly 
	t[0] = _mm256_add_epi16(t[4], t[1]);
	t[2] = _mm256_add_epi16(t[5], t[3]);

	t[1] = _mm256_sub_epi16(t[4], t[1]);
	t[3] = _mm256_sub_epi16(t[5], t[3]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[6]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[6]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[7]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[7]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//reduce to (-PARAM_Q,PARAM_Q)
	t[4] = _mm256_mulhi_epi16(t[0], c16x);
	t[5] = _mm256_mulhi_epi16(t[2], c16x);

	t[4] = _mm256_add_epi16(t[4], one16x);
	t[5] = _mm256_add_epi16(t[5], one16x);

	t[4] = _mm256_srai_epi16(t[4], 1);
	t[5] = _mm256_srai_epi16(t[5], 1);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[0], t[4]);
	t[5] = _mm256_sub_epi16(t[2], t[5]);

	//level 4 
	t[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	t[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);

	//butterfly  

	t[4] = _mm256_sub_epi16(t[0], t[1]);
	t[5] = _mm256_sub_epi16(t[2], t[3]);

	t[0] = _mm256_add_epi16(t[0], t[1]);
	t[2] = _mm256_add_epi16(t[2], t[3]);

	//mul
	t[1] = _mm256_mulhi_epi16(t[4], p256zeta[8]);
	t[4] = _mm256_mullo_epi16(t[4], p256zeta[8]);
	t[3] = _mm256_mulhi_epi16(t[5], p256zeta[9]);
	t[5] = _mm256_mullo_epi16(t[5], p256zeta[9]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 5  
	//butterfly 

	t[4] = _mm256_add_epi16(t[0], t[2]);
	t[5] = _mm256_add_epi16(t[1], t[3]);

	t[6] = _mm256_sub_epi16(t[0], t[2]);
	t[7] = _mm256_sub_epi16(t[1], t[3]);

	//mul invn
	t[0] = _mm256_mulhi_epi16(t[4], invn16x);
	t[4] = _mm256_mullo_epi16(t[4], invn16x);
	t[1] = _mm256_mulhi_epi16(t[5], invn16x);
	t[5] = _mm256_mullo_epi16(t[5], invn16x);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	p256a[0] = _mm256_sub_epi16(t[0], t[4]);
	p256a[1] = _mm256_sub_epi16(t[1], t[5]);

	//mul
	t[4] = _mm256_mullo_epi16(t[6], p256zeta[10]);
	t[6] = _mm256_mulhi_epi16(t[6], p256zeta[10]);
	t[5] = _mm256_mullo_epi16(t[7], p256zeta[10]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[10]);

	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	p256a[2] = _mm256_sub_epi16(t[6], t[4]);
	p256a[3] = _mm256_sub_epi16(t[7], t[5]);
}

#elif  PARAM_Q == 1409

ALIGN(32) int16_t zetas_avx[176] = {
	-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,-544,
	-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,-284,
	-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,
	551,551,551,551,551,551,551,551,
	-341,-341,-341,-341,-341,-341,-341,-341,599,599,599,599,599,599,599,599,220,220,220,220,220,220,220,220,-22,-22,-22,-22,-81,-81,-81,-81,196,196,196,196,-175,-175,-175,-175,354,354,354,354,-618,-618,-618,-618,-592,-592,-592,-592,126,126,126,126,-32,-32,-374,-374,157,157,514,514,643,643,382,382,676,676,-201,-201,161,161,-496,-496,487,487,320,320,-285,-285,-601,-601,-407,-407,615,615,-337,-152,-328,-311,299,-116,-102,393,-462,-292,-111,552,389,-297,249,-172,-672,600,479,-478,-587,-432,106,6,563,-553,364,-325,-349,60,-93,234
};

ALIGN(32) int16_t zetas_inv_avx[176] = {
	-234,93,-60,349,325,-364,553,-563,-6,-106,432,587,478,-479,-600,672,172,-249,297,-389,-552,111,292,462,-393,102,116,-299,311,328,152,337,-615,-615,407,407,601,601,285,285,-320,-320,-487,-487,496,496,-161,-161,201,201,-676,-676,-382,-382,-643,-643,-514,-514,-157,-157,374,374,32,32,-126,-126,-126,-126,592,592,592,592,618,618,618,618,-354,-354,-354,-354,175,175,175,175,-196,-196,-196,-196,81,81,81,81,22,22,22,22,-220,-220,-220,-220,-220,-220,-220,-220,-599,-599,-599,-599,-599,-599,-599,-599,341,341,341,341,341,341,341,341,-551,-551,-551,-551,-551,-551,-551,-551,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,
	501,501,501,501,501,501,501,501,501,501,501,501,501,501,501,501
};


void ntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i mask = _mm256_set1_epi16(1 << 8);
	__m256i t[8];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i tzeta[2];



	//level 5
	//mul
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[0]);


	t[4] = _mm256_mullo_epi16(t[2], tzeta[0]);
	t[2] = _mm256_mulhi_epi16(t[2], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[0]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[0]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[2], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);


	//butterfly
	t[2] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[1], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[1] = _mm256_add_epi16(t[1], t[5]);

	//level 4
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[1]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[2]);

	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[4] = _mm256_add_epi16(t[0], t[4]);
	t[5] = _mm256_add_epi16(t[2], t[5]);


	//level 3
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[3]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[4]);

	t[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	t[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[4] = _mm256_add_epi16(t[0], t[4]);
	t[5] = _mm256_add_epi16(t[2], t[5]);


	//level 2
	t[4] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[5], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xcc);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xcc);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xcc);


	t[0] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[2] = _mm256_permute4x64_epi64(t[2], 0xb1);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[5]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[6]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//level 1
	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xaa);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xaa);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xaa);

	t[0] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[2] = _mm256_permutevar8x32_epi32(t[2], idx);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[7]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[8]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//level 0
	t[4] = _mm256_srli_epi32(t[0], 16);
	t[5] = _mm256_srli_epi32(t[2], 16);
	t[6] = _mm256_slli_epi32(t[1], 16);
	t[7] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[6], 0xaa);
	t[1] = _mm256_blend_epi16(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[7], 0xaa);
	t[3] = _mm256_blend_epi16(t[5], t[3], 0xaa);

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[9]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[10]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);


	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[1], t[4]);
	t[5] = _mm256_sub_epi16(t[3], t[5]);

	//butterfly
	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	// reduce to (-Q,Q)

	t[4] = _mm256_mulhi_epi16(t[1], c16x);
	t[5] = _mm256_mulhi_epi16(t[3], c16x);

	t[4] = _mm256_add_epi16(t[4], mask);
	t[5] = _mm256_add_epi16(t[5], mask);

	t[4] = _mm256_srai_epi16(t[4], 9);
	t[5] = _mm256_srai_epi16(t[5], 9);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);


	t[4] = _mm256_mulhi_epi16(t[0], c16x);
	t[5] = _mm256_mulhi_epi16(t[2], c16x);

	t[4] = _mm256_add_epi16(t[4], mask);
	t[5] = _mm256_add_epi16(t[5], mask);

	t[4] = _mm256_srai_epi16(t[4], 9);
	t[5] = _mm256_srai_epi16(t[5], 9);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[0] = _mm256_sub_epi16(t[0], t[4]);
	t[2] = _mm256_sub_epi16(t[2], t[5]);

	//store
	t[4] = _mm256_unpacklo_epi16(t[0], t[1]);
	t[1] = _mm256_unpackhi_epi16(t[0], t[1]);
	t[5] = _mm256_unpacklo_epi16(t[2], t[3]);
	t[3] = _mm256_unpackhi_epi16(t[2], t[3]);

	p256a[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	p256a[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	p256a[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	p256a[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);
}
void invntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_inv_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i mask0 = _mm256_set1_epi16(1 << 8);
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i invn16x = _mm256_set1_epi16(1012); //invn*2^16 mod q
	__m256i t[10];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i mask = _mm256_set1_epi32(0xFFFF);



	//level 0
	//pack the data
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);


	t[4] = _mm256_permute2x128_si256(t[0], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[0], t[1], 0x31);
	t[5] = _mm256_permute2x128_si256(t[2], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[2], t[3], 0x31);


	t[6] = _mm256_and_si256(t[4], mask);
	t[7] = _mm256_and_si256(t[5], mask);
	t[8] = _mm256_and_si256(t[1], mask);
	t[9] = _mm256_and_si256(t[3], mask);
	t[0] = _mm256_packus_epi32(t[6], t[8]);
	t[2] = _mm256_packus_epi32(t[7], t[9]);


	t[4] = _mm256_srli_epi32(t[4], 16);
	t[1] = _mm256_srli_epi32(t[1], 16);
	t[5] = _mm256_srli_epi32(t[5], 16);
	t[3] = _mm256_srli_epi32(t[3], 16);

	t[4] = _mm256_packus_epi32(t[4], t[1]);
	t[5] = _mm256_packus_epi32(t[5], t[3]);

	//butterfly

	t[1] = _mm256_sub_epi16(t[0], t[4]);
	t[3] = _mm256_sub_epi16(t[2], t[5]);

	t[0] = _mm256_add_epi16(t[0], t[4]);
	t[2] = _mm256_add_epi16(t[2], t[5]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[0]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[1]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 1
	t[4] = _mm256_srli_epi32(t[0], 16);
	t[5] = _mm256_srli_epi32(t[2], 16);
	t[6] = _mm256_slli_epi32(t[1], 16);
	t[7] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[6], 0xaa);
	t[1] = _mm256_blend_epi16(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[7], 0xaa);
	t[3] = _mm256_blend_epi16(t[5], t[3], 0xaa);

	//butterfly

	t[4] = _mm256_sub_epi16(t[0], t[1]);
	t[5] = _mm256_sub_epi16(t[2], t[3]);

	t[0] = _mm256_add_epi16(t[0], t[1]);
	t[2] = _mm256_add_epi16(t[2], t[3]);

	//mul
	t[1] = _mm256_mulhi_epi16(t[4], p256zeta[2]);
	t[4] = _mm256_mullo_epi16(t[4], p256zeta[2]);
	t[3] = _mm256_mulhi_epi16(t[5], p256zeta[3]);
	t[5] = _mm256_mullo_epi16(t[5], p256zeta[3]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);


	//level 2

	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xaa);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xaa);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xaa);

	t[4] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[5] = _mm256_permutevar8x32_epi32(t[2], idx);


	//butterfly
	t[0] = _mm256_add_epi16(t[4], t[1]);
	t[2] = _mm256_add_epi16(t[5], t[3]);

	t[1] = _mm256_sub_epi16(t[4], t[1]);
	t[3] = _mm256_sub_epi16(t[5], t[3]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[4]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[4]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[5]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[5]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 3
	t[4] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[2], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[4], 0xcc);
	t[1] = _mm256_blend_epi32(t[4], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[5], 0xcc);
	t[3] = _mm256_blend_epi32(t[5], t[3], 0xcc);

	t[4] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[5] = _mm256_permute4x64_epi64(t[2], 0xb1);

	//butterfly
	t[0] = _mm256_add_epi16(t[4], t[1]);
	t[2] = _mm256_add_epi16(t[5], t[3]);

	t[1] = _mm256_sub_epi16(t[4], t[1]);
	t[3] = _mm256_sub_epi16(t[5], t[3]);

	//mul
	t[4] = _mm256_mullo_epi16(t[1], p256zeta[6]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[6]);
	t[5] = _mm256_mullo_epi16(t[3], p256zeta[7]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[7]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//reduce to (-PARAM_Q,PARAM_Q)
	t[4] = _mm256_mulhi_epi16(t[0], c16x);
	t[5] = _mm256_mulhi_epi16(t[2], c16x);

	t[4] = _mm256_add_epi16(t[4], mask0);
	t[5] = _mm256_add_epi16(t[5], mask0);

	t[4] = _mm256_srai_epi16(t[4], 9);
	t[5] = _mm256_srai_epi16(t[5], 9);

	t[4] = _mm256_mullo_epi16(t[4], vq16x);
	t[5] = _mm256_mullo_epi16(t[5], vq16x);

	t[4] = _mm256_sub_epi16(t[0], t[4]);
	t[5] = _mm256_sub_epi16(t[2], t[5]);

	//level 4
	t[0] = _mm256_permute2x128_si256(t[4], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[4], t[1], 0x31);
	t[2] = _mm256_permute2x128_si256(t[5], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[5], t[3], 0x31);

	//butterfly

	t[4] = _mm256_sub_epi16(t[0], t[1]);
	t[5] = _mm256_sub_epi16(t[2], t[3]);

	t[0] = _mm256_add_epi16(t[0], t[1]);
	t[2] = _mm256_add_epi16(t[2], t[3]);

	//mul
	t[1] = _mm256_mulhi_epi16(t[4], p256zeta[8]);
	t[4] = _mm256_mullo_epi16(t[4], p256zeta[8]);
	t[3] = _mm256_mulhi_epi16(t[5], p256zeta[9]);
	t[5] = _mm256_mullo_epi16(t[5], p256zeta[9]);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[4]);
	t[3] = _mm256_sub_epi16(t[3], t[5]);

	//level 5
	//butterfly

	t[4] = _mm256_add_epi16(t[0], t[2]);
	t[5] = _mm256_add_epi16(t[1], t[3]);

	t[6] = _mm256_sub_epi16(t[0], t[2]);
	t[7] = _mm256_sub_epi16(t[1], t[3]);

	//mul invn
	t[0] = _mm256_mulhi_epi16(t[4], invn16x);
	t[4] = _mm256_mullo_epi16(t[4], invn16x);
	t[1] = _mm256_mulhi_epi16(t[5], invn16x);
	t[5] = _mm256_mullo_epi16(t[5], invn16x);

	//reduce
	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	p256a[0] = _mm256_sub_epi16(t[0], t[4]);
	p256a[1] = _mm256_sub_epi16(t[1], t[5]);

	//mul
	t[4] = _mm256_mullo_epi16(t[6], p256zeta[10]);
	t[6] = _mm256_mulhi_epi16(t[6], p256zeta[10]);
	t[5] = _mm256_mullo_epi16(t[7], p256zeta[10]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[10]);

	t[4] = _mm256_mullo_epi16(t[4], vqinv16x);
	t[5] = _mm256_mullo_epi16(t[5], vqinv16x);

	t[4] = _mm256_mulhi_epi16(t[4], vq16x);
	t[5] = _mm256_mulhi_epi16(t[5], vq16x);

	p256a[2] = _mm256_sub_epi16(t[6], t[4]);
	p256a[3] = _mm256_sub_epi16(t[7], t[5]);
}

#elif  PARAM_Q == 3329 || PARAM_Q == 769

#if PARAM_Q == 3329
ALIGN(32) int16_t zetas_avx[624] = {
	-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,
	-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,
	-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,
	1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,
	1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,
	287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,
	202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
	-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,
	622,622,622,622,622,622,622,622,622,622,622,622,622,622,622,622,
	1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,
	182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,
	962,962,962,962,962,962,962,962,962,962,962,962,962,962,962,962,
	-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,
	-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,
	1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,
	573,573,573,573,573,573,573,573,-1325,-1325,-1325,-1325,-1325,-1325,-1325,-1325,
	264,264,264,264,264,264,264,264,383,383,383,383,383,383,383,383,
	-829,-829,-829,-829,-829,-829,-829,-829,1458,1458,1458,1458,1458,1458,1458,1458,
	-1602,-1602,-1602,-1602,-1602,-1602,-1602,-1602,-130,-130,-130,-130,-130,-130,-130,-130,
	-681,-681,-681,-681,-681,-681,-681,-681,1017,1017,1017,1017,1017,1017,1017,1017,
	732,732,732,732,732,732,732,732,608,608,608,608,608,608,608,608,
	-1542,-1542,-1542,-1542,-1542,-1542,-1542,-1542,411,411,411,411,411,411,411,411,
	-205,-205,-205,-205,-205,-205,-205,-205,-1571,-1571,-1571,-1571,-1571,-1571,-1571,-1571,
	1223,1223,1223,1223,652,652,652,652,-552,-552,-552,-552,1015,1015,1015,1015,
	-1293,-1293,-1293,-1293,1491,1491,1491,1491,-282,-282,-282,-282,-1544,-1544,-1544,-1544,
	516,516,516,516,-8,-8,-8,-8,-320,-320,-320,-320,-666,-666,-666,-666,
	-1618,-1618,-1618,-1618,-1162,-1162,-1162,-1162,126,126,126,126,1469,1469,1469,1469,
	-853,-853,-853,-853,-90,-90,-90,-90,-271,-271,-271,-271,830,830,830,830,
	107,107,107,107,-1421,-1421,-1421,-1421,-247,-247,-247,-247,-951,-951,-951,-951,
	-398,-398,-398,-398,961,961,961,961,-1508,-1508,-1508,-1508,-725,-725,-725,-725,
	448,448,448,448,-1065,-1065,-1065,-1065,677,677,677,677,-1275,-1275,-1275,-1275,
	-1103,-1103,430,430,555,555,843,843,-1251,-1251,871,871,1550,1550,105,105,
	422,422,587,587,177,177,-235,-235,-291,-291,-460,-460,1574,1574,1653,1653,
	-246,-246,778,778,1159,1159,-147,-147,-777,-777,1483,1483,-602,-602,1119,1119,
	-1590,-1590,644,644,-872,-872,349,349,418,418,329,329,-156,-156,-75,-75,
	817,817,1097,1097,603,603,610,610,1322,1322,-1285,-1285,-1465,-1465,384,384,
	-1215,-1215,-136,-136,1218,1218,-1335,-1335,-874,-874,220,220,-1187,-1187,-1659,-1659,
	-1185,-1185,-1530,-1530,-1278,-1278,794,794,-1510,-1510,-854,-854,-870,-870,478,478,
	-108,-108,-308,-308,996,996,991,991,958,958,-1460,-1460,1522,1522,1628,1628
};

ALIGN(32) int16_t zetas_inv_avx[624] = {
	1628,1628,1522,1522,-1460,-1460,958,958,991,991,996,996,-308,-308,-108,-108,
	478,478,-870,-870,-854,-854,-1510,-1510,794,794,-1278,-1278,-1530,-1530,-1185,-1185,
	-1659,-1659,-1187,-1187,220,220,-874,-874,-1335,-1335,1218,1218,-136,-136,-1215,-1215,
	384,384,-1465,-1465,-1285,-1285,1322,1322,610,610,603,603,1097,1097,817,817,
	-75,-75,-156,-156,329,329,418,418,349,349,-872,-872,644,644,-1590,-1590,
	1119,1119,-602,-602,1483,1483,-777,-777,-147,-147,1159,1159,778,778,-246,-246,
	1653,1653,1574,1574,-460,-460,-291,-291,-235,-235,177,177,587,587,422,422,
	105,105,1550,1550,871,871,-1251,-1251,843,843,555,555,430,430,-1103,-1103,
	-1275,-1275,-1275,-1275,677,677,677,677,-1065,-1065,-1065,-1065,448,448,448,448,
	-725,-725,-725,-725,-1508,-1508,-1508,-1508,961,961,961,961,-398,-398,-398,-398,
	-951,-951,-951,-951,-247,-247,-247,-247,-1421,-1421,-1421,-1421,107,107,107,107,
	830,830,830,830,-271,-271,-271,-271,-90,-90,-90,-90,-853,-853,-853,-853,
	1469,1469,1469,1469,126,126,126,126,-1162,-1162,-1162,-1162,-1618,-1618,-1618,-1618,
	-666,-666,-666,-666,-320,-320,-320,-320,-8,-8,-8,-8,516,516,516,516,
	-1544,-1544,-1544,-1544,-282,-282,-282,-282,1491,1491,1491,1491,-1293,-1293,-1293,-1293,
	1015,1015,1015,1015,-552,-552,-552,-552,652,652,652,652,1223,1223,1223,1223,
	-1571,-1571,-1571,-1571,-1571,-1571,-1571,-1571,-205,-205,-205,-205,-205,-205,-205,-205,
	411,411,411,411,411,411,411,411,-1542,-1542,-1542,-1542,-1542,-1542,-1542,-1542,
	608,608,608,608,608,608,608,608,732,732,732,732,732,732,732,732,
	1017,1017,1017,1017,1017,1017,1017,1017,-681,-681,-681,-681,-681,-681,-681,-681,
	-130,-130,-130,-130,-130,-130,-130,-130,-1602,-1602,-1602,-1602,-1602,-1602,-1602,-1602,
	1458,1458,1458,1458,1458,1458,1458,1458,-829,-829,-829,-829,-829,-829,-829,-829,
	383,383,383,383,383,383,383,383,264,264,264,264,264,264,264,264,
	-1325,-1325,-1325,-1325,-1325,-1325,-1325,-1325,573,573,573,573,573,573,573,573,
	1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,1468,
	-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,
	-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,
	962,962,962,962,962,962,962,962,962,962,962,962,962,962,962,962,
	182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,
	1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,1577,
	622,622,622,622,622,622,622,622,622,622,622,622,622,622,622,622,
	-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,-171,
	202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
	287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,
	1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,
	1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,
	-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,
	-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,
	-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758
};

#elif PARAM_Q == 769
ALIGN(32) int16_t zetas_avx[624] = {
	-164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164,
	-81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
	361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361,
	-186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250,
	120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
	-129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129,
	-308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308,
	223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223,
	-16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16,
	-143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143,
	362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362,
	-337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337,
	-131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131,
	-75, -75, -75, -75, -75, -75, -75, -75, -36, -36, -36, -36, -36, -36, -36, -36,
	76, 76, 76, 76, 76, 76, 76, 76, 98, 98, 98, 98, 98, 98, 98, 98,
	203, 203, 203, 203, 203, 203, 203, 203, 282, 282, 282, 282, 282, 282, 282, 282,
	-339, -339, -339, -339, -339, -339, -339, -339, -255, -255, -255, -255, -255, -255, -255, -255,
	178, 178, 178, 178, 178, 178, 178, 178, 270, 270, 270, 270, 270, 270, 270, 270,
	199, 199, 199, 199, 199, 199, 199, 199, 34, 34, 34, 34, 34, 34, 34, 34,
	-369, -369, -369, -369, -369, -369, -369, -369, 192, 192, 192, 192, 192, 192, 192, 192,
	-149, -149, -149, -149, -149, -149, -149, -149, -10, -10, -10, -10, -10, -10, -10, -10,
	-80, -80, -80, -80, -346, -346, -346, -346, -124, -124, -124, -124, 2, 2, 2, 2,
	114, 114, 114, 114, 147, 147, 147, 147, -54, -54, -54, -54, -272, -272, -272, -272,
	-169, -169, -169, -169, 288, 288, 288, 288, 161, 161, 161, 161, -15, -15, -15, -15,
	-86, -86, -86, -86, 51, 51, 51, 51, -364, -364, -364, -364, -267, -267, -267, -267,
	170, 170, 170, 170, -226, -226, -226, -226, -121, -121, -121, -121, 188, 188, 188, 188,
	-50, -50, -50, -50, -24, -24, -24, -24, 307, 307, 307, 307, -191, -191, -191, -191,
	263, 263, 263, 263, 157, 157, 157, 157, -246, -246, -246, -246, 128, 128, 128, 128,
	375, 375, 375, 375, 180, 180, 180, 180, -380, -380, -380, -380, 279, 279, 279, 279,
	-341, -341, -379, -379, 202, 202, 220, 220, 236, 236, 21, 21, 212, 212, 71, 71,
	-134, -134, 151, 151, 23, 23, -112, -112, -232, -232, 227, 227, -52, -52, -148, -148,
	244, 244, -252, -252, -237, -237, -83, -83, -117, -117, -333, -333, -66, -66, -247, -247,
	-292, -292, 352, 352, -145, -145, 238, 238, -276, -276, -194, -194, -274, -274, -70, -70,
	209, 209, -115, -115, -99, -99, 14, 14, 29, 29, 260, 260, -378, -378, -366, -366,
	355, 355, -291, -291, 358, 358, -105, -105, 167, 167, 357, 357, -241, -241, -331, -331,
	-348, -348, -44, -44, -78, -78, -222, -222, -350, -350, -168, -168, -158, -158, 201, 201,
	303, 303, 330, 330, -184, -184, 127, 127, 318, 318, -278, -278, -353, -353, -354, -354
};

ALIGN(32) int16_t zetas_inv_avx[624] = {
	-354, -354, -353, -353, -278, -278, 318, 318, 127, 127, -184, -184, 330, 330, 303, 303,
	201, 201, -158, -158, -168, -168, -350, -350, -222, -222, -78, -78, -44, -44, -348, -348,
	-331, -331, -241, -241, 357, 357, 167, 167, -105, -105, 358, 358, -291, -291, 355, 355,
	-366, -366, -378, -378, 260, 260, 29, 29, 14, 14, -99, -99, -115, -115, 209, 209,
	-70, -70, -274, -274, -194, -194, -276, -276, 238, 238, -145, -145, 352, 352, -292, -292,
	-247, -247, -66, -66, -333, -333, -117, -117, -83, -83, -237, -237, -252, -252, 244, 244,
	-148, -148, -52, -52, 227, 227, -232, -232, -112, -112, 23, 23, 151, 151, -134, -134,
	71, 71, 212, 212, 21, 21, 236, 236, 220, 220, 202, 202, -379, -379, -341, -341,
	279, 279, 279, 279, -380, -380, -380, -380, 180, 180, 180, 180, 375, 375, 375, 375,
	128, 128, 128, 128, -246, -246, -246, -246, 157, 157, 157, 157, 263, 263, 263, 263,
	-191, -191, -191, -191, 307, 307, 307, 307, -24, -24, -24, -24, -50, -50, -50, -50,
	188, 188, 188, 188, -121, -121, -121, -121, -226, -226, -226, -226, 170, 170, 170, 170,
	-267, -267, -267, -267, -364, -364, -364, -364, 51, 51, 51, 51, -86, -86, -86, -86,
	-15, -15, -15, -15, 161, 161, 161, 161, 288, 288, 288, 288, -169, -169, -169, -169,
	-272, -272, -272, -272, -54, -54, -54, -54, 147, 147, 147, 147, 114, 114, 114, 114,
	2, 2, 2, 2, -124, -124, -124, -124, -346, -346, -346, -346, -80, -80, -80, -80,
	-10, -10, -10, -10, -10, -10, -10, -10, -149, -149, -149, -149, -149, -149, -149, -149,
	192, 192, 192, 192, 192, 192, 192, 192, -369, -369, -369, -369, -369, -369, -369, -369,
	34, 34, 34, 34, 34, 34, 34, 34, 199, 199, 199, 199, 199, 199, 199, 199,
	270, 270, 270, 270, 270, 270, 270, 270, 178, 178, 178, 178, 178, 178, 178, 178,
	-255, -255, -255, -255, -255, -255, -255, -255, -339, -339, -339, -339, -339, -339, -339, -339,
	282, 282, 282, 282, 282, 282, 282, 282, 203, 203, 203, 203, 203, 203, 203, 203,
	98, 98, 98, 98, 98, 98, 98, 98, 76, 76, 76, 76, 76, 76, 76, 76,
	-36, -36, -36, -36, -36, -36, -36, -36, -75, -75, -75, -75, -75, -75, -75, -75,
	-131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131, -131,
	-337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337, -337,
	362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362, 362,
	-143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143, -143,
	-16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16, -16,
	223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223, 223,
	-308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308, -308,
	-129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129, -129,
	120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
	250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	-186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186, -186,
	361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361, 361,
	-81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
	-164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164, -164
};

#endif

#define MONT_REDUCE(r, a, b) \
	t[9] = _mm256_mulhi_epi16((a), (b)); \
	t[8] = _mm256_mullo_epi16((a), (b)); \
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x); \
	t[8] = _mm256_mulhi_epi16(t[8], vq16x); \
	(r) = _mm256_sub_epi16(t[9], t[8])

#if PARAM_Q == 3329
#define BARRETT_REDUCE(r, a) \
	t[11] = _mm256_mulhi_epi16((a), c16x); \
	t[11] = _mm256_add_epi16(t[11], one16x); \
	t[11] = _mm256_srai_epi16(t[11], 10); \
	t[11] = _mm256_mullo_epi16(t[11], vq16x); \
	(r) = _mm256_sub_epi16((a), t[11])
#elif PARAM_Q == 769
#define BARRETT_REDUCE(r, a) \
	t[11] = _mm256_mulhi_epi16((a), c16x); \
	t[11] = _mm256_add_epi16(t[11], one16x); \
	t[11] = _mm256_srai_epi16(t[11], 8); \
	t[11] = _mm256_mullo_epi16(t[11], vq16x); \
	(r) = _mm256_sub_epi16((a), t[11])
#endif

void ntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
#if PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
#endif
	__m256i t[12];

	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);
	t[4] = _mm256_load_si256((__m256i*) & p256a[4]);
	t[5] = _mm256_load_si256((__m256i*) & p256a[5]);
	t[6] = _mm256_load_si256((__m256i*) & p256a[6]);
	t[7] = _mm256_load_si256((__m256i*) & p256a[7]);

	//level 6
	MONT_REDUCE(t[9], t[4], p256zeta[0]);
	t[4] = _mm256_sub_epi16(t[0], t[9]);
	t[0] = _mm256_add_epi16(t[0], t[9]);

	MONT_REDUCE(t[9], t[5], p256zeta[0]);
	t[5] = _mm256_sub_epi16(t[1], t[9]);
	t[1] = _mm256_add_epi16(t[1], t[9]);

	MONT_REDUCE(t[9], t[6], p256zeta[0]);
	t[6] = _mm256_sub_epi16(t[2], t[9]);
	t[2] = _mm256_add_epi16(t[2], t[9]);

	MONT_REDUCE(t[9], t[7], p256zeta[0]);
	t[7] = _mm256_sub_epi16(t[3], t[9]);
	t[3] = _mm256_add_epi16(t[3], t[9]);

	//level 5
	MONT_REDUCE(t[9], t[2], p256zeta[1]);
	t[2] = _mm256_sub_epi16(t[0], t[9]);
	t[0] = _mm256_add_epi16(t[0], t[9]);

	MONT_REDUCE(t[9], t[3], p256zeta[1]);
	t[3] = _mm256_sub_epi16(t[1], t[9]);
	t[1] = _mm256_add_epi16(t[1], t[9]);

	MONT_REDUCE(t[9], t[6], p256zeta[2]);
	t[6] = _mm256_sub_epi16(t[4], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[9]);

	MONT_REDUCE(t[9], t[7], p256zeta[2]);
	t[7] = _mm256_sub_epi16(t[5], t[9]);
	t[5] = _mm256_add_epi16(t[5], t[9]);

	//level 4
	MONT_REDUCE(t[9], t[1], p256zeta[3]);
	t[1] = _mm256_sub_epi16(t[0], t[9]);
	t[0] = _mm256_add_epi16(t[0], t[9]);

	MONT_REDUCE(t[9], t[3], p256zeta[4]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[2] = _mm256_add_epi16(t[2], t[9]);

	MONT_REDUCE(t[9], t[5], p256zeta[5]);
	t[5] = _mm256_sub_epi16(t[4], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[9]);

	MONT_REDUCE(t[9], t[7], p256zeta[6]);
	t[7] = _mm256_sub_epi16(t[6], t[9]);
	t[6] = _mm256_add_epi16(t[6], t[9]);

	//level 3
	t[8] = _mm256_permute2x128_si256(t[0], t[0], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[7]);
	t[10] = _mm256_add_epi16(t[0], t[9]);
	t[11] = _mm256_sub_epi16(t[0], t[9]);
	t[0] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[1], t[1], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[8]);
	t[10] = _mm256_add_epi16(t[1], t[9]);
	t[11] = _mm256_sub_epi16(t[1], t[9]);
	t[1] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[2], t[2], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[9]);
	t[10] = _mm256_add_epi16(t[2], t[9]);
	t[11] = _mm256_sub_epi16(t[2], t[9]);
	t[2] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[3], t[3], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[10]);
	t[10] = _mm256_add_epi16(t[3], t[9]);
	t[11] = _mm256_sub_epi16(t[3], t[9]);
	t[3] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[4], t[4], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[11]);
	t[10] = _mm256_add_epi16(t[4], t[9]);
	t[11] = _mm256_sub_epi16(t[4], t[9]);
	t[4] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[5], t[5], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[12]);
	t[10] = _mm256_add_epi16(t[5], t[9]);
	t[11] = _mm256_sub_epi16(t[5], t[9]);
	t[5] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[6], t[6], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[13]);
	t[10] = _mm256_add_epi16(t[6], t[9]);
	t[11] = _mm256_sub_epi16(t[6], t[9]);
	t[6] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	t[8] = _mm256_permute2x128_si256(t[7], t[7], 0x01);
	MONT_REDUCE(t[9], t[8], p256zeta[14]);
	t[10] = _mm256_add_epi16(t[7], t[9]);
	t[11] = _mm256_sub_epi16(t[7], t[9]);
	t[7] = _mm256_permute2x128_si256(t[10], t[11], 0x20);

	//level 2
	t[8] = _mm256_permute4x64_epi64(t[0], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[15]);
	t[10] = _mm256_add_epi16(t[0], t[9]);
	t[11] = _mm256_sub_epi16(t[0], t[9]);
	t[0] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[1], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[16]);
	t[10] = _mm256_add_epi16(t[1], t[9]);
	t[11] = _mm256_sub_epi16(t[1], t[9]);
	t[1] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[2], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[17]);
	t[10] = _mm256_add_epi16(t[2], t[9]);
	t[11] = _mm256_sub_epi16(t[2], t[9]);
	t[2] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[3], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[18]);
	t[10] = _mm256_add_epi16(t[3], t[9]);
	t[11] = _mm256_sub_epi16(t[3], t[9]);
	t[3] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[4], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[19]);
	t[10] = _mm256_add_epi16(t[4], t[9]);
	t[11] = _mm256_sub_epi16(t[4], t[9]);
	t[4] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[5], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[20]);
	t[10] = _mm256_add_epi16(t[5], t[9]);
	t[11] = _mm256_sub_epi16(t[5], t[9]);
	t[5] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[6], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[21]);
	t[10] = _mm256_add_epi16(t[6], t[9]);
	t[11] = _mm256_sub_epi16(t[6], t[9]);
	t[6] = _mm256_unpacklo_epi64(t[10], t[11]);

	t[8] = _mm256_permute4x64_epi64(t[7], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[22]);
	t[10] = _mm256_add_epi16(t[7], t[9]);
	t[11] = _mm256_sub_epi16(t[7], t[9]);
	t[7] = _mm256_unpacklo_epi64(t[10], t[11]);

	//level 1
	t[8] = _mm256_shuffle_epi32(t[0], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[23]);
	t[10] = _mm256_add_epi16(t[0], t[9]);
	t[11] = _mm256_sub_epi16(t[0], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[0] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[1], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[24]);
	t[10] = _mm256_add_epi16(t[1], t[9]);
	t[11] = _mm256_sub_epi16(t[1], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[1] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[2], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[25]);
	t[10] = _mm256_add_epi16(t[2], t[9]);
	t[11] = _mm256_sub_epi16(t[2], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[2] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[3], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[26]);
	t[10] = _mm256_add_epi16(t[3], t[9]);
	t[11] = _mm256_sub_epi16(t[3], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[3] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[4], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[27]);
	t[10] = _mm256_add_epi16(t[4], t[9]);
	t[11] = _mm256_sub_epi16(t[4], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[4] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[5], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[28]);
	t[10] = _mm256_add_epi16(t[5], t[9]);
	t[11] = _mm256_sub_epi16(t[5], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[5] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[6], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[29]);
	t[10] = _mm256_add_epi16(t[6], t[9]);
	t[11] = _mm256_sub_epi16(t[6], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[6] = _mm256_unpacklo_epi64(t[8], t[10]);

	t[8] = _mm256_shuffle_epi32(t[7], 0xb1);
	MONT_REDUCE(t[9], t[8], p256zeta[30]);
	t[10] = _mm256_add_epi16(t[7], t[9]);
	t[11] = _mm256_sub_epi16(t[7], t[9]);
	t[8] = _mm256_unpacklo_epi32(t[10], t[11]);
	t[10] = _mm256_unpackhi_epi32(t[10], t[11]);
	t[7] = _mm256_unpacklo_epi64(t[8], t[10]);

	//level 0
	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[0], 16), _mm256_slli_epi32(t[0], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[31]);
	t[10] = _mm256_add_epi16(t[0], t[9]);
	t[11] = _mm256_sub_epi16(t[0], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[0] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[1], 16), _mm256_slli_epi32(t[1], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[32]);
	t[10] = _mm256_add_epi16(t[1], t[9]);
	t[11] = _mm256_sub_epi16(t[1], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[1] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[2], 16), _mm256_slli_epi32(t[2], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[33]);
	t[10] = _mm256_add_epi16(t[2], t[9]);
	t[11] = _mm256_sub_epi16(t[2], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[2] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[3], 16), _mm256_slli_epi32(t[3], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[34]);
	t[10] = _mm256_add_epi16(t[3], t[9]);
	t[11] = _mm256_sub_epi16(t[3], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[3] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[4], 16), _mm256_slli_epi32(t[4], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[35]);
	t[10] = _mm256_add_epi16(t[4], t[9]);
	t[11] = _mm256_sub_epi16(t[4], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[4] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[5], 16), _mm256_slli_epi32(t[5], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[36]);
	t[10] = _mm256_add_epi16(t[5], t[9]);
	t[11] = _mm256_sub_epi16(t[5], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[5] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[6], 16), _mm256_slli_epi32(t[6], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[37]);
	t[10] = _mm256_add_epi16(t[6], t[9]);
	t[11] = _mm256_sub_epi16(t[6], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[6] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[7], 16), _mm256_slli_epi32(t[7], 16));
	MONT_REDUCE(t[9], t[8], p256zeta[38]);
	t[10] = _mm256_add_epi16(t[7], t[9]);
	t[11] = _mm256_sub_epi16(t[7], t[9]);
	t[11] = _mm256_slli_epi32(t[11], 16);
	t[7] = _mm256_blend_epi16(t[10], t[11], 0xaa);

	// reduce to (-Q,Q)
	BARRETT_REDUCE(t[9], t[0]);
	_mm256_store_si256(&p256a[0], t[9]);
	BARRETT_REDUCE(t[9], t[1]);
	_mm256_store_si256(&p256a[1], t[9]);
	BARRETT_REDUCE(t[9], t[2]);
	_mm256_store_si256(&p256a[2], t[9]);
	BARRETT_REDUCE(t[9], t[3]);
	_mm256_store_si256(&p256a[3], t[9]);
	BARRETT_REDUCE(t[9], t[4]);
	_mm256_store_si256(&p256a[4], t[9]);
	BARRETT_REDUCE(t[9], t[5]);
	_mm256_store_si256(&p256a[5], t[9]);
	BARRETT_REDUCE(t[9], t[6]);
	_mm256_store_si256(&p256a[6], t[9]);
	BARRETT_REDUCE(t[9], t[7]);
	_mm256_store_si256(&p256a[7], t[9]);

}

void invntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_inv_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
#if PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
	__m256i invn16x = _mm256_set1_epi16(1441);
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
	__m256i invn16x = _mm256_set1_epi16(655);
#endif
	__m256i t[12];

	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);
	t[4] = _mm256_load_si256((__m256i*) & p256a[4]);
	t[5] = _mm256_load_si256((__m256i*) & p256a[5]);
	t[6] = _mm256_load_si256((__m256i*) & p256a[6]);
	t[7] = _mm256_load_si256((__m256i*) & p256a[7]);

	//level 0
	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[0], 16), _mm256_slli_epi32(t[0], 16));
	t[9] = _mm256_add_epi16(t[0], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[0]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[0]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[0] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[1], 16), _mm256_slli_epi32(t[1], 16));
	t[9] = _mm256_add_epi16(t[1], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[1]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[1]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[1] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[2], 16), _mm256_slli_epi32(t[2], 16));
	t[9] = _mm256_add_epi16(t[2], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[2]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[2]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[2] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[3], 16), _mm256_slli_epi32(t[3], 16));
	t[9] = _mm256_add_epi16(t[3], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[3]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[3]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[3] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[4], 16), _mm256_slli_epi32(t[4], 16));
	t[9] = _mm256_add_epi16(t[4], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[4]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[4]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[4] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[5], 16), _mm256_slli_epi32(t[5], 16));
	t[9] = _mm256_add_epi16(t[5], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[5]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[5]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[5] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[6], 16), _mm256_slli_epi32(t[6], 16));
	t[9] = _mm256_add_epi16(t[6], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[6]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[6]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[6] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	t[8] = _mm256_or_si256(_mm256_srli_epi32(t[7], 16), _mm256_slli_epi32(t[7], 16));
	t[9] = _mm256_add_epi16(t[7], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[7]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[7]);
	t[10] = _mm256_slli_epi32(t[10], 16);
	t[7] = _mm256_blend_epi16(t[11], t[10], 0xaa);

	//level 1
	t[8] = _mm256_shuffle_epi32(t[0], 0xb1);
	t[9] = _mm256_add_epi16(t[0], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[0]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[8]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[0] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[1], 0xb1);
	t[9] = _mm256_add_epi16(t[1], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[1]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[9]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[1] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[2], 0xb1);
	t[9] = _mm256_add_epi16(t[2], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[2]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[10]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[2] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[3], 0xb1);
	t[9] = _mm256_add_epi16(t[3], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[3]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[11]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[3] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[4], 0xb1);
	t[9] = _mm256_add_epi16(t[4], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[4]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[12]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[4] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[5], 0xb1);
	t[9] = _mm256_add_epi16(t[5], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[5]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[13]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[5] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[6], 0xb1);
	t[9] = _mm256_add_epi16(t[6], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[6]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[14]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[6] = _mm256_unpacklo_epi64(t[8], t[9]);

	t[8] = _mm256_shuffle_epi32(t[7], 0xb1);
	t[9] = _mm256_add_epi16(t[7], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[7]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[15]);
	t[8] = _mm256_unpacklo_epi32(t[11], t[10]);
	t[9] = _mm256_unpackhi_epi32(t[11], t[10]);
	t[7] = _mm256_unpacklo_epi64(t[8], t[9]);

	//level 2
	t[8] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[9] = _mm256_add_epi16(t[0], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[0]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[16]);
	t[0] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[1], 0xb1);
	t[9] = _mm256_add_epi16(t[1], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[1]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[17]);
	t[1] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[2], 0xb1);
	t[9] = _mm256_add_epi16(t[2], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[2]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[18]);
	t[2] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[3], 0xb1);
	t[9] = _mm256_add_epi16(t[3], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[3]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[19]);
	t[3] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[9] = _mm256_add_epi16(t[4], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[4]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[20]);
	t[4] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[5], 0xb1);
	t[9] = _mm256_add_epi16(t[5], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[5]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[21]);
	t[5] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[6], 0xb1);
	t[9] = _mm256_add_epi16(t[6], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[6]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[22]);
	t[6] = _mm256_unpacklo_epi64(t[11], t[10]);

	t[8] = _mm256_permute4x64_epi64(t[7], 0xb1);
	t[9] = _mm256_add_epi16(t[7], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[7]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[23]);
	t[7] = _mm256_unpacklo_epi64(t[11], t[10]);

	//level 3
	t[8] = _mm256_permute2x128_si256(t[0], t[0], 0x01);
	t[9] = _mm256_add_epi16(t[0], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[0]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[24]);
	t[0] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[1], t[1], 0x01);
	t[9] = _mm256_add_epi16(t[1], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[1]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[25]);
	t[1] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[2], t[2], 0x01);
	t[9] = _mm256_add_epi16(t[2], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[2]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[26]);
	t[2] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[3], t[3], 0x01);
	t[9] = _mm256_add_epi16(t[3], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[3]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[27]);
	t[3] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[4], t[4], 0x01);
	t[9] = _mm256_add_epi16(t[4], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[4]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[28]);
	t[4] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[5], t[5], 0x01);
	t[9] = _mm256_add_epi16(t[5], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[5]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[29]);
	t[5] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[6], t[6], 0x01);
	t[9] = _mm256_add_epi16(t[6], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[6]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[30]);
	t[6] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	t[8] = _mm256_permute2x128_si256(t[7], t[7], 0x01);
	t[9] = _mm256_add_epi16(t[7], t[8]);
	t[10] = _mm256_sub_epi16(t[8], t[7]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[31]);
	t[7] = _mm256_permute2x128_si256(t[11], t[10], 0x20);

	//level 4
	t[9] = _mm256_add_epi16(t[0], t[1]);
	t[10] = _mm256_sub_epi16(t[1], t[0]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[32]);
	t[0] = t[11];
	t[1] = t[10];

	t[9] = _mm256_add_epi16(t[2], t[3]);
	t[10] = _mm256_sub_epi16(t[3], t[2]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[33]);
	t[2] = t[11];
	t[3] = t[10];

	t[9] = _mm256_add_epi16(t[4], t[5]);
	t[10] = _mm256_sub_epi16(t[5], t[4]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[34]);
	t[4] = t[11];
	t[5] = t[10];

	t[9] = _mm256_add_epi16(t[6], t[7]);
	t[10] = _mm256_sub_epi16(t[7], t[6]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[35]);
	t[6] = t[11];
	t[7] = t[10];

	//level 5
	t[9] = _mm256_add_epi16(t[0], t[2]);
	t[10] = _mm256_sub_epi16(t[2], t[0]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[36]);
	t[0] = t[11];
	t[2] = t[10];

	t[9] = _mm256_add_epi16(t[1], t[3]);
	t[10] = _mm256_sub_epi16(t[3], t[1]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[36]);
	t[1] = t[11];
	t[3] = t[10];

	t[9] = _mm256_add_epi16(t[4], t[6]);
	t[10] = _mm256_sub_epi16(t[6], t[4]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[37]);
	t[4] = t[11];
	t[6] = t[10];

	t[9] = _mm256_add_epi16(t[5], t[7]);
	t[10] = _mm256_sub_epi16(t[7], t[5]);
	t[11] = t[9];
	MONT_REDUCE(t[10], t[10], p256zeta[37]);
	t[5] = t[11];
	t[7] = t[10];

	//level 6
	t[9] = _mm256_add_epi16(t[0], t[4]);
	t[10] = _mm256_sub_epi16(t[4], t[0]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[38]);
	t[0] = t[11];
	t[4] = t[10];

	t[9] = _mm256_add_epi16(t[1], t[5]);
	t[10] = _mm256_sub_epi16(t[5], t[1]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[38]);
	t[1] = t[11];
	t[5] = t[10];

	t[9] = _mm256_add_epi16(t[2], t[6]);
	t[10] = _mm256_sub_epi16(t[6], t[2]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[38]);
	t[2] = t[11];
	t[6] = t[10];

	t[9] = _mm256_add_epi16(t[3], t[7]);
	t[10] = _mm256_sub_epi16(t[7], t[3]);
	BARRETT_REDUCE(t[11], t[9]);
	MONT_REDUCE(t[10], t[10], p256zeta[38]);
	t[3] = t[11];
	t[7] = t[10];

	//mul invn
	MONT_REDUCE(t[9], t[0], invn16x);
	_mm256_store_si256((__m256i*) & p256a[0], t[9]);

	MONT_REDUCE(t[9], t[1], invn16x);
	_mm256_store_si256((__m256i*) & p256a[1], t[9]);

	MONT_REDUCE(t[9], t[2], invn16x);
	_mm256_store_si256((__m256i*) & p256a[2], t[9]);

	MONT_REDUCE(t[9], t[3], invn16x);
	_mm256_store_si256((__m256i*) & p256a[3], t[9]);

	MONT_REDUCE(t[9], t[4], invn16x);
	_mm256_store_si256((__m256i*) & p256a[4], t[9]);

	MONT_REDUCE(t[9], t[5], invn16x);
	_mm256_store_si256((__m256i*) & p256a[5], t[9]);

	MONT_REDUCE(t[9], t[6], invn16x);
	_mm256_store_si256((__m256i*) & p256a[6], t[9]);

	MONT_REDUCE(t[9], t[7], invn16x);
	_mm256_store_si256((__m256i*) & p256a[7], t[9]);

}

#elif  PARAM_Q == 769

const static int16_t zetas[128] = {
	171, -164, -81, 361, -186, 3, 250, 120,
	-129, -308, 223, -16, -143, 362, -337, -131,
	-75, -36, 76, 98, 203, 282, -339, -255,
	178, 270, 199, 34, -369, 192, -149, -10,
	-80, -346, -124, 2, 114, 147, -54, -272,
	-169, 288, 161, -15, -86, 51, -364, -267,
	170, -226, -121, 188, -50, -24, 307, -191,
	263, 157, -246, 128, 375, 180, -380, 279,
	-341, -379, 202, 220, 236, 21, 212, 71,
	-134, 151, 23, -112, -232, 227, -52, -148,
	244, -252, -237, -83, -117, -333, -66, -247,
	-292, 352, -145, 238, -276, -194, -274, -70,
	209, -115, -99, 14, 29, 260, -378, -366,
	355, -291, 358, -105, 167, 357, -241, -331,
	-348, -44, -78, -222, -350, -168, -158, 201,
	303, 330, -184, 127, 318, -278, -353, -354
};

static inline int16_t montgomery_reduce(int32_t a)
{
	int16_t t;
	t = (int16_t)a * QINV;
	t = (a - (int32_t)t * PARAM_Q) >> 16;
	return t;
}

static inline int16_t fqmul(int16_t a, int16_t b) {
	return montgomery_reduce((int32_t)a * b);
}


static inline int16_t barrett_reduce(int16_t a)
{
	int16_t t;
	const int16_t v = ((1 << 24) + PARAM_Q / 2) / PARAM_Q;

	t = ((int32_t)v * a + (1 << 23)) >> 24;

	return (int16_t)(a - (int32_t)t * PARAM_Q);
}

void ntt(int16_t *a) {
	unsigned int len, start, j, k;
	int16_t t, zeta;
	k = 1;
	for (len = 64; len > 0; len >>= 1) {
		for (start = 0; start < NTT_DIM; start = j + len) {
			zeta = zetas[k++];
			for (j = start; j < start + len; j++) {
				t = fqmul(zeta, a[j + len]);
				a[j + len] = a[j] - t;
				a[j] = a[j] + t;
			}
		}
	}
	for (int i = 0; i < NTT_DIM; i++)
		a[i] = barrett_reduce(a[i]);
}

void invntt(int16_t *a) {
	unsigned int start, len, j, k;
	int16_t t, zeta;
	const int16_t f = 655; // mont^2/128

	k = NTT_DIM;
	for (len = 1; len < NTT_DIM; len <<= 1) {
		for (start = 0; start < NTT_DIM; start = j + len) {
			zeta = zetas[--k];
			for (j = start; j < start + len; j++) {
				t = a[j];
				a[j] = barrett_reduce(t + a[j + len]);
				a[j + len] = a[j + len] - t;
				a[j + len] = fqmul(zeta, a[j + len]);
			}
		}
	}
	for (j = 0; j < NTT_DIM; j++) {
		a[j] = fqmul(a[j], f);
	}
}


#undef MONT_REDUCE
#undef BARRETT_REDUCE


#endif
