#include "inttypes.h"
#include "avx2_neon.h"
#include "ntt.h"
#include "params.h"

#if NTT_DIM == 128

const int16_t zetas_avx[368] = { -758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-758,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-359,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,-1517,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1493,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,1422,287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,287,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,-171,-171,-171,-171,-171,-171,-171,-171,622,622,622,622,622,622,622,622,1577,1577,1577,1577,1577,1577,1577,1577,182,182,182,182,182,182,182,182,962,962,962,962,962,962,962,962,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1202,-1474,-1474,-1474,-1474,-1474,-1474,-1474,-1474,1468,1468,1468,1468,1468,1468,1468,1468,573,573,573,573,-1325,-1325,-1325,-1325,264,264,264,264,383,383,383,383,-829,-829,-829,-829,1458,1458,1458,1458,-1602,-1602,-1602,-1602,-130,-130,-130,-130,-681,-681,-681,-681,1017,1017,1017,1017,732,732,732,732,608,608,608,608,-1542,-1542,-1542,-1542,411,411,411,411,-205,-205,-205,-205,-1571,-1571,-1571,-1571,1223,1223,652,652,-552,-552,1015,1015,-1293,-1293,1491,1491,-282,-282,-1544,-1544,516,516,-8,-8,-320,-320,-666,-666,-1618,-1618,-1162,-1162,126,126,1469,1469,-853,-853,-90,-90,-271,-271,830,830,107,107,-1421,-1421,-247,-247,-951,-951,-398,-398,961,961,-1508,-1508,-725,-725,448,448,-1065,-1065,677,677,-1275,-1275,-1103,430,555,843,-1251,871,1550,105,422,587,177,-235,-291,-460,1574,1653,-246,778,1159,-147,-777,1483,-602,1119,-1590,644,-872,349,418,329,-156,-75,817,1097,603,610,1322,-1285,-1465,384,-1215,-136,1218,-1335,-874,220,-1187,-1659,-1185,-1530,-1278,794,-1510,-854,-870,478,-108,-308,996,991,958,-1460,1522,1628 };

const int16_t zetas_inv_avx[368] = { -1628,-1522,1460,-958,-991,-996,308,108,-478,870,854,1510,-794,1278,1530,1185,1659,1187,-220,874,1335,-1218,136,1215,-384,1465,1285,-1322,-610,-603,-1097,-817,75,156,-329,-418,-349,872,-644,1590,-1119,602,-1483,777,147,-1159,-778,246,-1653,-1574,460,291,235,-177,-587,-422,-105,-1550,-871,1251,-843,-555,-430,1103,1275,1275,-677,-677,1065,1065,-448,-448,725,725,1508,1508,-961,-961,398,398,951,951,247,247,1421,1421,-107,-107,-830,-830,271,271,90,90,853,853,-1469,-1469,-126,-126,1162,1162,1618,1618,666,666,320,320,8,8,-516,-516,1544,1544,282,282,-1491,-1491,1293,1293,-1015,-1015,552,552,-652,-652,-1223,-1223,1571,1571,1571,1571,205,205,205,205,-411,-411,-411,-411,1542,1542,1542,1542,-608,-608,-608,-608,-732,-732,-732,-732,-1017,-1017,-1017,-1017,681,681,681,681,130,130,130,130,1602,1602,1602,1602,-1458,-1458,-1458,-1458,829,829,829,829,-383,-383,-383,-383,-264,-264,-264,-264,1325,1325,1325,1325,-573,-573,-573,-573,-1468,-1468,-1468,-1468,-1468,-1468,-1468,-1468,1474,1474,1474,1474,1474,1474,1474,1474,1202,1202,1202,1202,1202,1202,1202,1202,-962,-962,-962,-962,-962,-962,-962,-962,-182,-182,-182,-182,-182,-182,-182,-182,-1577,-1577,-1577,-1577,-1577,-1577,-1577,-1577,-622,-622,-622,-622,-622,-622,-622,-622,171,171,171,171,171,171,171,171,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-202,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-287,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1422,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,-1493,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,1517,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266 };

#elif NTT_DIM == 64

int16_t zetas_avx[176] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,-16,-16,-16,-16,-16,-16,-16,-16,100,100,100,100,100,100,100,100,250,250,250,250,250,250,250,250,40,40,40,40,40,40,40,40,-10,-10,-10,-10,-258,-258,-258,-258,-4,-4,-4,-4,25,25,25,25,-282,-282,-282,-282,160,160,160,160,-241,-241,-241,-241,64,64,64,64,-32,-32,200,200,-141,-141,80,80,-5,-5,-129,-129,-2,-2,-308,-308,77,77,320,320,159,159,128,128,-8,-8,50,50,125,125,20,20,29,-21,268,248,305,177,122,199,-210,-290,-84,-116,-153,155,67,62,-31,-287,244,-243,-105,-145,-42,-58,-306,310,134,124,-168,-232,61,-221 };

int16_t zetas_inv_avx[176] = { 221,-61,232,168,-124,-134,-310,306,58,42,145,105,243,-244,287,31,-62,-67,-155,153,116,84,290,210,-199,-122,-177,-305,-248,-268,21,-29,-20,-20,-125,-125,-50,-50,8,8,-128,-128,-159,-159,-320,-320,-77,-77,308,308,2,2,129,129,5,5,-80,-80,141,141,-200,-200,32,32,-64,-64,-64,-64,241,241,241,241,-160,-160,-160,-160,282,282,282,282,-25,-25,-25,-25,4,4,4,4,258,258,258,258,10,10,10,10,-40,-40,-40,-40,-40,-40,-40,-40,-250,-250,-250,-250,-250,-250,-250,-250,-100,-100,-100,-100,-100,-100,-100,-100,16,16,16,16,16,16,16,16,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-318,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10 };
#endif 

#if NTT_DIM == 128 && PARAM_Q == 3329
void ntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(39);
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i t[12];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i tzeta[4];



	//level 6
	//mul
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);

	t[4] = _mm256_load_si256((__m256i*) & p256a[4]);
	t[5] = _mm256_load_si256((__m256i*) & p256a[5]);
	t[6] = _mm256_load_si256((__m256i*) & p256a[6]);
	t[7] = _mm256_load_si256((__m256i*) & p256a[7]);

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[0]);

	t[8] = _mm256_mullo_epi16(t[4], tzeta[0]);
	t[4] = _mm256_mulhi_epi16(t[4], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[5], tzeta[0]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[0]);
	t[10] = _mm256_mullo_epi16(t[6], tzeta[0]);
	t[6] = _mm256_mulhi_epi16(t[6], tzeta[0]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[0]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[0]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[4], t[8]);
	t[9] = _mm256_sub_epi16(t[5], t[9]);
	t[10] = _mm256_sub_epi16(t[6], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly
	t[4] = _mm256_sub_epi16(t[0], t[8]);
	t[5] = _mm256_sub_epi16(t[1], t[9]);
	t[6] = _mm256_sub_epi16(t[2], t[10]);
	t[7] = _mm256_sub_epi16(t[3], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[1] = _mm256_add_epi16(t[1], t[9]);
	t[2] = _mm256_add_epi16(t[2], t[10]);
	t[3] = _mm256_add_epi16(t[3], t[11]);

	//level 5
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[1]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[2]);

	t[8] = _mm256_mullo_epi16(t[2], tzeta[0]);
	t[2] = _mm256_mulhi_epi16(t[2], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[0]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[0]);
	t[10] = _mm256_mullo_epi16(t[6], tzeta[1]);
	t[6] = _mm256_mulhi_epi16(t[6], tzeta[1]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[1]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[1]);


	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[2], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[6], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly
	t[2] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[1], t[9]);
	t[6] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[5], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[1] = _mm256_add_epi16(t[1], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[10]);
	t[5] = _mm256_add_epi16(t[5], t[11]);

	//level 4
	//mul

	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[3]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[4]);
	tzeta[2] = _mm256_load_si256((__m256i*) & p256zeta[5]);
	tzeta[3] = _mm256_load_si256((__m256i*) & p256zeta[6]);

	t[8] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], tzeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[3]);


	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[1], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[5], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[8] = _mm256_add_epi16(t[0], t[8]);
	t[9] = _mm256_add_epi16(t[2], t[9]);
	t[10] = _mm256_add_epi16(t[4], t[10]);
	t[11] = _mm256_add_epi16(t[6], t[11]);


	//level 3  
	t[0] = _mm256_permute2x128_si256(t[8], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[8], t[1], 0x31);
	t[2] = _mm256_permute2x128_si256(t[9], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[9], t[3], 0x31);
	t[4] = _mm256_permute2x128_si256(t[10], t[5], 0x20);
	t[5] = _mm256_permute2x128_si256(t[10], t[5], 0x31);
	t[6] = _mm256_permute2x128_si256(t[11], t[7], 0x20);
	t[7] = _mm256_permute2x128_si256(t[11], t[7], 0x31);



	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[7]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[8]);
	tzeta[2] = _mm256_load_si256((__m256i*) & p256zeta[9]);
	tzeta[3] = _mm256_load_si256((__m256i*) & p256zeta[10]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], tzeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[3]);



	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[1], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[5], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[8] = _mm256_add_epi16(t[0], t[8]);
	t[9] = _mm256_add_epi16(t[2], t[9]);
	t[10] = _mm256_add_epi16(t[4], t[10]);
	t[11] = _mm256_add_epi16(t[6], t[11]);


	//level 2   
	t[8] = _mm256_permute4x64_epi64(t[8], 0xb1);
	t[9] = _mm256_permute4x64_epi64(t[9], 0xb1);
	t[10] = _mm256_permute4x64_epi64(t[10], 0xb1);
	t[11] = _mm256_permute4x64_epi64(t[11], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[8], 0xcc);
	t[1] = _mm256_blend_epi32(t[8], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[9], 0xcc);
	t[3] = _mm256_blend_epi32(t[9], t[3], 0xcc);
	t[4] = _mm256_blend_epi32(t[5], t[10], 0xcc);
	t[5] = _mm256_blend_epi32(t[10], t[5], 0xcc);
	t[6] = _mm256_blend_epi32(t[7], t[11], 0xcc);
	t[7] = _mm256_blend_epi32(t[11], t[7], 0xcc);

	t[0] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[2] = _mm256_permute4x64_epi64(t[2], 0xb1);
	t[4] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[6] = _mm256_permute4x64_epi64(t[6], 0xb1);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[11]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[12]);
	tzeta[2] = _mm256_load_si256((__m256i*) & p256zeta[13]);
	tzeta[3] = _mm256_load_si256((__m256i*) & p256zeta[14]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], tzeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[3]);


	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[1], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[5], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly        
	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[2] = _mm256_add_epi16(t[2], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[10]);
	t[6] = _mm256_add_epi16(t[6], t[11]);

	//level 1  
	t[8] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[9] = _mm256_permutevar8x32_epi32(t[2], idx);
	t[10] = _mm256_permutevar8x32_epi32(t[4], idx);
	t[11] = _mm256_permutevar8x32_epi32(t[6], idx);

	t[0] = _mm256_blend_epi32(t[1], t[8], 0xaa);
	t[1] = _mm256_blend_epi32(t[8], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[9], 0xaa);
	t[3] = _mm256_blend_epi32(t[9], t[3], 0xaa);
	t[4] = _mm256_blend_epi32(t[5], t[10], 0xaa);
	t[5] = _mm256_blend_epi32(t[10], t[5], 0xaa);
	t[6] = _mm256_blend_epi32(t[7], t[11], 0xaa);
	t[7] = _mm256_blend_epi32(t[11], t[7], 0xaa);

	t[0] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[2] = _mm256_permutevar8x32_epi32(t[2], idx);
	t[4] = _mm256_permutevar8x32_epi32(t[4], idx);
	t[6] = _mm256_permutevar8x32_epi32(t[6], idx);



	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[15]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[16]);
	tzeta[2] = _mm256_load_si256((__m256i*) & p256zeta[17]);
	tzeta[3] = _mm256_load_si256((__m256i*) & p256zeta[18]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], tzeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[3]);



	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[1], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[5], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[2] = _mm256_add_epi16(t[2], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[10]);
	t[6] = _mm256_add_epi16(t[6], t[11]);

	//level 0
	t[8] = _mm256_srli_epi32(t[0], 16);
	t[9] = _mm256_srli_epi32(t[2], 16);
	t[10] = _mm256_slli_epi32(t[1], 16);
	t[11] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[10], 0xaa);
	t[1] = _mm256_blend_epi16(t[8], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[11], 0xaa);
	t[3] = _mm256_blend_epi16(t[9], t[3], 0xaa);

	t[8] = _mm256_srli_epi32(t[4], 16);
	t[9] = _mm256_srli_epi32(t[6], 16);
	t[10] = _mm256_slli_epi32(t[5], 16);
	t[11] = _mm256_slli_epi32(t[7], 16);

	t[4] = _mm256_blend_epi16(t[4], t[10], 0xaa);
	t[5] = _mm256_blend_epi16(t[8], t[5], 0xaa);
	t[6] = _mm256_blend_epi16(t[6], t[11], 0xaa);
	t[7] = _mm256_blend_epi16(t[9], t[7], 0xaa);


	tzeta[0] = _mm256_load_si256((__m256i*) & p256zeta[19]);
	tzeta[1] = _mm256_load_si256((__m256i*) & p256zeta[20]);
	tzeta[2] = _mm256_load_si256((__m256i*) & p256zeta[21]);
	tzeta[3] = _mm256_load_si256((__m256i*) & p256zeta[22]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], tzeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], tzeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], tzeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], tzeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], tzeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], tzeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], tzeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], tzeta[3]);


	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[8] = _mm256_sub_epi16(t[1], t[8]);
	t[9] = _mm256_sub_epi16(t[3], t[9]);
	t[10] = _mm256_sub_epi16(t[5], t[10]);
	t[11] = _mm256_sub_epi16(t[7], t[11]);

	//butterfly      
	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[2] = _mm256_add_epi16(t[2], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[10]);
	t[6] = _mm256_add_epi16(t[6], t[11]);

	// reduce to (-Q,Q)

	t[8] = _mm256_mulhi_epi16(t[1], c16x);
	t[9] = _mm256_mulhi_epi16(t[3], c16x);
	t[10] = _mm256_mulhi_epi16(t[5], c16x);
	t[11] = _mm256_mulhi_epi16(t[7], c16x);

	t[8] = _mm256_add_epi16(t[8], one16x);
	t[9] = _mm256_add_epi16(t[9], one16x);
	t[10] = _mm256_add_epi16(t[10], one16x);
	t[11] = _mm256_add_epi16(t[11], one16x);

	t[8] = _mm256_srai_epi16(t[8], 1);
	t[9] = _mm256_srai_epi16(t[9], 1);
	t[10] = _mm256_srai_epi16(t[10], 1);
	t[11] = _mm256_srai_epi16(t[11], 1);

	t[8] = _mm256_mullo_epi16(t[8], vq16x);
	t[9] = _mm256_mullo_epi16(t[9], vq16x);
	t[10] = _mm256_mullo_epi16(t[10], vq16x);
	t[11] = _mm256_mullo_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);


	t[8] = _mm256_mulhi_epi16(t[0], c16x);
	t[9] = _mm256_mulhi_epi16(t[2], c16x);
	t[10] = _mm256_mulhi_epi16(t[4], c16x);
	t[11] = _mm256_mulhi_epi16(t[6], c16x);

	t[8] = _mm256_add_epi16(t[8], one16x);
	t[9] = _mm256_add_epi16(t[9], one16x);
	t[10] = _mm256_add_epi16(t[10], one16x);
	t[11] = _mm256_add_epi16(t[11], one16x);

	t[8] = _mm256_srai_epi16(t[8], 1);
	t[9] = _mm256_srai_epi16(t[9], 1);
	t[10] = _mm256_srai_epi16(t[10], 1);
	t[11] = _mm256_srai_epi16(t[11], 1);

	t[8] = _mm256_mullo_epi16(t[8], vq16x);
	t[9] = _mm256_mullo_epi16(t[9], vq16x);
	t[10] = _mm256_mullo_epi16(t[10], vq16x);
	t[11] = _mm256_mullo_epi16(t[11], vq16x);

	t[0] = _mm256_sub_epi16(t[0], t[8]);
	t[2] = _mm256_sub_epi16(t[2], t[9]);
	t[4] = _mm256_sub_epi16(t[4], t[10]);
	t[6] = _mm256_sub_epi16(t[6], t[11]);

	//store   
	t[8] = _mm256_unpacklo_epi16(t[0], t[1]);
	t[1] = _mm256_unpackhi_epi16(t[0], t[1]);
	t[9] = _mm256_unpacklo_epi16(t[2], t[3]);
	t[3] = _mm256_unpackhi_epi16(t[2], t[3]);
	t[10] = _mm256_unpacklo_epi16(t[4], t[5]);
	t[5] = _mm256_unpackhi_epi16(t[4], t[5]);
	t[11] = _mm256_unpacklo_epi16(t[6], t[7]);
	t[7] = _mm256_unpackhi_epi16(t[6], t[7]);

	p256a[0] = _mm256_permute2x128_si256(t[8], t[1], 0x20);
	p256a[1] = _mm256_permute2x128_si256(t[8], t[1], 0x31);
	p256a[2] = _mm256_permute2x128_si256(t[9], t[3], 0x20);
	p256a[3] = _mm256_permute2x128_si256(t[9], t[3], 0x31);
	p256a[4] = _mm256_permute2x128_si256(t[10], t[5], 0x20);
	p256a[5] = _mm256_permute2x128_si256(t[10], t[5], 0x31);
	p256a[6] = _mm256_permute2x128_si256(t[11], t[7], 0x20);
	p256a[7] = _mm256_permute2x128_si256(t[11], t[7], 0x31);

}

void invntt(int16_t* a)
{
	__m256i* p256a = (__m256i*) a;
	__m256i* p256zeta = (__m256i*) zetas_inv_avx;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i c16x = _mm256_set1_epi16(39);
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i invn16x = _mm256_set1_epi16(512); //invn*2^16 mod q
	__m256i t[16];
	__m256i idx = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
	__m256i mask = _mm256_set1_epi32(0xFFFF);



	//level 0   
	//pack the data
	t[0] = _mm256_load_si256((__m256i*) & p256a[0]);
	t[1] = _mm256_load_si256((__m256i*) & p256a[1]);
	t[2] = _mm256_load_si256((__m256i*) & p256a[2]);
	t[3] = _mm256_load_si256((__m256i*) & p256a[3]);
	t[4] = _mm256_load_si256((__m256i*) & p256a[4]);
	t[5] = _mm256_load_si256((__m256i*) & p256a[5]);
	t[6] = _mm256_load_si256((__m256i*) & p256a[6]);
	t[7] = _mm256_load_si256((__m256i*) & p256a[7]);


	t[8] = _mm256_permute2x128_si256(t[0], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[0], t[1], 0x31);
	t[9] = _mm256_permute2x128_si256(t[2], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[2], t[3], 0x31);
	t[10] = _mm256_permute2x128_si256(t[4], t[5], 0x20);
	t[5] = _mm256_permute2x128_si256(t[4], t[5], 0x31);
	t[11] = _mm256_permute2x128_si256(t[6], t[7], 0x20);
	t[7] = _mm256_permute2x128_si256(t[6], t[7], 0x31);


	t[12] = _mm256_and_si256(t[8], mask);
	t[13] = _mm256_and_si256(t[9], mask);
	t[14] = _mm256_and_si256(t[1], mask);
	t[15] = _mm256_and_si256(t[3], mask);
	t[0] = _mm256_packus_epi32(t[12], t[14]);
	t[2] = _mm256_packus_epi32(t[13], t[15]);

	t[12] = _mm256_and_si256(t[10], mask);
	t[13] = _mm256_and_si256(t[11], mask);
	t[14] = _mm256_and_si256(t[5], mask);
	t[15] = _mm256_and_si256(t[7], mask);
	t[4] = _mm256_packus_epi32(t[12], t[14]);
	t[6] = _mm256_packus_epi32(t[13], t[15]);


	t[8] = _mm256_srli_epi32(t[8], 16);
	t[1] = _mm256_srli_epi32(t[1], 16);
	t[9] = _mm256_srli_epi32(t[9], 16);
	t[3] = _mm256_srli_epi32(t[3], 16);
	t[10] = _mm256_srli_epi32(t[10], 16);
	t[5] = _mm256_srli_epi32(t[5], 16);
	t[11] = _mm256_srli_epi32(t[11], 16);
	t[7] = _mm256_srli_epi32(t[7], 16);

	t[8] = _mm256_packus_epi32(t[8], t[1]);
	t[9] = _mm256_packus_epi32(t[9], t[3]);
	t[10] = _mm256_packus_epi32(t[10], t[5]);
	t[11] = _mm256_packus_epi32(t[11], t[7]);

	//butterfly 

	t[1] = _mm256_sub_epi16(t[0], t[8]);
	t[3] = _mm256_sub_epi16(t[2], t[9]);
	t[5] = _mm256_sub_epi16(t[4], t[10]);
	t[7] = _mm256_sub_epi16(t[6], t[11]);

	t[0] = _mm256_add_epi16(t[0], t[8]);
	t[2] = _mm256_add_epi16(t[2], t[9]);
	t[4] = _mm256_add_epi16(t[4], t[10]);
	t[6] = _mm256_add_epi16(t[6], t[11]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], p256zeta[0]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[0]);
	t[9] = _mm256_mullo_epi16(t[3], p256zeta[1]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[1]);
	t[10] = _mm256_mullo_epi16(t[5], p256zeta[2]);
	t[5] = _mm256_mulhi_epi16(t[5], p256zeta[2]);
	t[11] = _mm256_mullo_epi16(t[7], p256zeta[3]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[3]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);

	//level 1  
	t[8] = _mm256_srli_epi32(t[0], 16);
	t[9] = _mm256_srli_epi32(t[2], 16);
	t[10] = _mm256_slli_epi32(t[1], 16);
	t[11] = _mm256_slli_epi32(t[3], 16);

	t[0] = _mm256_blend_epi16(t[0], t[10], 0xaa);
	t[1] = _mm256_blend_epi16(t[8], t[1], 0xaa);
	t[2] = _mm256_blend_epi16(t[2], t[11], 0xaa);
	t[3] = _mm256_blend_epi16(t[9], t[3], 0xaa);

	t[8] = _mm256_srli_epi32(t[4], 16);
	t[9] = _mm256_srli_epi32(t[6], 16);
	t[10] = _mm256_slli_epi32(t[5], 16);
	t[11] = _mm256_slli_epi32(t[7], 16);

	t[4] = _mm256_blend_epi16(t[4], t[10], 0xaa);
	t[5] = _mm256_blend_epi16(t[8], t[5], 0xaa);
	t[6] = _mm256_blend_epi16(t[6], t[11], 0xaa);
	t[7] = _mm256_blend_epi16(t[9], t[7], 0xaa);


	//butterfly 

	t[8] = _mm256_sub_epi16(t[0], t[1]);
	t[9] = _mm256_sub_epi16(t[2], t[3]);
	t[10] = _mm256_sub_epi16(t[4], t[5]);
	t[11] = _mm256_sub_epi16(t[6], t[7]);

	t[0] = _mm256_add_epi16(t[0], t[1]);
	t[2] = _mm256_add_epi16(t[2], t[3]);
	t[4] = _mm256_add_epi16(t[4], t[5]);
	t[6] = _mm256_add_epi16(t[6], t[7]);

	//mul
	t[1] = _mm256_mulhi_epi16(t[8], p256zeta[4]);
	t[8] = _mm256_mullo_epi16(t[8], p256zeta[4]);
	t[3] = _mm256_mulhi_epi16(t[9], p256zeta[5]);
	t[9] = _mm256_mullo_epi16(t[9], p256zeta[5]);
	t[5] = _mm256_mulhi_epi16(t[10], p256zeta[6]);
	t[10] = _mm256_mullo_epi16(t[10], p256zeta[6]);
	t[7] = _mm256_mulhi_epi16(t[11], p256zeta[7]);
	t[11] = _mm256_mullo_epi16(t[11], p256zeta[7]);


	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);


	//level 2

	t[8] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[9] = _mm256_permutevar8x32_epi32(t[2], idx);
	t[10] = _mm256_permutevar8x32_epi32(t[4], idx);
	t[11] = _mm256_permutevar8x32_epi32(t[6], idx);

	t[0] = _mm256_blend_epi32(t[1], t[8], 0xaa);
	t[1] = _mm256_blend_epi32(t[8], t[1], 0xaa);
	t[2] = _mm256_blend_epi32(t[3], t[9], 0xaa);
	t[3] = _mm256_blend_epi32(t[9], t[3], 0xaa);
	t[4] = _mm256_blend_epi32(t[5], t[10], 0xaa);
	t[5] = _mm256_blend_epi32(t[10], t[5], 0xaa);
	t[6] = _mm256_blend_epi32(t[7], t[11], 0xaa);
	t[7] = _mm256_blend_epi32(t[11], t[7], 0xaa);

	t[8] = _mm256_permutevar8x32_epi32(t[0], idx);
	t[9] = _mm256_permutevar8x32_epi32(t[2], idx);
	t[10] = _mm256_permutevar8x32_epi32(t[4], idx);
	t[11] = _mm256_permutevar8x32_epi32(t[6], idx);


	//butterfly 
	t[0] = _mm256_add_epi16(t[8], t[1]);
	t[2] = _mm256_add_epi16(t[9], t[3]);
	t[4] = _mm256_add_epi16(t[10], t[5]);
	t[6] = _mm256_add_epi16(t[11], t[7]);

	t[1] = _mm256_sub_epi16(t[8], t[1]);
	t[3] = _mm256_sub_epi16(t[9], t[3]);
	t[5] = _mm256_sub_epi16(t[10], t[5]);
	t[7] = _mm256_sub_epi16(t[11], t[7]);



	//mul
	t[8] = _mm256_mullo_epi16(t[1], p256zeta[8]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[8]);
	t[9] = _mm256_mullo_epi16(t[3], p256zeta[9]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[9]);
	t[10] = _mm256_mullo_epi16(t[5], p256zeta[10]);
	t[5] = _mm256_mulhi_epi16(t[5], p256zeta[10]);
	t[11] = _mm256_mullo_epi16(t[7], p256zeta[11]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[11]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);


	//barrat reduce to (-PARAM_Q,PARAM_Q)
	t[8] = _mm256_mulhi_epi16(t[0], c16x);
	t[9] = _mm256_mulhi_epi16(t[2], c16x);
	t[10] = _mm256_mulhi_epi16(t[4], c16x);
	t[11] = _mm256_mulhi_epi16(t[6], c16x);

	t[8] = _mm256_add_epi16(t[8], one16x);
	t[9] = _mm256_add_epi16(t[9], one16x);
	t[10] = _mm256_add_epi16(t[10], one16x);
	t[11] = _mm256_add_epi16(t[11], one16x);

	t[8] = _mm256_srai_epi16(t[8], 1);
	t[9] = _mm256_srai_epi16(t[9], 1);
	t[10] = _mm256_srai_epi16(t[10], 1);
	t[11] = _mm256_srai_epi16(t[11], 1);

	t[8] = _mm256_mullo_epi16(t[8], vq16x);
	t[9] = _mm256_mullo_epi16(t[9], vq16x);
	t[10] = _mm256_mullo_epi16(t[10], vq16x);
	t[11] = _mm256_mullo_epi16(t[11], vq16x);

	t[0] = _mm256_sub_epi16(t[0], t[8]);
	t[2] = _mm256_sub_epi16(t[2], t[9]);
	t[4] = _mm256_sub_epi16(t[4], t[10]);
	t[6] = _mm256_sub_epi16(t[6], t[11]);

	//level 3 
	t[8] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[9] = _mm256_permute4x64_epi64(t[2], 0xb1);
	t[10] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[11] = _mm256_permute4x64_epi64(t[6], 0xb1);

	t[0] = _mm256_blend_epi32(t[1], t[8], 0xcc);
	t[1] = _mm256_blend_epi32(t[8], t[1], 0xcc);
	t[2] = _mm256_blend_epi32(t[3], t[9], 0xcc);
	t[3] = _mm256_blend_epi32(t[9], t[3], 0xcc);
	t[4] = _mm256_blend_epi32(t[5], t[10], 0xcc);
	t[5] = _mm256_blend_epi32(t[10], t[5], 0xcc);
	t[6] = _mm256_blend_epi32(t[7], t[11], 0xcc);
	t[7] = _mm256_blend_epi32(t[11], t[7], 0xcc);

	t[8] = _mm256_permute4x64_epi64(t[0], 0xb1);
	t[9] = _mm256_permute4x64_epi64(t[2], 0xb1);
	t[10] = _mm256_permute4x64_epi64(t[4], 0xb1);
	t[11] = _mm256_permute4x64_epi64(t[6], 0xb1);

	//butterfly 
	t[0] = _mm256_add_epi16(t[8], t[1]);
	t[2] = _mm256_add_epi16(t[9], t[3]);
	t[4] = _mm256_add_epi16(t[10], t[5]);
	t[6] = _mm256_add_epi16(t[11], t[7]);

	t[1] = _mm256_sub_epi16(t[8], t[1]);
	t[3] = _mm256_sub_epi16(t[9], t[3]);
	t[5] = _mm256_sub_epi16(t[10], t[5]);
	t[7] = _mm256_sub_epi16(t[11], t[7]);

	//mul
	t[8] = _mm256_mullo_epi16(t[1], p256zeta[12]);
	t[1] = _mm256_mulhi_epi16(t[1], p256zeta[12]);
	t[9] = _mm256_mullo_epi16(t[3], p256zeta[13]);
	t[3] = _mm256_mulhi_epi16(t[3], p256zeta[13]);
	t[10] = _mm256_mullo_epi16(t[5], p256zeta[14]);
	t[5] = _mm256_mulhi_epi16(t[5], p256zeta[14]);
	t[11] = _mm256_mullo_epi16(t[7], p256zeta[15]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[15]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);

	//level 4 
	t[8] = _mm256_permute2x128_si256(t[0], t[1], 0x20);
	t[1] = _mm256_permute2x128_si256(t[0], t[1], 0x31);
	t[9] = _mm256_permute2x128_si256(t[2], t[3], 0x20);
	t[3] = _mm256_permute2x128_si256(t[2], t[3], 0x31);
	t[10] = _mm256_permute2x128_si256(t[4], t[5], 0x20);
	t[5] = _mm256_permute2x128_si256(t[4], t[5], 0x31);
	t[11] = _mm256_permute2x128_si256(t[6], t[7], 0x20);
	t[7] = _mm256_permute2x128_si256(t[6], t[7], 0x31);

	t[0] = _mm256_add_epi16(t[8], t[1]);
	t[2] = _mm256_add_epi16(t[9], t[3]);
	t[4] = _mm256_add_epi16(t[10], t[5]);
	t[6] = _mm256_add_epi16(t[11], t[7]);
	
	t[8] = _mm256_sub_epi16(t[8], t[1]);
	t[9] = _mm256_sub_epi16(t[9], t[3]);
	t[10] = _mm256_sub_epi16(t[10], t[5]);
	t[11] = _mm256_sub_epi16(t[11], t[7]);

	

	//mul
	t[1] = _mm256_mulhi_epi16(t[8], p256zeta[16]);
	t[8] = _mm256_mullo_epi16(t[8], p256zeta[16]);
	t[3] = _mm256_mulhi_epi16(t[9], p256zeta[17]);
	t[9] = _mm256_mullo_epi16(t[9], p256zeta[17]);
	t[5] = _mm256_mulhi_epi16(t[10], p256zeta[18]);
	t[10] = _mm256_mullo_epi16(t[10], p256zeta[18]);
	t[7] = _mm256_mulhi_epi16(t[11], p256zeta[19]);
	t[11] = _mm256_mullo_epi16(t[11], p256zeta[19]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[1] = _mm256_sub_epi16(t[1], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[5] = _mm256_sub_epi16(t[5], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);


	//level 5  
	//butterfly 

	t[8] = _mm256_sub_epi16(t[0], t[2]);
	t[9] = _mm256_sub_epi16(t[1], t[3]);
	t[10] = _mm256_sub_epi16(t[4], t[6]);
	t[11] = _mm256_sub_epi16(t[5], t[7]);

	t[0] = _mm256_add_epi16(t[0], t[2]);
	t[1] = _mm256_add_epi16(t[1], t[3]);
	t[4] = _mm256_add_epi16(t[4], t[6]);
	t[5] = _mm256_add_epi16(t[5], t[7]);



	//mul
	t[2] = _mm256_mulhi_epi16(t[8], p256zeta[20]);
	t[8] = _mm256_mullo_epi16(t[8], p256zeta[20]);
	t[3] = _mm256_mulhi_epi16(t[9], p256zeta[20]);
	t[9] = _mm256_mullo_epi16(t[9], p256zeta[20]);
	t[6] = _mm256_mulhi_epi16(t[10], p256zeta[21]);
	t[10] = _mm256_mullo_epi16(t[10], p256zeta[21]);
	t[7] = _mm256_mulhi_epi16(t[11], p256zeta[21]);
	t[11] = _mm256_mullo_epi16(t[11], p256zeta[21]);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	t[2] = _mm256_sub_epi16(t[2], t[8]);
	t[3] = _mm256_sub_epi16(t[3], t[9]);
	t[6] = _mm256_sub_epi16(t[6], t[10]);
	t[7] = _mm256_sub_epi16(t[7], t[11]);


	
	//barrat reduce to (-PARAM_Q,PARAM_Q)
	t[8] = _mm256_mulhi_epi16(t[0], c16x);
	t[9] = _mm256_mulhi_epi16(t[1], c16x);
	t[10] = _mm256_mulhi_epi16(t[4], c16x);
	t[11] = _mm256_mulhi_epi16(t[4], c16x);

	t[8] = _mm256_add_epi16(t[8], one16x);
	t[9] = _mm256_add_epi16(t[9], one16x);
	t[10] = _mm256_add_epi16(t[10], one16x);
	t[11] = _mm256_add_epi16(t[11], one16x);

	t[8] = _mm256_srai_epi16(t[8], 1);
	t[9] = _mm256_srai_epi16(t[9], 1);
	t[10] = _mm256_srai_epi16(t[10], 1);
	t[11] = _mm256_srai_epi16(t[11], 1);

	t[8] = _mm256_mullo_epi16(t[8], vq16x);
	t[9] = _mm256_mullo_epi16(t[9], vq16x);
	t[10] = _mm256_mullo_epi16(t[10], vq16x);
	t[11] = _mm256_mullo_epi16(t[11], vq16x);

	t[0] = _mm256_sub_epi16(t[0], t[8]);
	t[1] = _mm256_sub_epi16(t[1], t[9]);
	t[4] = _mm256_sub_epi16(t[4], t[10]);
	t[5] = _mm256_sub_epi16(t[5], t[11]);


	//level 6

	//butterfly 
	t[8] = _mm256_add_epi16(t[0], t[4]);
	t[9] = _mm256_add_epi16(t[1], t[5]);
	t[10] = _mm256_add_epi16(t[2], t[6]);
	t[11] = _mm256_add_epi16(t[3], t[7]);

	t[4] = _mm256_sub_epi16(t[0], t[4]);
	t[5] = _mm256_sub_epi16(t[1], t[5]);
	t[6] = _mm256_sub_epi16(t[2], t[6]);
	t[7] = _mm256_sub_epi16(t[3], t[7]);


	//mul invn
	t[0] = _mm256_mulhi_epi16(t[8], invn16x);
	t[8] = _mm256_mullo_epi16(t[8], invn16x);
	t[1] = _mm256_mulhi_epi16(t[9], invn16x);
	t[9] = _mm256_mullo_epi16(t[9], invn16x);
	t[2] = _mm256_mulhi_epi16(t[10], invn16x);
	t[10] = _mm256_mullo_epi16(t[10], invn16x);
	t[3] = _mm256_mulhi_epi16(t[11], invn16x);
	t[11] = _mm256_mullo_epi16(t[11], invn16x);

	//reduce
	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	p256a[0] = _mm256_sub_epi16(t[0], t[8]);
	p256a[1] = _mm256_sub_epi16(t[1], t[9]);
	p256a[2] = _mm256_sub_epi16(t[2], t[10]);
	p256a[3] = _mm256_sub_epi16(t[3], t[11]);

	//mul
	t[8] = _mm256_mullo_epi16(t[4], p256zeta[22]);
	t[4] = _mm256_mulhi_epi16(t[4], p256zeta[22]);
	t[9] = _mm256_mullo_epi16(t[5], p256zeta[22]);
	t[5] = _mm256_mulhi_epi16(t[5], p256zeta[22]);
	t[10] = _mm256_mullo_epi16(t[6], p256zeta[22]);
	t[6] = _mm256_mulhi_epi16(t[6], p256zeta[22]);
	t[11] = _mm256_mullo_epi16(t[7], p256zeta[22]);
	t[7] = _mm256_mulhi_epi16(t[7], p256zeta[22]);

	t[8] = _mm256_mullo_epi16(t[8], vqinv16x);
	t[9] = _mm256_mullo_epi16(t[9], vqinv16x);
	t[10] = _mm256_mullo_epi16(t[10], vqinv16x);
	t[11] = _mm256_mullo_epi16(t[11], vqinv16x);

	t[8] = _mm256_mulhi_epi16(t[8], vq16x);
	t[9] = _mm256_mulhi_epi16(t[9], vq16x);
	t[10] = _mm256_mulhi_epi16(t[10], vq16x);
	t[11] = _mm256_mulhi_epi16(t[11], vq16x);

	p256a[4] = _mm256_sub_epi16(t[4], t[8]);
	p256a[5] = _mm256_sub_epi16(t[5], t[9]);
	p256a[6] = _mm256_sub_epi16(t[6], t[10]);
	p256a[7] = _mm256_sub_epi16(t[7], t[11]);
}
#elif NTT_DIM == 64
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
	__m256i invn16x = _mm256_set1_epi16(1024); //invn*2^16 mod q
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
#endif
