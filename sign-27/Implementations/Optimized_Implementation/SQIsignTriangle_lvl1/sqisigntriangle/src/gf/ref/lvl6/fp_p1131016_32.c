// clang-format off
//Automatically generated modular arithmetic C code
//Command line : python monty.py 32 0x70ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//Python Script by Mike Scott (Technology Innovation Institute, UAE, 2025)

#include <stdio.h>
#include <stdint.h>

#define sspint int32_t
#define spint uint32_t
#define dpint uint64_t
#define udpint uint64_t
#define sdpint int64_t
#define Wordlength 32
#define Nlimbs 36
#define Radix 29
#define Nbits 1023
#define Nbytes 128

#define MONTGOMERY
//propagate carries
inline static spint prop(spint *n) {
	int i;
	spint mask=((spint)1<<29u)-(spint)1;
	sspint carry=(sspint)n[0];
	carry>>=29u;
	n[0]&=mask;
	for (i=1;i<35;i++) {
		carry+=(sspint)n[i];
		n[i] = (spint)carry & mask;
		carry>>=29u;
	}
	n[35]+=(spint)carry;
	return -((n[35]>>1)>>30u);
}

//propagate carries and add p if negative, propagate carries again
static spint flatten(spint *n) {
	spint carry=prop(n);
	n[0]-=(spint)1u&carry;
	n[35]+=((spint)0xe2u)&carry;
	(void)prop(n);
	return (carry&1);
}

//Montgomery final subtract
static spint modfsb(spint *n) {
	n[0]+=(spint)1u;
	n[35]-=(spint)0xe2u;
	return flatten(n);
}

//Modular addition - reduce less than 2p
static void modadd(const spint *a,const spint *b,spint *n) {
	spint carry;
	n[0]=a[0]+b[0];
	n[1]=a[1]+b[1];
	n[2]=a[2]+b[2];
	n[3]=a[3]+b[3];
	n[4]=a[4]+b[4];
	n[5]=a[5]+b[5];
	n[6]=a[6]+b[6];
	n[7]=a[7]+b[7];
	n[8]=a[8]+b[8];
	n[9]=a[9]+b[9];
	n[10]=a[10]+b[10];
	n[11]=a[11]+b[11];
	n[12]=a[12]+b[12];
	n[13]=a[13]+b[13];
	n[14]=a[14]+b[14];
	n[15]=a[15]+b[15];
	n[16]=a[16]+b[16];
	n[17]=a[17]+b[17];
	n[18]=a[18]+b[18];
	n[19]=a[19]+b[19];
	n[20]=a[20]+b[20];
	n[21]=a[21]+b[21];
	n[22]=a[22]+b[22];
	n[23]=a[23]+b[23];
	n[24]=a[24]+b[24];
	n[25]=a[25]+b[25];
	n[26]=a[26]+b[26];
	n[27]=a[27]+b[27];
	n[28]=a[28]+b[28];
	n[29]=a[29]+b[29];
	n[30]=a[30]+b[30];
	n[31]=a[31]+b[31];
	n[32]=a[32]+b[32];
	n[33]=a[33]+b[33];
	n[34]=a[34]+b[34];
	n[35]=a[35]+b[35];
	n[0]+=(spint)2u;
	n[35]-=(spint)0x1c4u;
	carry=prop(n);
	n[0]-=(spint)2u&carry;
	n[35]+=((spint)0x1c4u)&carry;
	(void)prop(n);
}

//Modular subtraction - reduce less than 2p
static void modsub(const spint *a,const spint *b,spint *n) {
	spint carry;
	n[0]=a[0]-b[0];
	n[1]=a[1]-b[1];
	n[2]=a[2]-b[2];
	n[3]=a[3]-b[3];
	n[4]=a[4]-b[4];
	n[5]=a[5]-b[5];
	n[6]=a[6]-b[6];
	n[7]=a[7]-b[7];
	n[8]=a[8]-b[8];
	n[9]=a[9]-b[9];
	n[10]=a[10]-b[10];
	n[11]=a[11]-b[11];
	n[12]=a[12]-b[12];
	n[13]=a[13]-b[13];
	n[14]=a[14]-b[14];
	n[15]=a[15]-b[15];
	n[16]=a[16]-b[16];
	n[17]=a[17]-b[17];
	n[18]=a[18]-b[18];
	n[19]=a[19]-b[19];
	n[20]=a[20]-b[20];
	n[21]=a[21]-b[21];
	n[22]=a[22]-b[22];
	n[23]=a[23]-b[23];
	n[24]=a[24]-b[24];
	n[25]=a[25]-b[25];
	n[26]=a[26]-b[26];
	n[27]=a[27]-b[27];
	n[28]=a[28]-b[28];
	n[29]=a[29]-b[29];
	n[30]=a[30]-b[30];
	n[31]=a[31]-b[31];
	n[32]=a[32]-b[32];
	n[33]=a[33]-b[33];
	n[34]=a[34]-b[34];
	n[35]=a[35]-b[35];
	carry=prop(n);
	n[0]-=(spint)2u&carry;
	n[35]+=((spint)0x1c4u)&carry;
	(void)prop(n);
}

//Modular negation
static void modneg(const spint *b,spint *n) {
	spint carry;
	n[0]=(spint)0-b[0];
	n[1]=(spint)0-b[1];
	n[2]=(spint)0-b[2];
	n[3]=(spint)0-b[3];
	n[4]=(spint)0-b[4];
	n[5]=(spint)0-b[5];
	n[6]=(spint)0-b[6];
	n[7]=(spint)0-b[7];
	n[8]=(spint)0-b[8];
	n[9]=(spint)0-b[9];
	n[10]=(spint)0-b[10];
	n[11]=(spint)0-b[11];
	n[12]=(spint)0-b[12];
	n[13]=(spint)0-b[13];
	n[14]=(spint)0-b[14];
	n[15]=(spint)0-b[15];
	n[16]=(spint)0-b[16];
	n[17]=(spint)0-b[17];
	n[18]=(spint)0-b[18];
	n[19]=(spint)0-b[19];
	n[20]=(spint)0-b[20];
	n[21]=(spint)0-b[21];
	n[22]=(spint)0-b[22];
	n[23]=(spint)0-b[23];
	n[24]=(spint)0-b[24];
	n[25]=(spint)0-b[25];
	n[26]=(spint)0-b[26];
	n[27]=(spint)0-b[27];
	n[28]=(spint)0-b[28];
	n[29]=(spint)0-b[29];
	n[30]=(spint)0-b[30];
	n[31]=(spint)0-b[31];
	n[32]=(spint)0-b[32];
	n[33]=(spint)0-b[33];
	n[34]=(spint)0-b[34];
	n[35]=(spint)0-b[35];
	carry=prop(n);
	n[0]-=(spint)2u&carry;
	n[35]+=((spint)0x1c4u)&carry;
	(void)prop(n);
}

// Overflow limit   = 18446744073709551616
// maximum possible = 10376293658499481410
// Modular multiplication, c=a*b mod 2p
static void modmul(const spint *a,const spint *b,spint *c) {
	dpint t=0;
	spint p35=0xe2u;
	spint q=((spint)1<<29u); // q is unsaturated radix 
	spint mask=(spint)(q-(spint)1);
	t+=(dpint)a[0]*b[0]; spint v0=((spint)t & mask); t>>=29;
	t+=(dpint)a[0]*b[1]; t+=(dpint)a[1]*b[0]; spint v1=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[2]; t+=(dpint)a[1]*b[1]; t+=(dpint)a[2]*b[0]; spint v2=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[3]; t+=(dpint)a[1]*b[2]; t+=(dpint)a[2]*b[1]; t+=(dpint)a[3]*b[0]; spint v3=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[4]; t+=(dpint)a[1]*b[3]; t+=(dpint)a[2]*b[2]; t+=(dpint)a[3]*b[1]; t+=(dpint)a[4]*b[0]; spint v4=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[5]; t+=(dpint)a[1]*b[4]; t+=(dpint)a[2]*b[3]; t+=(dpint)a[3]*b[2]; t+=(dpint)a[4]*b[1]; t+=(dpint)a[5]*b[0]; spint v5=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[6]; t+=(dpint)a[1]*b[5]; t+=(dpint)a[2]*b[4]; t+=(dpint)a[3]*b[3]; t+=(dpint)a[4]*b[2]; t+=(dpint)a[5]*b[1]; t+=(dpint)a[6]*b[0]; spint v6=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[7]; t+=(dpint)a[1]*b[6]; t+=(dpint)a[2]*b[5]; t+=(dpint)a[3]*b[4]; t+=(dpint)a[4]*b[3]; t+=(dpint)a[5]*b[2]; t+=(dpint)a[6]*b[1]; t+=(dpint)a[7]*b[0]; spint v7=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[8]; t+=(dpint)a[1]*b[7]; t+=(dpint)a[2]*b[6]; t+=(dpint)a[3]*b[5]; t+=(dpint)a[4]*b[4]; t+=(dpint)a[5]*b[3]; t+=(dpint)a[6]*b[2]; t+=(dpint)a[7]*b[1]; t+=(dpint)a[8]*b[0]; spint v8=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[9]; t+=(dpint)a[1]*b[8]; t+=(dpint)a[2]*b[7]; t+=(dpint)a[3]*b[6]; t+=(dpint)a[4]*b[5]; t+=(dpint)a[5]*b[4]; t+=(dpint)a[6]*b[3]; t+=(dpint)a[7]*b[2]; t+=(dpint)a[8]*b[1]; t+=(dpint)a[9]*b[0]; spint v9=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[10]; t+=(dpint)a[1]*b[9]; t+=(dpint)a[2]*b[8]; t+=(dpint)a[3]*b[7]; t+=(dpint)a[4]*b[6]; t+=(dpint)a[5]*b[5]; t+=(dpint)a[6]*b[4]; t+=(dpint)a[7]*b[3]; t+=(dpint)a[8]*b[2]; t+=(dpint)a[9]*b[1]; t+=(dpint)a[10]*b[0]; spint v10=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[11]; t+=(dpint)a[1]*b[10]; t+=(dpint)a[2]*b[9]; t+=(dpint)a[3]*b[8]; t+=(dpint)a[4]*b[7]; t+=(dpint)a[5]*b[6]; t+=(dpint)a[6]*b[5]; t+=(dpint)a[7]*b[4]; t+=(dpint)a[8]*b[3]; t+=(dpint)a[9]*b[2]; t+=(dpint)a[10]*b[1]; t+=(dpint)a[11]*b[0]; spint v11=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[12]; t+=(dpint)a[1]*b[11]; t+=(dpint)a[2]*b[10]; t+=(dpint)a[3]*b[9]; t+=(dpint)a[4]*b[8]; t+=(dpint)a[5]*b[7]; t+=(dpint)a[6]*b[6]; t+=(dpint)a[7]*b[5]; t+=(dpint)a[8]*b[4]; t+=(dpint)a[9]*b[3]; t+=(dpint)a[10]*b[2]; t+=(dpint)a[11]*b[1]; t+=(dpint)a[12]*b[0]; spint v12=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[13]; t+=(dpint)a[1]*b[12]; t+=(dpint)a[2]*b[11]; t+=(dpint)a[3]*b[10]; t+=(dpint)a[4]*b[9]; t+=(dpint)a[5]*b[8]; t+=(dpint)a[6]*b[7]; t+=(dpint)a[7]*b[6]; t+=(dpint)a[8]*b[5]; t+=(dpint)a[9]*b[4]; t+=(dpint)a[10]*b[3]; t+=(dpint)a[11]*b[2]; t+=(dpint)a[12]*b[1]; t+=(dpint)a[13]*b[0]; spint v13=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[14]; t+=(dpint)a[1]*b[13]; t+=(dpint)a[2]*b[12]; t+=(dpint)a[3]*b[11]; t+=(dpint)a[4]*b[10]; t+=(dpint)a[5]*b[9]; t+=(dpint)a[6]*b[8]; t+=(dpint)a[7]*b[7]; t+=(dpint)a[8]*b[6]; t+=(dpint)a[9]*b[5]; t+=(dpint)a[10]*b[4]; t+=(dpint)a[11]*b[3]; t+=(dpint)a[12]*b[2]; t+=(dpint)a[13]*b[1]; t+=(dpint)a[14]*b[0]; spint v14=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[15]; t+=(dpint)a[1]*b[14]; t+=(dpint)a[2]*b[13]; t+=(dpint)a[3]*b[12]; t+=(dpint)a[4]*b[11]; t+=(dpint)a[5]*b[10]; t+=(dpint)a[6]*b[9]; t+=(dpint)a[7]*b[8]; t+=(dpint)a[8]*b[7]; t+=(dpint)a[9]*b[6]; t+=(dpint)a[10]*b[5]; t+=(dpint)a[11]*b[4]; t+=(dpint)a[12]*b[3]; t+=(dpint)a[13]*b[2]; t+=(dpint)a[14]*b[1]; t+=(dpint)a[15]*b[0]; spint v15=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[16]; t+=(dpint)a[1]*b[15]; t+=(dpint)a[2]*b[14]; t+=(dpint)a[3]*b[13]; t+=(dpint)a[4]*b[12]; t+=(dpint)a[5]*b[11]; t+=(dpint)a[6]*b[10]; t+=(dpint)a[7]*b[9]; t+=(dpint)a[8]*b[8]; t+=(dpint)a[9]*b[7]; t+=(dpint)a[10]*b[6]; t+=(dpint)a[11]*b[5]; t+=(dpint)a[12]*b[4]; t+=(dpint)a[13]*b[3]; t+=(dpint)a[14]*b[2]; t+=(dpint)a[15]*b[1]; t+=(dpint)a[16]*b[0]; spint v16=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[17]; t+=(dpint)a[1]*b[16]; t+=(dpint)a[2]*b[15]; t+=(dpint)a[3]*b[14]; t+=(dpint)a[4]*b[13]; t+=(dpint)a[5]*b[12]; t+=(dpint)a[6]*b[11]; t+=(dpint)a[7]*b[10]; t+=(dpint)a[8]*b[9]; t+=(dpint)a[9]*b[8]; t+=(dpint)a[10]*b[7]; t+=(dpint)a[11]*b[6]; t+=(dpint)a[12]*b[5]; t+=(dpint)a[13]*b[4]; t+=(dpint)a[14]*b[3]; t+=(dpint)a[15]*b[2]; t+=(dpint)a[16]*b[1]; t+=(dpint)a[17]*b[0]; spint v17=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[18]; t+=(dpint)a[1]*b[17]; t+=(dpint)a[2]*b[16]; t+=(dpint)a[3]*b[15]; t+=(dpint)a[4]*b[14]; t+=(dpint)a[5]*b[13]; t+=(dpint)a[6]*b[12]; t+=(dpint)a[7]*b[11]; t+=(dpint)a[8]*b[10]; t+=(dpint)a[9]*b[9]; t+=(dpint)a[10]*b[8]; t+=(dpint)a[11]*b[7]; t+=(dpint)a[12]*b[6]; t+=(dpint)a[13]*b[5]; t+=(dpint)a[14]*b[4]; t+=(dpint)a[15]*b[3]; t+=(dpint)a[16]*b[2]; t+=(dpint)a[17]*b[1]; t+=(dpint)a[18]*b[0]; spint v18=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[19]; t+=(dpint)a[1]*b[18]; t+=(dpint)a[2]*b[17]; t+=(dpint)a[3]*b[16]; t+=(dpint)a[4]*b[15]; t+=(dpint)a[5]*b[14]; t+=(dpint)a[6]*b[13]; t+=(dpint)a[7]*b[12]; t+=(dpint)a[8]*b[11]; t+=(dpint)a[9]*b[10]; t+=(dpint)a[10]*b[9]; t+=(dpint)a[11]*b[8]; t+=(dpint)a[12]*b[7]; t+=(dpint)a[13]*b[6]; t+=(dpint)a[14]*b[5]; t+=(dpint)a[15]*b[4]; t+=(dpint)a[16]*b[3]; t+=(dpint)a[17]*b[2]; t+=(dpint)a[18]*b[1]; t+=(dpint)a[19]*b[0]; spint v19=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[20]; t+=(dpint)a[1]*b[19]; t+=(dpint)a[2]*b[18]; t+=(dpint)a[3]*b[17]; t+=(dpint)a[4]*b[16]; t+=(dpint)a[5]*b[15]; t+=(dpint)a[6]*b[14]; t+=(dpint)a[7]*b[13]; t+=(dpint)a[8]*b[12]; t+=(dpint)a[9]*b[11]; t+=(dpint)a[10]*b[10]; t+=(dpint)a[11]*b[9]; t+=(dpint)a[12]*b[8]; t+=(dpint)a[13]*b[7]; t+=(dpint)a[14]*b[6]; t+=(dpint)a[15]*b[5]; t+=(dpint)a[16]*b[4]; t+=(dpint)a[17]*b[3]; t+=(dpint)a[18]*b[2]; t+=(dpint)a[19]*b[1]; t+=(dpint)a[20]*b[0]; spint v20=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[21]; t+=(dpint)a[1]*b[20]; t+=(dpint)a[2]*b[19]; t+=(dpint)a[3]*b[18]; t+=(dpint)a[4]*b[17]; t+=(dpint)a[5]*b[16]; t+=(dpint)a[6]*b[15]; t+=(dpint)a[7]*b[14]; t+=(dpint)a[8]*b[13]; t+=(dpint)a[9]*b[12]; t+=(dpint)a[10]*b[11]; t+=(dpint)a[11]*b[10]; t+=(dpint)a[12]*b[9]; t+=(dpint)a[13]*b[8]; t+=(dpint)a[14]*b[7]; t+=(dpint)a[15]*b[6]; t+=(dpint)a[16]*b[5]; t+=(dpint)a[17]*b[4]; t+=(dpint)a[18]*b[3]; t+=(dpint)a[19]*b[2]; t+=(dpint)a[20]*b[1]; t+=(dpint)a[21]*b[0]; spint v21=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[22]; t+=(dpint)a[1]*b[21]; t+=(dpint)a[2]*b[20]; t+=(dpint)a[3]*b[19]; t+=(dpint)a[4]*b[18]; t+=(dpint)a[5]*b[17]; t+=(dpint)a[6]*b[16]; t+=(dpint)a[7]*b[15]; t+=(dpint)a[8]*b[14]; t+=(dpint)a[9]*b[13]; t+=(dpint)a[10]*b[12]; t+=(dpint)a[11]*b[11]; t+=(dpint)a[12]*b[10]; t+=(dpint)a[13]*b[9]; t+=(dpint)a[14]*b[8]; t+=(dpint)a[15]*b[7]; t+=(dpint)a[16]*b[6]; t+=(dpint)a[17]*b[5]; t+=(dpint)a[18]*b[4]; t+=(dpint)a[19]*b[3]; t+=(dpint)a[20]*b[2]; t+=(dpint)a[21]*b[1]; t+=(dpint)a[22]*b[0]; spint v22=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[23]; t+=(dpint)a[1]*b[22]; t+=(dpint)a[2]*b[21]; t+=(dpint)a[3]*b[20]; t+=(dpint)a[4]*b[19]; t+=(dpint)a[5]*b[18]; t+=(dpint)a[6]*b[17]; t+=(dpint)a[7]*b[16]; t+=(dpint)a[8]*b[15]; t+=(dpint)a[9]*b[14]; t+=(dpint)a[10]*b[13]; t+=(dpint)a[11]*b[12]; t+=(dpint)a[12]*b[11]; t+=(dpint)a[13]*b[10]; t+=(dpint)a[14]*b[9]; t+=(dpint)a[15]*b[8]; t+=(dpint)a[16]*b[7]; t+=(dpint)a[17]*b[6]; t+=(dpint)a[18]*b[5]; t+=(dpint)a[19]*b[4]; t+=(dpint)a[20]*b[3]; t+=(dpint)a[21]*b[2]; t+=(dpint)a[22]*b[1]; t+=(dpint)a[23]*b[0]; spint v23=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[24]; t+=(dpint)a[1]*b[23]; t+=(dpint)a[2]*b[22]; t+=(dpint)a[3]*b[21]; t+=(dpint)a[4]*b[20]; t+=(dpint)a[5]*b[19]; t+=(dpint)a[6]*b[18]; t+=(dpint)a[7]*b[17]; t+=(dpint)a[8]*b[16]; t+=(dpint)a[9]*b[15]; t+=(dpint)a[10]*b[14]; t+=(dpint)a[11]*b[13]; t+=(dpint)a[12]*b[12]; t+=(dpint)a[13]*b[11]; t+=(dpint)a[14]*b[10]; t+=(dpint)a[15]*b[9]; t+=(dpint)a[16]*b[8]; t+=(dpint)a[17]*b[7]; t+=(dpint)a[18]*b[6]; t+=(dpint)a[19]*b[5]; t+=(dpint)a[20]*b[4]; t+=(dpint)a[21]*b[3]; t+=(dpint)a[22]*b[2]; t+=(dpint)a[23]*b[1]; t+=(dpint)a[24]*b[0]; spint v24=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[25]; t+=(dpint)a[1]*b[24]; t+=(dpint)a[2]*b[23]; t+=(dpint)a[3]*b[22]; t+=(dpint)a[4]*b[21]; t+=(dpint)a[5]*b[20]; t+=(dpint)a[6]*b[19]; t+=(dpint)a[7]*b[18]; t+=(dpint)a[8]*b[17]; t+=(dpint)a[9]*b[16]; t+=(dpint)a[10]*b[15]; t+=(dpint)a[11]*b[14]; t+=(dpint)a[12]*b[13]; t+=(dpint)a[13]*b[12]; t+=(dpint)a[14]*b[11]; t+=(dpint)a[15]*b[10]; t+=(dpint)a[16]*b[9]; t+=(dpint)a[17]*b[8]; t+=(dpint)a[18]*b[7]; t+=(dpint)a[19]*b[6]; t+=(dpint)a[20]*b[5]; t+=(dpint)a[21]*b[4]; t+=(dpint)a[22]*b[3]; t+=(dpint)a[23]*b[2]; t+=(dpint)a[24]*b[1]; t+=(dpint)a[25]*b[0]; spint v25=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[26]; t+=(dpint)a[1]*b[25]; t+=(dpint)a[2]*b[24]; t+=(dpint)a[3]*b[23]; t+=(dpint)a[4]*b[22]; t+=(dpint)a[5]*b[21]; t+=(dpint)a[6]*b[20]; t+=(dpint)a[7]*b[19]; t+=(dpint)a[8]*b[18]; t+=(dpint)a[9]*b[17]; t+=(dpint)a[10]*b[16]; t+=(dpint)a[11]*b[15]; t+=(dpint)a[12]*b[14]; t+=(dpint)a[13]*b[13]; t+=(dpint)a[14]*b[12]; t+=(dpint)a[15]*b[11]; t+=(dpint)a[16]*b[10]; t+=(dpint)a[17]*b[9]; t+=(dpint)a[18]*b[8]; t+=(dpint)a[19]*b[7]; t+=(dpint)a[20]*b[6]; t+=(dpint)a[21]*b[5]; t+=(dpint)a[22]*b[4]; t+=(dpint)a[23]*b[3]; t+=(dpint)a[24]*b[2]; t+=(dpint)a[25]*b[1]; t+=(dpint)a[26]*b[0]; spint v26=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[27]; t+=(dpint)a[1]*b[26]; t+=(dpint)a[2]*b[25]; t+=(dpint)a[3]*b[24]; t+=(dpint)a[4]*b[23]; t+=(dpint)a[5]*b[22]; t+=(dpint)a[6]*b[21]; t+=(dpint)a[7]*b[20]; t+=(dpint)a[8]*b[19]; t+=(dpint)a[9]*b[18]; t+=(dpint)a[10]*b[17]; t+=(dpint)a[11]*b[16]; t+=(dpint)a[12]*b[15]; t+=(dpint)a[13]*b[14]; t+=(dpint)a[14]*b[13]; t+=(dpint)a[15]*b[12]; t+=(dpint)a[16]*b[11]; t+=(dpint)a[17]*b[10]; t+=(dpint)a[18]*b[9]; t+=(dpint)a[19]*b[8]; t+=(dpint)a[20]*b[7]; t+=(dpint)a[21]*b[6]; t+=(dpint)a[22]*b[5]; t+=(dpint)a[23]*b[4]; t+=(dpint)a[24]*b[3]; t+=(dpint)a[25]*b[2]; t+=(dpint)a[26]*b[1]; t+=(dpint)a[27]*b[0]; spint v27=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[28]; t+=(dpint)a[1]*b[27]; t+=(dpint)a[2]*b[26]; t+=(dpint)a[3]*b[25]; t+=(dpint)a[4]*b[24]; t+=(dpint)a[5]*b[23]; t+=(dpint)a[6]*b[22]; t+=(dpint)a[7]*b[21]; t+=(dpint)a[8]*b[20]; t+=(dpint)a[9]*b[19]; t+=(dpint)a[10]*b[18]; t+=(dpint)a[11]*b[17]; t+=(dpint)a[12]*b[16]; t+=(dpint)a[13]*b[15]; t+=(dpint)a[14]*b[14]; t+=(dpint)a[15]*b[13]; t+=(dpint)a[16]*b[12]; t+=(dpint)a[17]*b[11]; t+=(dpint)a[18]*b[10]; t+=(dpint)a[19]*b[9]; t+=(dpint)a[20]*b[8]; t+=(dpint)a[21]*b[7]; t+=(dpint)a[22]*b[6]; t+=(dpint)a[23]*b[5]; t+=(dpint)a[24]*b[4]; t+=(dpint)a[25]*b[3]; t+=(dpint)a[26]*b[2]; t+=(dpint)a[27]*b[1]; t+=(dpint)a[28]*b[0]; spint v28=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[29]; t+=(dpint)a[1]*b[28]; t+=(dpint)a[2]*b[27]; t+=(dpint)a[3]*b[26]; t+=(dpint)a[4]*b[25]; t+=(dpint)a[5]*b[24]; t+=(dpint)a[6]*b[23]; t+=(dpint)a[7]*b[22]; t+=(dpint)a[8]*b[21]; t+=(dpint)a[9]*b[20]; t+=(dpint)a[10]*b[19]; t+=(dpint)a[11]*b[18]; t+=(dpint)a[12]*b[17]; t+=(dpint)a[13]*b[16]; t+=(dpint)a[14]*b[15]; t+=(dpint)a[15]*b[14]; t+=(dpint)a[16]*b[13]; t+=(dpint)a[17]*b[12]; t+=(dpint)a[18]*b[11]; t+=(dpint)a[19]*b[10]; t+=(dpint)a[20]*b[9]; t+=(dpint)a[21]*b[8]; t+=(dpint)a[22]*b[7]; t+=(dpint)a[23]*b[6]; t+=(dpint)a[24]*b[5]; t+=(dpint)a[25]*b[4]; t+=(dpint)a[26]*b[3]; t+=(dpint)a[27]*b[2]; t+=(dpint)a[28]*b[1]; t+=(dpint)a[29]*b[0]; spint v29=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[30]; t+=(dpint)a[1]*b[29]; t+=(dpint)a[2]*b[28]; t+=(dpint)a[3]*b[27]; t+=(dpint)a[4]*b[26]; t+=(dpint)a[5]*b[25]; t+=(dpint)a[6]*b[24]; t+=(dpint)a[7]*b[23]; t+=(dpint)a[8]*b[22]; t+=(dpint)a[9]*b[21]; t+=(dpint)a[10]*b[20]; t+=(dpint)a[11]*b[19]; t+=(dpint)a[12]*b[18]; t+=(dpint)a[13]*b[17]; t+=(dpint)a[14]*b[16]; t+=(dpint)a[15]*b[15]; t+=(dpint)a[16]*b[14]; t+=(dpint)a[17]*b[13]; t+=(dpint)a[18]*b[12]; t+=(dpint)a[19]*b[11]; t+=(dpint)a[20]*b[10]; t+=(dpint)a[21]*b[9]; t+=(dpint)a[22]*b[8]; t+=(dpint)a[23]*b[7]; t+=(dpint)a[24]*b[6]; t+=(dpint)a[25]*b[5]; t+=(dpint)a[26]*b[4]; t+=(dpint)a[27]*b[3]; t+=(dpint)a[28]*b[2]; t+=(dpint)a[29]*b[1]; t+=(dpint)a[30]*b[0]; spint v30=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[31]; t+=(dpint)a[1]*b[30]; t+=(dpint)a[2]*b[29]; t+=(dpint)a[3]*b[28]; t+=(dpint)a[4]*b[27]; t+=(dpint)a[5]*b[26]; t+=(dpint)a[6]*b[25]; t+=(dpint)a[7]*b[24]; t+=(dpint)a[8]*b[23]; t+=(dpint)a[9]*b[22]; t+=(dpint)a[10]*b[21]; t+=(dpint)a[11]*b[20]; t+=(dpint)a[12]*b[19]; t+=(dpint)a[13]*b[18]; t+=(dpint)a[14]*b[17]; t+=(dpint)a[15]*b[16]; t+=(dpint)a[16]*b[15]; t+=(dpint)a[17]*b[14]; t+=(dpint)a[18]*b[13]; t+=(dpint)a[19]*b[12]; t+=(dpint)a[20]*b[11]; t+=(dpint)a[21]*b[10]; t+=(dpint)a[22]*b[9]; t+=(dpint)a[23]*b[8]; t+=(dpint)a[24]*b[7]; t+=(dpint)a[25]*b[6]; t+=(dpint)a[26]*b[5]; t+=(dpint)a[27]*b[4]; t+=(dpint)a[28]*b[3]; t+=(dpint)a[29]*b[2]; t+=(dpint)a[30]*b[1]; t+=(dpint)a[31]*b[0]; spint v31=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[32]; t+=(dpint)a[1]*b[31]; t+=(dpint)a[2]*b[30]; t+=(dpint)a[3]*b[29]; t+=(dpint)a[4]*b[28]; t+=(dpint)a[5]*b[27]; t+=(dpint)a[6]*b[26]; t+=(dpint)a[7]*b[25]; t+=(dpint)a[8]*b[24]; t+=(dpint)a[9]*b[23]; t+=(dpint)a[10]*b[22]; t+=(dpint)a[11]*b[21]; t+=(dpint)a[12]*b[20]; t+=(dpint)a[13]*b[19]; t+=(dpint)a[14]*b[18]; t+=(dpint)a[15]*b[17]; t+=(dpint)a[16]*b[16]; t+=(dpint)a[17]*b[15]; t+=(dpint)a[18]*b[14]; t+=(dpint)a[19]*b[13]; t+=(dpint)a[20]*b[12]; t+=(dpint)a[21]*b[11]; t+=(dpint)a[22]*b[10]; t+=(dpint)a[23]*b[9]; t+=(dpint)a[24]*b[8]; t+=(dpint)a[25]*b[7]; t+=(dpint)a[26]*b[6]; t+=(dpint)a[27]*b[5]; t+=(dpint)a[28]*b[4]; t+=(dpint)a[29]*b[3]; t+=(dpint)a[30]*b[2]; t+=(dpint)a[31]*b[1]; t+=(dpint)a[32]*b[0]; spint v32=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[33]; t+=(dpint)a[1]*b[32]; t+=(dpint)a[2]*b[31]; t+=(dpint)a[3]*b[30]; t+=(dpint)a[4]*b[29]; t+=(dpint)a[5]*b[28]; t+=(dpint)a[6]*b[27]; t+=(dpint)a[7]*b[26]; t+=(dpint)a[8]*b[25]; t+=(dpint)a[9]*b[24]; t+=(dpint)a[10]*b[23]; t+=(dpint)a[11]*b[22]; t+=(dpint)a[12]*b[21]; t+=(dpint)a[13]*b[20]; t+=(dpint)a[14]*b[19]; t+=(dpint)a[15]*b[18]; t+=(dpint)a[16]*b[17]; t+=(dpint)a[17]*b[16]; t+=(dpint)a[18]*b[15]; t+=(dpint)a[19]*b[14]; t+=(dpint)a[20]*b[13]; t+=(dpint)a[21]*b[12]; t+=(dpint)a[22]*b[11]; t+=(dpint)a[23]*b[10]; t+=(dpint)a[24]*b[9]; t+=(dpint)a[25]*b[8]; t+=(dpint)a[26]*b[7]; t+=(dpint)a[27]*b[6]; t+=(dpint)a[28]*b[5]; t+=(dpint)a[29]*b[4]; t+=(dpint)a[30]*b[3]; t+=(dpint)a[31]*b[2]; t+=(dpint)a[32]*b[1]; t+=(dpint)a[33]*b[0]; spint v33=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[34]; t+=(dpint)a[1]*b[33]; t+=(dpint)a[2]*b[32]; t+=(dpint)a[3]*b[31]; t+=(dpint)a[4]*b[30]; t+=(dpint)a[5]*b[29]; t+=(dpint)a[6]*b[28]; t+=(dpint)a[7]*b[27]; t+=(dpint)a[8]*b[26]; t+=(dpint)a[9]*b[25]; t+=(dpint)a[10]*b[24]; t+=(dpint)a[11]*b[23]; t+=(dpint)a[12]*b[22]; t+=(dpint)a[13]*b[21]; t+=(dpint)a[14]*b[20]; t+=(dpint)a[15]*b[19]; t+=(dpint)a[16]*b[18]; t+=(dpint)a[17]*b[17]; t+=(dpint)a[18]*b[16]; t+=(dpint)a[19]*b[15]; t+=(dpint)a[20]*b[14]; t+=(dpint)a[21]*b[13]; t+=(dpint)a[22]*b[12]; t+=(dpint)a[23]*b[11]; t+=(dpint)a[24]*b[10]; t+=(dpint)a[25]*b[9]; t+=(dpint)a[26]*b[8]; t+=(dpint)a[27]*b[7]; t+=(dpint)a[28]*b[6]; t+=(dpint)a[29]*b[5]; t+=(dpint)a[30]*b[4]; t+=(dpint)a[31]*b[3]; t+=(dpint)a[32]*b[2]; t+=(dpint)a[33]*b[1]; t+=(dpint)a[34]*b[0]; spint v34=((spint)t & mask);  t>>=29;
	t+=(dpint)a[0]*b[35]; t+=(dpint)a[1]*b[34]; t+=(dpint)a[2]*b[33]; t+=(dpint)a[3]*b[32]; t+=(dpint)a[4]*b[31]; t+=(dpint)a[5]*b[30]; t+=(dpint)a[6]*b[29]; t+=(dpint)a[7]*b[28]; t+=(dpint)a[8]*b[27]; t+=(dpint)a[9]*b[26]; t+=(dpint)a[10]*b[25]; t+=(dpint)a[11]*b[24]; t+=(dpint)a[12]*b[23]; t+=(dpint)a[13]*b[22]; t+=(dpint)a[14]*b[21]; t+=(dpint)a[15]*b[20]; t+=(dpint)a[16]*b[19]; t+=(dpint)a[17]*b[18]; t+=(dpint)a[18]*b[17]; t+=(dpint)a[19]*b[16]; t+=(dpint)a[20]*b[15]; t+=(dpint)a[21]*b[14]; t+=(dpint)a[22]*b[13]; t+=(dpint)a[23]*b[12]; t+=(dpint)a[24]*b[11]; t+=(dpint)a[25]*b[10]; t+=(dpint)a[26]*b[9]; t+=(dpint)a[27]*b[8]; t+=(dpint)a[28]*b[7]; t+=(dpint)a[29]*b[6]; t+=(dpint)a[30]*b[5]; t+=(dpint)a[31]*b[4]; t+=(dpint)a[32]*b[3]; t+=(dpint)a[33]*b[2]; t+=(dpint)a[34]*b[1]; t+=(dpint)a[35]*b[0]; t+=(dpint)v0*(dpint)p35;  spint v35=((spint)t & mask);  t>>=29;
	t+=(dpint)a[1]*b[35]; t+=(dpint)a[2]*b[34]; t+=(dpint)a[3]*b[33]; t+=(dpint)a[4]*b[32]; t+=(dpint)a[5]*b[31]; t+=(dpint)a[6]*b[30]; t+=(dpint)a[7]*b[29]; t+=(dpint)a[8]*b[28]; t+=(dpint)a[9]*b[27]; t+=(dpint)a[10]*b[26]; t+=(dpint)a[11]*b[25]; t+=(dpint)a[12]*b[24]; t+=(dpint)a[13]*b[23]; t+=(dpint)a[14]*b[22]; t+=(dpint)a[15]*b[21]; t+=(dpint)a[16]*b[20]; t+=(dpint)a[17]*b[19]; t+=(dpint)a[18]*b[18]; t+=(dpint)a[19]*b[17]; t+=(dpint)a[20]*b[16]; t+=(dpint)a[21]*b[15]; t+=(dpint)a[22]*b[14]; t+=(dpint)a[23]*b[13]; t+=(dpint)a[24]*b[12]; t+=(dpint)a[25]*b[11]; t+=(dpint)a[26]*b[10]; t+=(dpint)a[27]*b[9]; t+=(dpint)a[28]*b[8]; t+=(dpint)a[29]*b[7]; t+=(dpint)a[30]*b[6]; t+=(dpint)a[31]*b[5]; t+=(dpint)a[32]*b[4]; t+=(dpint)a[33]*b[3]; t+=(dpint)a[34]*b[2]; t+=(dpint)a[35]*b[1]; t+=(dpint)v1*(dpint)p35;  c[0]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[2]*b[35]; t+=(dpint)a[3]*b[34]; t+=(dpint)a[4]*b[33]; t+=(dpint)a[5]*b[32]; t+=(dpint)a[6]*b[31]; t+=(dpint)a[7]*b[30]; t+=(dpint)a[8]*b[29]; t+=(dpint)a[9]*b[28]; t+=(dpint)a[10]*b[27]; t+=(dpint)a[11]*b[26]; t+=(dpint)a[12]*b[25]; t+=(dpint)a[13]*b[24]; t+=(dpint)a[14]*b[23]; t+=(dpint)a[15]*b[22]; t+=(dpint)a[16]*b[21]; t+=(dpint)a[17]*b[20]; t+=(dpint)a[18]*b[19]; t+=(dpint)a[19]*b[18]; t+=(dpint)a[20]*b[17]; t+=(dpint)a[21]*b[16]; t+=(dpint)a[22]*b[15]; t+=(dpint)a[23]*b[14]; t+=(dpint)a[24]*b[13]; t+=(dpint)a[25]*b[12]; t+=(dpint)a[26]*b[11]; t+=(dpint)a[27]*b[10]; t+=(dpint)a[28]*b[9]; t+=(dpint)a[29]*b[8]; t+=(dpint)a[30]*b[7]; t+=(dpint)a[31]*b[6]; t+=(dpint)a[32]*b[5]; t+=(dpint)a[33]*b[4]; t+=(dpint)a[34]*b[3]; t+=(dpint)a[35]*b[2]; t+=(dpint)v2*(dpint)p35;  c[1]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[3]*b[35]; t+=(dpint)a[4]*b[34]; t+=(dpint)a[5]*b[33]; t+=(dpint)a[6]*b[32]; t+=(dpint)a[7]*b[31]; t+=(dpint)a[8]*b[30]; t+=(dpint)a[9]*b[29]; t+=(dpint)a[10]*b[28]; t+=(dpint)a[11]*b[27]; t+=(dpint)a[12]*b[26]; t+=(dpint)a[13]*b[25]; t+=(dpint)a[14]*b[24]; t+=(dpint)a[15]*b[23]; t+=(dpint)a[16]*b[22]; t+=(dpint)a[17]*b[21]; t+=(dpint)a[18]*b[20]; t+=(dpint)a[19]*b[19]; t+=(dpint)a[20]*b[18]; t+=(dpint)a[21]*b[17]; t+=(dpint)a[22]*b[16]; t+=(dpint)a[23]*b[15]; t+=(dpint)a[24]*b[14]; t+=(dpint)a[25]*b[13]; t+=(dpint)a[26]*b[12]; t+=(dpint)a[27]*b[11]; t+=(dpint)a[28]*b[10]; t+=(dpint)a[29]*b[9]; t+=(dpint)a[30]*b[8]; t+=(dpint)a[31]*b[7]; t+=(dpint)a[32]*b[6]; t+=(dpint)a[33]*b[5]; t+=(dpint)a[34]*b[4]; t+=(dpint)a[35]*b[3]; t+=(dpint)v3*(dpint)p35;  c[2]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[4]*b[35]; t+=(dpint)a[5]*b[34]; t+=(dpint)a[6]*b[33]; t+=(dpint)a[7]*b[32]; t+=(dpint)a[8]*b[31]; t+=(dpint)a[9]*b[30]; t+=(dpint)a[10]*b[29]; t+=(dpint)a[11]*b[28]; t+=(dpint)a[12]*b[27]; t+=(dpint)a[13]*b[26]; t+=(dpint)a[14]*b[25]; t+=(dpint)a[15]*b[24]; t+=(dpint)a[16]*b[23]; t+=(dpint)a[17]*b[22]; t+=(dpint)a[18]*b[21]; t+=(dpint)a[19]*b[20]; t+=(dpint)a[20]*b[19]; t+=(dpint)a[21]*b[18]; t+=(dpint)a[22]*b[17]; t+=(dpint)a[23]*b[16]; t+=(dpint)a[24]*b[15]; t+=(dpint)a[25]*b[14]; t+=(dpint)a[26]*b[13]; t+=(dpint)a[27]*b[12]; t+=(dpint)a[28]*b[11]; t+=(dpint)a[29]*b[10]; t+=(dpint)a[30]*b[9]; t+=(dpint)a[31]*b[8]; t+=(dpint)a[32]*b[7]; t+=(dpint)a[33]*b[6]; t+=(dpint)a[34]*b[5]; t+=(dpint)a[35]*b[4]; t+=(dpint)v4*(dpint)p35;  c[3]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[5]*b[35]; t+=(dpint)a[6]*b[34]; t+=(dpint)a[7]*b[33]; t+=(dpint)a[8]*b[32]; t+=(dpint)a[9]*b[31]; t+=(dpint)a[10]*b[30]; t+=(dpint)a[11]*b[29]; t+=(dpint)a[12]*b[28]; t+=(dpint)a[13]*b[27]; t+=(dpint)a[14]*b[26]; t+=(dpint)a[15]*b[25]; t+=(dpint)a[16]*b[24]; t+=(dpint)a[17]*b[23]; t+=(dpint)a[18]*b[22]; t+=(dpint)a[19]*b[21]; t+=(dpint)a[20]*b[20]; t+=(dpint)a[21]*b[19]; t+=(dpint)a[22]*b[18]; t+=(dpint)a[23]*b[17]; t+=(dpint)a[24]*b[16]; t+=(dpint)a[25]*b[15]; t+=(dpint)a[26]*b[14]; t+=(dpint)a[27]*b[13]; t+=(dpint)a[28]*b[12]; t+=(dpint)a[29]*b[11]; t+=(dpint)a[30]*b[10]; t+=(dpint)a[31]*b[9]; t+=(dpint)a[32]*b[8]; t+=(dpint)a[33]*b[7]; t+=(dpint)a[34]*b[6]; t+=(dpint)a[35]*b[5]; t+=(dpint)v5*(dpint)p35;  c[4]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[6]*b[35]; t+=(dpint)a[7]*b[34]; t+=(dpint)a[8]*b[33]; t+=(dpint)a[9]*b[32]; t+=(dpint)a[10]*b[31]; t+=(dpint)a[11]*b[30]; t+=(dpint)a[12]*b[29]; t+=(dpint)a[13]*b[28]; t+=(dpint)a[14]*b[27]; t+=(dpint)a[15]*b[26]; t+=(dpint)a[16]*b[25]; t+=(dpint)a[17]*b[24]; t+=(dpint)a[18]*b[23]; t+=(dpint)a[19]*b[22]; t+=(dpint)a[20]*b[21]; t+=(dpint)a[21]*b[20]; t+=(dpint)a[22]*b[19]; t+=(dpint)a[23]*b[18]; t+=(dpint)a[24]*b[17]; t+=(dpint)a[25]*b[16]; t+=(dpint)a[26]*b[15]; t+=(dpint)a[27]*b[14]; t+=(dpint)a[28]*b[13]; t+=(dpint)a[29]*b[12]; t+=(dpint)a[30]*b[11]; t+=(dpint)a[31]*b[10]; t+=(dpint)a[32]*b[9]; t+=(dpint)a[33]*b[8]; t+=(dpint)a[34]*b[7]; t+=(dpint)a[35]*b[6]; t+=(dpint)v6*(dpint)p35;  c[5]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[7]*b[35]; t+=(dpint)a[8]*b[34]; t+=(dpint)a[9]*b[33]; t+=(dpint)a[10]*b[32]; t+=(dpint)a[11]*b[31]; t+=(dpint)a[12]*b[30]; t+=(dpint)a[13]*b[29]; t+=(dpint)a[14]*b[28]; t+=(dpint)a[15]*b[27]; t+=(dpint)a[16]*b[26]; t+=(dpint)a[17]*b[25]; t+=(dpint)a[18]*b[24]; t+=(dpint)a[19]*b[23]; t+=(dpint)a[20]*b[22]; t+=(dpint)a[21]*b[21]; t+=(dpint)a[22]*b[20]; t+=(dpint)a[23]*b[19]; t+=(dpint)a[24]*b[18]; t+=(dpint)a[25]*b[17]; t+=(dpint)a[26]*b[16]; t+=(dpint)a[27]*b[15]; t+=(dpint)a[28]*b[14]; t+=(dpint)a[29]*b[13]; t+=(dpint)a[30]*b[12]; t+=(dpint)a[31]*b[11]; t+=(dpint)a[32]*b[10]; t+=(dpint)a[33]*b[9]; t+=(dpint)a[34]*b[8]; t+=(dpint)a[35]*b[7]; t+=(dpint)v7*(dpint)p35;  c[6]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[8]*b[35]; t+=(dpint)a[9]*b[34]; t+=(dpint)a[10]*b[33]; t+=(dpint)a[11]*b[32]; t+=(dpint)a[12]*b[31]; t+=(dpint)a[13]*b[30]; t+=(dpint)a[14]*b[29]; t+=(dpint)a[15]*b[28]; t+=(dpint)a[16]*b[27]; t+=(dpint)a[17]*b[26]; t+=(dpint)a[18]*b[25]; t+=(dpint)a[19]*b[24]; t+=(dpint)a[20]*b[23]; t+=(dpint)a[21]*b[22]; t+=(dpint)a[22]*b[21]; t+=(dpint)a[23]*b[20]; t+=(dpint)a[24]*b[19]; t+=(dpint)a[25]*b[18]; t+=(dpint)a[26]*b[17]; t+=(dpint)a[27]*b[16]; t+=(dpint)a[28]*b[15]; t+=(dpint)a[29]*b[14]; t+=(dpint)a[30]*b[13]; t+=(dpint)a[31]*b[12]; t+=(dpint)a[32]*b[11]; t+=(dpint)a[33]*b[10]; t+=(dpint)a[34]*b[9]; t+=(dpint)a[35]*b[8]; t+=(dpint)v8*(dpint)p35;  c[7]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[9]*b[35]; t+=(dpint)a[10]*b[34]; t+=(dpint)a[11]*b[33]; t+=(dpint)a[12]*b[32]; t+=(dpint)a[13]*b[31]; t+=(dpint)a[14]*b[30]; t+=(dpint)a[15]*b[29]; t+=(dpint)a[16]*b[28]; t+=(dpint)a[17]*b[27]; t+=(dpint)a[18]*b[26]; t+=(dpint)a[19]*b[25]; t+=(dpint)a[20]*b[24]; t+=(dpint)a[21]*b[23]; t+=(dpint)a[22]*b[22]; t+=(dpint)a[23]*b[21]; t+=(dpint)a[24]*b[20]; t+=(dpint)a[25]*b[19]; t+=(dpint)a[26]*b[18]; t+=(dpint)a[27]*b[17]; t+=(dpint)a[28]*b[16]; t+=(dpint)a[29]*b[15]; t+=(dpint)a[30]*b[14]; t+=(dpint)a[31]*b[13]; t+=(dpint)a[32]*b[12]; t+=(dpint)a[33]*b[11]; t+=(dpint)a[34]*b[10]; t+=(dpint)a[35]*b[9]; t+=(dpint)v9*(dpint)p35;  c[8]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[10]*b[35]; t+=(dpint)a[11]*b[34]; t+=(dpint)a[12]*b[33]; t+=(dpint)a[13]*b[32]; t+=(dpint)a[14]*b[31]; t+=(dpint)a[15]*b[30]; t+=(dpint)a[16]*b[29]; t+=(dpint)a[17]*b[28]; t+=(dpint)a[18]*b[27]; t+=(dpint)a[19]*b[26]; t+=(dpint)a[20]*b[25]; t+=(dpint)a[21]*b[24]; t+=(dpint)a[22]*b[23]; t+=(dpint)a[23]*b[22]; t+=(dpint)a[24]*b[21]; t+=(dpint)a[25]*b[20]; t+=(dpint)a[26]*b[19]; t+=(dpint)a[27]*b[18]; t+=(dpint)a[28]*b[17]; t+=(dpint)a[29]*b[16]; t+=(dpint)a[30]*b[15]; t+=(dpint)a[31]*b[14]; t+=(dpint)a[32]*b[13]; t+=(dpint)a[33]*b[12]; t+=(dpint)a[34]*b[11]; t+=(dpint)a[35]*b[10]; t+=(dpint)v10*(dpint)p35;  c[9]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[11]*b[35]; t+=(dpint)a[12]*b[34]; t+=(dpint)a[13]*b[33]; t+=(dpint)a[14]*b[32]; t+=(dpint)a[15]*b[31]; t+=(dpint)a[16]*b[30]; t+=(dpint)a[17]*b[29]; t+=(dpint)a[18]*b[28]; t+=(dpint)a[19]*b[27]; t+=(dpint)a[20]*b[26]; t+=(dpint)a[21]*b[25]; t+=(dpint)a[22]*b[24]; t+=(dpint)a[23]*b[23]; t+=(dpint)a[24]*b[22]; t+=(dpint)a[25]*b[21]; t+=(dpint)a[26]*b[20]; t+=(dpint)a[27]*b[19]; t+=(dpint)a[28]*b[18]; t+=(dpint)a[29]*b[17]; t+=(dpint)a[30]*b[16]; t+=(dpint)a[31]*b[15]; t+=(dpint)a[32]*b[14]; t+=(dpint)a[33]*b[13]; t+=(dpint)a[34]*b[12]; t+=(dpint)a[35]*b[11]; t+=(dpint)v11*(dpint)p35;  c[10]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[12]*b[35]; t+=(dpint)a[13]*b[34]; t+=(dpint)a[14]*b[33]; t+=(dpint)a[15]*b[32]; t+=(dpint)a[16]*b[31]; t+=(dpint)a[17]*b[30]; t+=(dpint)a[18]*b[29]; t+=(dpint)a[19]*b[28]; t+=(dpint)a[20]*b[27]; t+=(dpint)a[21]*b[26]; t+=(dpint)a[22]*b[25]; t+=(dpint)a[23]*b[24]; t+=(dpint)a[24]*b[23]; t+=(dpint)a[25]*b[22]; t+=(dpint)a[26]*b[21]; t+=(dpint)a[27]*b[20]; t+=(dpint)a[28]*b[19]; t+=(dpint)a[29]*b[18]; t+=(dpint)a[30]*b[17]; t+=(dpint)a[31]*b[16]; t+=(dpint)a[32]*b[15]; t+=(dpint)a[33]*b[14]; t+=(dpint)a[34]*b[13]; t+=(dpint)a[35]*b[12]; t+=(dpint)v12*(dpint)p35;  c[11]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[13]*b[35]; t+=(dpint)a[14]*b[34]; t+=(dpint)a[15]*b[33]; t+=(dpint)a[16]*b[32]; t+=(dpint)a[17]*b[31]; t+=(dpint)a[18]*b[30]; t+=(dpint)a[19]*b[29]; t+=(dpint)a[20]*b[28]; t+=(dpint)a[21]*b[27]; t+=(dpint)a[22]*b[26]; t+=(dpint)a[23]*b[25]; t+=(dpint)a[24]*b[24]; t+=(dpint)a[25]*b[23]; t+=(dpint)a[26]*b[22]; t+=(dpint)a[27]*b[21]; t+=(dpint)a[28]*b[20]; t+=(dpint)a[29]*b[19]; t+=(dpint)a[30]*b[18]; t+=(dpint)a[31]*b[17]; t+=(dpint)a[32]*b[16]; t+=(dpint)a[33]*b[15]; t+=(dpint)a[34]*b[14]; t+=(dpint)a[35]*b[13]; t+=(dpint)v13*(dpint)p35;  c[12]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[14]*b[35]; t+=(dpint)a[15]*b[34]; t+=(dpint)a[16]*b[33]; t+=(dpint)a[17]*b[32]; t+=(dpint)a[18]*b[31]; t+=(dpint)a[19]*b[30]; t+=(dpint)a[20]*b[29]; t+=(dpint)a[21]*b[28]; t+=(dpint)a[22]*b[27]; t+=(dpint)a[23]*b[26]; t+=(dpint)a[24]*b[25]; t+=(dpint)a[25]*b[24]; t+=(dpint)a[26]*b[23]; t+=(dpint)a[27]*b[22]; t+=(dpint)a[28]*b[21]; t+=(dpint)a[29]*b[20]; t+=(dpint)a[30]*b[19]; t+=(dpint)a[31]*b[18]; t+=(dpint)a[32]*b[17]; t+=(dpint)a[33]*b[16]; t+=(dpint)a[34]*b[15]; t+=(dpint)a[35]*b[14]; t+=(dpint)v14*(dpint)p35;  c[13]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[15]*b[35]; t+=(dpint)a[16]*b[34]; t+=(dpint)a[17]*b[33]; t+=(dpint)a[18]*b[32]; t+=(dpint)a[19]*b[31]; t+=(dpint)a[20]*b[30]; t+=(dpint)a[21]*b[29]; t+=(dpint)a[22]*b[28]; t+=(dpint)a[23]*b[27]; t+=(dpint)a[24]*b[26]; t+=(dpint)a[25]*b[25]; t+=(dpint)a[26]*b[24]; t+=(dpint)a[27]*b[23]; t+=(dpint)a[28]*b[22]; t+=(dpint)a[29]*b[21]; t+=(dpint)a[30]*b[20]; t+=(dpint)a[31]*b[19]; t+=(dpint)a[32]*b[18]; t+=(dpint)a[33]*b[17]; t+=(dpint)a[34]*b[16]; t+=(dpint)a[35]*b[15]; t+=(dpint)v15*(dpint)p35;  c[14]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[16]*b[35]; t+=(dpint)a[17]*b[34]; t+=(dpint)a[18]*b[33]; t+=(dpint)a[19]*b[32]; t+=(dpint)a[20]*b[31]; t+=(dpint)a[21]*b[30]; t+=(dpint)a[22]*b[29]; t+=(dpint)a[23]*b[28]; t+=(dpint)a[24]*b[27]; t+=(dpint)a[25]*b[26]; t+=(dpint)a[26]*b[25]; t+=(dpint)a[27]*b[24]; t+=(dpint)a[28]*b[23]; t+=(dpint)a[29]*b[22]; t+=(dpint)a[30]*b[21]; t+=(dpint)a[31]*b[20]; t+=(dpint)a[32]*b[19]; t+=(dpint)a[33]*b[18]; t+=(dpint)a[34]*b[17]; t+=(dpint)a[35]*b[16]; t+=(dpint)v16*(dpint)p35;  c[15]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[17]*b[35]; t+=(dpint)a[18]*b[34]; t+=(dpint)a[19]*b[33]; t+=(dpint)a[20]*b[32]; t+=(dpint)a[21]*b[31]; t+=(dpint)a[22]*b[30]; t+=(dpint)a[23]*b[29]; t+=(dpint)a[24]*b[28]; t+=(dpint)a[25]*b[27]; t+=(dpint)a[26]*b[26]; t+=(dpint)a[27]*b[25]; t+=(dpint)a[28]*b[24]; t+=(dpint)a[29]*b[23]; t+=(dpint)a[30]*b[22]; t+=(dpint)a[31]*b[21]; t+=(dpint)a[32]*b[20]; t+=(dpint)a[33]*b[19]; t+=(dpint)a[34]*b[18]; t+=(dpint)a[35]*b[17]; t+=(dpint)v17*(dpint)p35;  c[16]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[18]*b[35]; t+=(dpint)a[19]*b[34]; t+=(dpint)a[20]*b[33]; t+=(dpint)a[21]*b[32]; t+=(dpint)a[22]*b[31]; t+=(dpint)a[23]*b[30]; t+=(dpint)a[24]*b[29]; t+=(dpint)a[25]*b[28]; t+=(dpint)a[26]*b[27]; t+=(dpint)a[27]*b[26]; t+=(dpint)a[28]*b[25]; t+=(dpint)a[29]*b[24]; t+=(dpint)a[30]*b[23]; t+=(dpint)a[31]*b[22]; t+=(dpint)a[32]*b[21]; t+=(dpint)a[33]*b[20]; t+=(dpint)a[34]*b[19]; t+=(dpint)a[35]*b[18]; t+=(dpint)v18*(dpint)p35;  c[17]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[19]*b[35]; t+=(dpint)a[20]*b[34]; t+=(dpint)a[21]*b[33]; t+=(dpint)a[22]*b[32]; t+=(dpint)a[23]*b[31]; t+=(dpint)a[24]*b[30]; t+=(dpint)a[25]*b[29]; t+=(dpint)a[26]*b[28]; t+=(dpint)a[27]*b[27]; t+=(dpint)a[28]*b[26]; t+=(dpint)a[29]*b[25]; t+=(dpint)a[30]*b[24]; t+=(dpint)a[31]*b[23]; t+=(dpint)a[32]*b[22]; t+=(dpint)a[33]*b[21]; t+=(dpint)a[34]*b[20]; t+=(dpint)a[35]*b[19]; t+=(dpint)v19*(dpint)p35;  c[18]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[20]*b[35]; t+=(dpint)a[21]*b[34]; t+=(dpint)a[22]*b[33]; t+=(dpint)a[23]*b[32]; t+=(dpint)a[24]*b[31]; t+=(dpint)a[25]*b[30]; t+=(dpint)a[26]*b[29]; t+=(dpint)a[27]*b[28]; t+=(dpint)a[28]*b[27]; t+=(dpint)a[29]*b[26]; t+=(dpint)a[30]*b[25]; t+=(dpint)a[31]*b[24]; t+=(dpint)a[32]*b[23]; t+=(dpint)a[33]*b[22]; t+=(dpint)a[34]*b[21]; t+=(dpint)a[35]*b[20]; t+=(dpint)v20*(dpint)p35;  c[19]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[21]*b[35]; t+=(dpint)a[22]*b[34]; t+=(dpint)a[23]*b[33]; t+=(dpint)a[24]*b[32]; t+=(dpint)a[25]*b[31]; t+=(dpint)a[26]*b[30]; t+=(dpint)a[27]*b[29]; t+=(dpint)a[28]*b[28]; t+=(dpint)a[29]*b[27]; t+=(dpint)a[30]*b[26]; t+=(dpint)a[31]*b[25]; t+=(dpint)a[32]*b[24]; t+=(dpint)a[33]*b[23]; t+=(dpint)a[34]*b[22]; t+=(dpint)a[35]*b[21]; t+=(dpint)v21*(dpint)p35;  c[20]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[22]*b[35]; t+=(dpint)a[23]*b[34]; t+=(dpint)a[24]*b[33]; t+=(dpint)a[25]*b[32]; t+=(dpint)a[26]*b[31]; t+=(dpint)a[27]*b[30]; t+=(dpint)a[28]*b[29]; t+=(dpint)a[29]*b[28]; t+=(dpint)a[30]*b[27]; t+=(dpint)a[31]*b[26]; t+=(dpint)a[32]*b[25]; t+=(dpint)a[33]*b[24]; t+=(dpint)a[34]*b[23]; t+=(dpint)a[35]*b[22]; t+=(dpint)v22*(dpint)p35;  c[21]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[23]*b[35]; t+=(dpint)a[24]*b[34]; t+=(dpint)a[25]*b[33]; t+=(dpint)a[26]*b[32]; t+=(dpint)a[27]*b[31]; t+=(dpint)a[28]*b[30]; t+=(dpint)a[29]*b[29]; t+=(dpint)a[30]*b[28]; t+=(dpint)a[31]*b[27]; t+=(dpint)a[32]*b[26]; t+=(dpint)a[33]*b[25]; t+=(dpint)a[34]*b[24]; t+=(dpint)a[35]*b[23]; t+=(dpint)v23*(dpint)p35;  c[22]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[24]*b[35]; t+=(dpint)a[25]*b[34]; t+=(dpint)a[26]*b[33]; t+=(dpint)a[27]*b[32]; t+=(dpint)a[28]*b[31]; t+=(dpint)a[29]*b[30]; t+=(dpint)a[30]*b[29]; t+=(dpint)a[31]*b[28]; t+=(dpint)a[32]*b[27]; t+=(dpint)a[33]*b[26]; t+=(dpint)a[34]*b[25]; t+=(dpint)a[35]*b[24]; t+=(dpint)v24*(dpint)p35;  c[23]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[25]*b[35]; t+=(dpint)a[26]*b[34]; t+=(dpint)a[27]*b[33]; t+=(dpint)a[28]*b[32]; t+=(dpint)a[29]*b[31]; t+=(dpint)a[30]*b[30]; t+=(dpint)a[31]*b[29]; t+=(dpint)a[32]*b[28]; t+=(dpint)a[33]*b[27]; t+=(dpint)a[34]*b[26]; t+=(dpint)a[35]*b[25]; t+=(dpint)v25*(dpint)p35;  c[24]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[26]*b[35]; t+=(dpint)a[27]*b[34]; t+=(dpint)a[28]*b[33]; t+=(dpint)a[29]*b[32]; t+=(dpint)a[30]*b[31]; t+=(dpint)a[31]*b[30]; t+=(dpint)a[32]*b[29]; t+=(dpint)a[33]*b[28]; t+=(dpint)a[34]*b[27]; t+=(dpint)a[35]*b[26]; t+=(dpint)v26*(dpint)p35;  c[25]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[27]*b[35]; t+=(dpint)a[28]*b[34]; t+=(dpint)a[29]*b[33]; t+=(dpint)a[30]*b[32]; t+=(dpint)a[31]*b[31]; t+=(dpint)a[32]*b[30]; t+=(dpint)a[33]*b[29]; t+=(dpint)a[34]*b[28]; t+=(dpint)a[35]*b[27]; t+=(dpint)v27*(dpint)p35;  c[26]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[28]*b[35]; t+=(dpint)a[29]*b[34]; t+=(dpint)a[30]*b[33]; t+=(dpint)a[31]*b[32]; t+=(dpint)a[32]*b[31]; t+=(dpint)a[33]*b[30]; t+=(dpint)a[34]*b[29]; t+=(dpint)a[35]*b[28]; t+=(dpint)v28*(dpint)p35;  c[27]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[29]*b[35]; t+=(dpint)a[30]*b[34]; t+=(dpint)a[31]*b[33]; t+=(dpint)a[32]*b[32]; t+=(dpint)a[33]*b[31]; t+=(dpint)a[34]*b[30]; t+=(dpint)a[35]*b[29]; t+=(dpint)v29*(dpint)p35;  c[28]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[30]*b[35]; t+=(dpint)a[31]*b[34]; t+=(dpint)a[32]*b[33]; t+=(dpint)a[33]*b[32]; t+=(dpint)a[34]*b[31]; t+=(dpint)a[35]*b[30]; t+=(dpint)v30*(dpint)p35;  c[29]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[31]*b[35]; t+=(dpint)a[32]*b[34]; t+=(dpint)a[33]*b[33]; t+=(dpint)a[34]*b[32]; t+=(dpint)a[35]*b[31]; t+=(dpint)v31*(dpint)p35;  c[30]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[32]*b[35]; t+=(dpint)a[33]*b[34]; t+=(dpint)a[34]*b[33]; t+=(dpint)a[35]*b[32]; t+=(dpint)v32*(dpint)p35;  c[31]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[33]*b[35]; t+=(dpint)a[34]*b[34]; t+=(dpint)a[35]*b[33]; t+=(dpint)v33*(dpint)p35;  c[32]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[34]*b[35]; t+=(dpint)a[35]*b[34]; t+=(dpint)v34*(dpint)p35;  c[33]=((spint)t & mask);  t>>=29;
	t+=(dpint)a[35]*b[35]; t+=(dpint)v35*(dpint)p35;  c[34]=((spint)t & mask);  t>>=29;
	c[35] = (spint)t;
}

// Modular squaring, c=a*a  mod 2p
static void modsqr(const spint *a,spint *c) {
	dpint tot;
	dpint t=0;
	spint p35=0xe2u;
	spint q=((spint)1<<29u); // q is unsaturated radix 
	spint mask=(spint)(q-(spint)1);
	tot=(dpint)a[0]*a[0]; t=tot; spint v0=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[1]; tot*=2; t+=tot;  spint v1=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[2]; tot*=2; tot+=(dpint)a[1]*a[1]; t+=tot;  spint v2=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[3]; tot+=(dpint)a[1]*a[2]; tot*=2; t+=tot;  spint v3=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[4]; tot+=(dpint)a[1]*a[3]; tot*=2; tot+=(dpint)a[2]*a[2]; t+=tot;  spint v4=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[5]; tot+=(dpint)a[1]*a[4]; tot+=(dpint)a[2]*a[3]; tot*=2; t+=tot;  spint v5=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[6]; tot+=(dpint)a[1]*a[5]; tot+=(dpint)a[2]*a[4]; tot*=2; tot+=(dpint)a[3]*a[3]; t+=tot;  spint v6=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[7]; tot+=(dpint)a[1]*a[6]; tot+=(dpint)a[2]*a[5]; tot+=(dpint)a[3]*a[4]; tot*=2; t+=tot;  spint v7=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[8]; tot+=(dpint)a[1]*a[7]; tot+=(dpint)a[2]*a[6]; tot+=(dpint)a[3]*a[5]; tot*=2; tot+=(dpint)a[4]*a[4]; t+=tot;  spint v8=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[9]; tot+=(dpint)a[1]*a[8]; tot+=(dpint)a[2]*a[7]; tot+=(dpint)a[3]*a[6]; tot+=(dpint)a[4]*a[5]; tot*=2; t+=tot;  spint v9=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[10]; tot+=(dpint)a[1]*a[9]; tot+=(dpint)a[2]*a[8]; tot+=(dpint)a[3]*a[7]; tot+=(dpint)a[4]*a[6]; tot*=2; tot+=(dpint)a[5]*a[5]; t+=tot;  spint v10=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[11]; tot+=(dpint)a[1]*a[10]; tot+=(dpint)a[2]*a[9]; tot+=(dpint)a[3]*a[8]; tot+=(dpint)a[4]*a[7]; tot+=(dpint)a[5]*a[6]; tot*=2; t+=tot;  spint v11=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[12]; tot+=(dpint)a[1]*a[11]; tot+=(dpint)a[2]*a[10]; tot+=(dpint)a[3]*a[9]; tot+=(dpint)a[4]*a[8]; tot+=(dpint)a[5]*a[7]; tot*=2; tot+=(dpint)a[6]*a[6]; t+=tot;  spint v12=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[13]; tot+=(dpint)a[1]*a[12]; tot+=(dpint)a[2]*a[11]; tot+=(dpint)a[3]*a[10]; tot+=(dpint)a[4]*a[9]; tot+=(dpint)a[5]*a[8]; tot+=(dpint)a[6]*a[7]; tot*=2; t+=tot;  spint v13=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[14]; tot+=(dpint)a[1]*a[13]; tot+=(dpint)a[2]*a[12]; tot+=(dpint)a[3]*a[11]; tot+=(dpint)a[4]*a[10]; tot+=(dpint)a[5]*a[9]; tot+=(dpint)a[6]*a[8]; tot*=2; tot+=(dpint)a[7]*a[7]; t+=tot;  spint v14=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[15]; tot+=(dpint)a[1]*a[14]; tot+=(dpint)a[2]*a[13]; tot+=(dpint)a[3]*a[12]; tot+=(dpint)a[4]*a[11]; tot+=(dpint)a[5]*a[10]; tot+=(dpint)a[6]*a[9]; tot+=(dpint)a[7]*a[8]; tot*=2; t+=tot;  spint v15=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[16]; tot+=(dpint)a[1]*a[15]; tot+=(dpint)a[2]*a[14]; tot+=(dpint)a[3]*a[13]; tot+=(dpint)a[4]*a[12]; tot+=(dpint)a[5]*a[11]; tot+=(dpint)a[6]*a[10]; tot+=(dpint)a[7]*a[9]; tot*=2; tot+=(dpint)a[8]*a[8]; t+=tot;  spint v16=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[17]; tot+=(dpint)a[1]*a[16]; tot+=(dpint)a[2]*a[15]; tot+=(dpint)a[3]*a[14]; tot+=(dpint)a[4]*a[13]; tot+=(dpint)a[5]*a[12]; tot+=(dpint)a[6]*a[11]; tot+=(dpint)a[7]*a[10]; tot+=(dpint)a[8]*a[9]; tot*=2; t+=tot;  spint v17=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[18]; tot+=(dpint)a[1]*a[17]; tot+=(dpint)a[2]*a[16]; tot+=(dpint)a[3]*a[15]; tot+=(dpint)a[4]*a[14]; tot+=(dpint)a[5]*a[13]; tot+=(dpint)a[6]*a[12]; tot+=(dpint)a[7]*a[11]; tot+=(dpint)a[8]*a[10]; tot*=2; tot+=(dpint)a[9]*a[9]; t+=tot;  spint v18=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[19]; tot+=(dpint)a[1]*a[18]; tot+=(dpint)a[2]*a[17]; tot+=(dpint)a[3]*a[16]; tot+=(dpint)a[4]*a[15]; tot+=(dpint)a[5]*a[14]; tot+=(dpint)a[6]*a[13]; tot+=(dpint)a[7]*a[12]; tot+=(dpint)a[8]*a[11]; tot+=(dpint)a[9]*a[10]; tot*=2; t+=tot;  spint v19=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[20]; tot+=(dpint)a[1]*a[19]; tot+=(dpint)a[2]*a[18]; tot+=(dpint)a[3]*a[17]; tot+=(dpint)a[4]*a[16]; tot+=(dpint)a[5]*a[15]; tot+=(dpint)a[6]*a[14]; tot+=(dpint)a[7]*a[13]; tot+=(dpint)a[8]*a[12]; tot+=(dpint)a[9]*a[11]; tot*=2; tot+=(dpint)a[10]*a[10]; t+=tot;  spint v20=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[21]; tot+=(dpint)a[1]*a[20]; tot+=(dpint)a[2]*a[19]; tot+=(dpint)a[3]*a[18]; tot+=(dpint)a[4]*a[17]; tot+=(dpint)a[5]*a[16]; tot+=(dpint)a[6]*a[15]; tot+=(dpint)a[7]*a[14]; tot+=(dpint)a[8]*a[13]; tot+=(dpint)a[9]*a[12]; tot+=(dpint)a[10]*a[11]; tot*=2; t+=tot;  spint v21=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[22]; tot+=(dpint)a[1]*a[21]; tot+=(dpint)a[2]*a[20]; tot+=(dpint)a[3]*a[19]; tot+=(dpint)a[4]*a[18]; tot+=(dpint)a[5]*a[17]; tot+=(dpint)a[6]*a[16]; tot+=(dpint)a[7]*a[15]; tot+=(dpint)a[8]*a[14]; tot+=(dpint)a[9]*a[13]; tot+=(dpint)a[10]*a[12]; tot*=2; tot+=(dpint)a[11]*a[11]; t+=tot;  spint v22=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[23]; tot+=(dpint)a[1]*a[22]; tot+=(dpint)a[2]*a[21]; tot+=(dpint)a[3]*a[20]; tot+=(dpint)a[4]*a[19]; tot+=(dpint)a[5]*a[18]; tot+=(dpint)a[6]*a[17]; tot+=(dpint)a[7]*a[16]; tot+=(dpint)a[8]*a[15]; tot+=(dpint)a[9]*a[14]; tot+=(dpint)a[10]*a[13]; tot+=(dpint)a[11]*a[12]; tot*=2; t+=tot;  spint v23=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[24]; tot+=(dpint)a[1]*a[23]; tot+=(dpint)a[2]*a[22]; tot+=(dpint)a[3]*a[21]; tot+=(dpint)a[4]*a[20]; tot+=(dpint)a[5]*a[19]; tot+=(dpint)a[6]*a[18]; tot+=(dpint)a[7]*a[17]; tot+=(dpint)a[8]*a[16]; tot+=(dpint)a[9]*a[15]; tot+=(dpint)a[10]*a[14]; tot+=(dpint)a[11]*a[13]; tot*=2; tot+=(dpint)a[12]*a[12]; t+=tot;  spint v24=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[25]; tot+=(dpint)a[1]*a[24]; tot+=(dpint)a[2]*a[23]; tot+=(dpint)a[3]*a[22]; tot+=(dpint)a[4]*a[21]; tot+=(dpint)a[5]*a[20]; tot+=(dpint)a[6]*a[19]; tot+=(dpint)a[7]*a[18]; tot+=(dpint)a[8]*a[17]; tot+=(dpint)a[9]*a[16]; tot+=(dpint)a[10]*a[15]; tot+=(dpint)a[11]*a[14]; tot+=(dpint)a[12]*a[13]; tot*=2; t+=tot;  spint v25=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[26]; tot+=(dpint)a[1]*a[25]; tot+=(dpint)a[2]*a[24]; tot+=(dpint)a[3]*a[23]; tot+=(dpint)a[4]*a[22]; tot+=(dpint)a[5]*a[21]; tot+=(dpint)a[6]*a[20]; tot+=(dpint)a[7]*a[19]; tot+=(dpint)a[8]*a[18]; tot+=(dpint)a[9]*a[17]; tot+=(dpint)a[10]*a[16]; tot+=(dpint)a[11]*a[15]; tot+=(dpint)a[12]*a[14]; tot*=2; tot+=(dpint)a[13]*a[13]; t+=tot;  spint v26=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[27]; tot+=(dpint)a[1]*a[26]; tot+=(dpint)a[2]*a[25]; tot+=(dpint)a[3]*a[24]; tot+=(dpint)a[4]*a[23]; tot+=(dpint)a[5]*a[22]; tot+=(dpint)a[6]*a[21]; tot+=(dpint)a[7]*a[20]; tot+=(dpint)a[8]*a[19]; tot+=(dpint)a[9]*a[18]; tot+=(dpint)a[10]*a[17]; tot+=(dpint)a[11]*a[16]; tot+=(dpint)a[12]*a[15]; tot+=(dpint)a[13]*a[14]; tot*=2; t+=tot;  spint v27=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[28]; tot+=(dpint)a[1]*a[27]; tot+=(dpint)a[2]*a[26]; tot+=(dpint)a[3]*a[25]; tot+=(dpint)a[4]*a[24]; tot+=(dpint)a[5]*a[23]; tot+=(dpint)a[6]*a[22]; tot+=(dpint)a[7]*a[21]; tot+=(dpint)a[8]*a[20]; tot+=(dpint)a[9]*a[19]; tot+=(dpint)a[10]*a[18]; tot+=(dpint)a[11]*a[17]; tot+=(dpint)a[12]*a[16]; tot+=(dpint)a[13]*a[15]; tot*=2; tot+=(dpint)a[14]*a[14]; t+=tot;  spint v28=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[29]; tot+=(dpint)a[1]*a[28]; tot+=(dpint)a[2]*a[27]; tot+=(dpint)a[3]*a[26]; tot+=(dpint)a[4]*a[25]; tot+=(dpint)a[5]*a[24]; tot+=(dpint)a[6]*a[23]; tot+=(dpint)a[7]*a[22]; tot+=(dpint)a[8]*a[21]; tot+=(dpint)a[9]*a[20]; tot+=(dpint)a[10]*a[19]; tot+=(dpint)a[11]*a[18]; tot+=(dpint)a[12]*a[17]; tot+=(dpint)a[13]*a[16]; tot+=(dpint)a[14]*a[15]; tot*=2; t+=tot;  spint v29=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[30]; tot+=(dpint)a[1]*a[29]; tot+=(dpint)a[2]*a[28]; tot+=(dpint)a[3]*a[27]; tot+=(dpint)a[4]*a[26]; tot+=(dpint)a[5]*a[25]; tot+=(dpint)a[6]*a[24]; tot+=(dpint)a[7]*a[23]; tot+=(dpint)a[8]*a[22]; tot+=(dpint)a[9]*a[21]; tot+=(dpint)a[10]*a[20]; tot+=(dpint)a[11]*a[19]; tot+=(dpint)a[12]*a[18]; tot+=(dpint)a[13]*a[17]; tot+=(dpint)a[14]*a[16]; tot*=2; tot+=(dpint)a[15]*a[15]; t+=tot;  spint v30=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[31]; tot+=(dpint)a[1]*a[30]; tot+=(dpint)a[2]*a[29]; tot+=(dpint)a[3]*a[28]; tot+=(dpint)a[4]*a[27]; tot+=(dpint)a[5]*a[26]; tot+=(dpint)a[6]*a[25]; tot+=(dpint)a[7]*a[24]; tot+=(dpint)a[8]*a[23]; tot+=(dpint)a[9]*a[22]; tot+=(dpint)a[10]*a[21]; tot+=(dpint)a[11]*a[20]; tot+=(dpint)a[12]*a[19]; tot+=(dpint)a[13]*a[18]; tot+=(dpint)a[14]*a[17]; tot+=(dpint)a[15]*a[16]; tot*=2; t+=tot;  spint v31=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[32]; tot+=(dpint)a[1]*a[31]; tot+=(dpint)a[2]*a[30]; tot+=(dpint)a[3]*a[29]; tot+=(dpint)a[4]*a[28]; tot+=(dpint)a[5]*a[27]; tot+=(dpint)a[6]*a[26]; tot+=(dpint)a[7]*a[25]; tot+=(dpint)a[8]*a[24]; tot+=(dpint)a[9]*a[23]; tot+=(dpint)a[10]*a[22]; tot+=(dpint)a[11]*a[21]; tot+=(dpint)a[12]*a[20]; tot+=(dpint)a[13]*a[19]; tot+=(dpint)a[14]*a[18]; tot+=(dpint)a[15]*a[17]; tot*=2; tot+=(dpint)a[16]*a[16]; t+=tot;  spint v32=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[33]; tot+=(dpint)a[1]*a[32]; tot+=(dpint)a[2]*a[31]; tot+=(dpint)a[3]*a[30]; tot+=(dpint)a[4]*a[29]; tot+=(dpint)a[5]*a[28]; tot+=(dpint)a[6]*a[27]; tot+=(dpint)a[7]*a[26]; tot+=(dpint)a[8]*a[25]; tot+=(dpint)a[9]*a[24]; tot+=(dpint)a[10]*a[23]; tot+=(dpint)a[11]*a[22]; tot+=(dpint)a[12]*a[21]; tot+=(dpint)a[13]*a[20]; tot+=(dpint)a[14]*a[19]; tot+=(dpint)a[15]*a[18]; tot+=(dpint)a[16]*a[17]; tot*=2; t+=tot;  spint v33=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[34]; tot+=(dpint)a[1]*a[33]; tot+=(dpint)a[2]*a[32]; tot+=(dpint)a[3]*a[31]; tot+=(dpint)a[4]*a[30]; tot+=(dpint)a[5]*a[29]; tot+=(dpint)a[6]*a[28]; tot+=(dpint)a[7]*a[27]; tot+=(dpint)a[8]*a[26]; tot+=(dpint)a[9]*a[25]; tot+=(dpint)a[10]*a[24]; tot+=(dpint)a[11]*a[23]; tot+=(dpint)a[12]*a[22]; tot+=(dpint)a[13]*a[21]; tot+=(dpint)a[14]*a[20]; tot+=(dpint)a[15]*a[19]; tot+=(dpint)a[16]*a[18]; tot*=2; tot+=(dpint)a[17]*a[17]; t+=tot;  spint v34=((spint)t & mask); t>>=29;
	tot=(dpint)a[0]*a[35]; tot+=(dpint)a[1]*a[34]; tot+=(dpint)a[2]*a[33]; tot+=(dpint)a[3]*a[32]; tot+=(dpint)a[4]*a[31]; tot+=(dpint)a[5]*a[30]; tot+=(dpint)a[6]*a[29]; tot+=(dpint)a[7]*a[28]; tot+=(dpint)a[8]*a[27]; tot+=(dpint)a[9]*a[26]; tot+=(dpint)a[10]*a[25]; tot+=(dpint)a[11]*a[24]; tot+=(dpint)a[12]*a[23]; tot+=(dpint)a[13]*a[22]; tot+=(dpint)a[14]*a[21]; tot+=(dpint)a[15]*a[20]; tot+=(dpint)a[16]*a[19]; tot+=(dpint)a[17]*a[18]; tot*=2; t+=tot;  t+=(dpint)v0*p35;  spint v35=((spint)t & mask); t>>=29;
	tot=(dpint)a[1]*a[35]; tot+=(dpint)a[2]*a[34]; tot+=(dpint)a[3]*a[33]; tot+=(dpint)a[4]*a[32]; tot+=(dpint)a[5]*a[31]; tot+=(dpint)a[6]*a[30]; tot+=(dpint)a[7]*a[29]; tot+=(dpint)a[8]*a[28]; tot+=(dpint)a[9]*a[27]; tot+=(dpint)a[10]*a[26]; tot+=(dpint)a[11]*a[25]; tot+=(dpint)a[12]*a[24]; tot+=(dpint)a[13]*a[23]; tot+=(dpint)a[14]*a[22]; tot+=(dpint)a[15]*a[21]; tot+=(dpint)a[16]*a[20]; tot+=(dpint)a[17]*a[19]; tot*=2; tot+=(dpint)a[18]*a[18]; t+=tot;  t+=(dpint)v1*p35;  c[0]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[2]*a[35]; tot+=(dpint)a[3]*a[34]; tot+=(dpint)a[4]*a[33]; tot+=(dpint)a[5]*a[32]; tot+=(dpint)a[6]*a[31]; tot+=(dpint)a[7]*a[30]; tot+=(dpint)a[8]*a[29]; tot+=(dpint)a[9]*a[28]; tot+=(dpint)a[10]*a[27]; tot+=(dpint)a[11]*a[26]; tot+=(dpint)a[12]*a[25]; tot+=(dpint)a[13]*a[24]; tot+=(dpint)a[14]*a[23]; tot+=(dpint)a[15]*a[22]; tot+=(dpint)a[16]*a[21]; tot+=(dpint)a[17]*a[20]; tot+=(dpint)a[18]*a[19]; tot*=2; t+=tot;  t+=(dpint)v2*p35;  c[1]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[3]*a[35]; tot+=(dpint)a[4]*a[34]; tot+=(dpint)a[5]*a[33]; tot+=(dpint)a[6]*a[32]; tot+=(dpint)a[7]*a[31]; tot+=(dpint)a[8]*a[30]; tot+=(dpint)a[9]*a[29]; tot+=(dpint)a[10]*a[28]; tot+=(dpint)a[11]*a[27]; tot+=(dpint)a[12]*a[26]; tot+=(dpint)a[13]*a[25]; tot+=(dpint)a[14]*a[24]; tot+=(dpint)a[15]*a[23]; tot+=(dpint)a[16]*a[22]; tot+=(dpint)a[17]*a[21]; tot+=(dpint)a[18]*a[20]; tot*=2; tot+=(dpint)a[19]*a[19]; t+=tot;  t+=(dpint)v3*p35;  c[2]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[4]*a[35]; tot+=(dpint)a[5]*a[34]; tot+=(dpint)a[6]*a[33]; tot+=(dpint)a[7]*a[32]; tot+=(dpint)a[8]*a[31]; tot+=(dpint)a[9]*a[30]; tot+=(dpint)a[10]*a[29]; tot+=(dpint)a[11]*a[28]; tot+=(dpint)a[12]*a[27]; tot+=(dpint)a[13]*a[26]; tot+=(dpint)a[14]*a[25]; tot+=(dpint)a[15]*a[24]; tot+=(dpint)a[16]*a[23]; tot+=(dpint)a[17]*a[22]; tot+=(dpint)a[18]*a[21]; tot+=(dpint)a[19]*a[20]; tot*=2; t+=tot;  t+=(dpint)v4*p35;  c[3]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[5]*a[35]; tot+=(dpint)a[6]*a[34]; tot+=(dpint)a[7]*a[33]; tot+=(dpint)a[8]*a[32]; tot+=(dpint)a[9]*a[31]; tot+=(dpint)a[10]*a[30]; tot+=(dpint)a[11]*a[29]; tot+=(dpint)a[12]*a[28]; tot+=(dpint)a[13]*a[27]; tot+=(dpint)a[14]*a[26]; tot+=(dpint)a[15]*a[25]; tot+=(dpint)a[16]*a[24]; tot+=(dpint)a[17]*a[23]; tot+=(dpint)a[18]*a[22]; tot+=(dpint)a[19]*a[21]; tot*=2; tot+=(dpint)a[20]*a[20]; t+=tot;  t+=(dpint)v5*p35;  c[4]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[6]*a[35]; tot+=(dpint)a[7]*a[34]; tot+=(dpint)a[8]*a[33]; tot+=(dpint)a[9]*a[32]; tot+=(dpint)a[10]*a[31]; tot+=(dpint)a[11]*a[30]; tot+=(dpint)a[12]*a[29]; tot+=(dpint)a[13]*a[28]; tot+=(dpint)a[14]*a[27]; tot+=(dpint)a[15]*a[26]; tot+=(dpint)a[16]*a[25]; tot+=(dpint)a[17]*a[24]; tot+=(dpint)a[18]*a[23]; tot+=(dpint)a[19]*a[22]; tot+=(dpint)a[20]*a[21]; tot*=2; t+=tot;  t+=(dpint)v6*p35;  c[5]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[7]*a[35]; tot+=(dpint)a[8]*a[34]; tot+=(dpint)a[9]*a[33]; tot+=(dpint)a[10]*a[32]; tot+=(dpint)a[11]*a[31]; tot+=(dpint)a[12]*a[30]; tot+=(dpint)a[13]*a[29]; tot+=(dpint)a[14]*a[28]; tot+=(dpint)a[15]*a[27]; tot+=(dpint)a[16]*a[26]; tot+=(dpint)a[17]*a[25]; tot+=(dpint)a[18]*a[24]; tot+=(dpint)a[19]*a[23]; tot+=(dpint)a[20]*a[22]; tot*=2; tot+=(dpint)a[21]*a[21]; t+=tot;  t+=(dpint)v7*p35;  c[6]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[8]*a[35]; tot+=(dpint)a[9]*a[34]; tot+=(dpint)a[10]*a[33]; tot+=(dpint)a[11]*a[32]; tot+=(dpint)a[12]*a[31]; tot+=(dpint)a[13]*a[30]; tot+=(dpint)a[14]*a[29]; tot+=(dpint)a[15]*a[28]; tot+=(dpint)a[16]*a[27]; tot+=(dpint)a[17]*a[26]; tot+=(dpint)a[18]*a[25]; tot+=(dpint)a[19]*a[24]; tot+=(dpint)a[20]*a[23]; tot+=(dpint)a[21]*a[22]; tot*=2; t+=tot;  t+=(dpint)v8*p35;  c[7]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[9]*a[35]; tot+=(dpint)a[10]*a[34]; tot+=(dpint)a[11]*a[33]; tot+=(dpint)a[12]*a[32]; tot+=(dpint)a[13]*a[31]; tot+=(dpint)a[14]*a[30]; tot+=(dpint)a[15]*a[29]; tot+=(dpint)a[16]*a[28]; tot+=(dpint)a[17]*a[27]; tot+=(dpint)a[18]*a[26]; tot+=(dpint)a[19]*a[25]; tot+=(dpint)a[20]*a[24]; tot+=(dpint)a[21]*a[23]; tot*=2; tot+=(dpint)a[22]*a[22]; t+=tot;  t+=(dpint)v9*p35;  c[8]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[10]*a[35]; tot+=(dpint)a[11]*a[34]; tot+=(dpint)a[12]*a[33]; tot+=(dpint)a[13]*a[32]; tot+=(dpint)a[14]*a[31]; tot+=(dpint)a[15]*a[30]; tot+=(dpint)a[16]*a[29]; tot+=(dpint)a[17]*a[28]; tot+=(dpint)a[18]*a[27]; tot+=(dpint)a[19]*a[26]; tot+=(dpint)a[20]*a[25]; tot+=(dpint)a[21]*a[24]; tot+=(dpint)a[22]*a[23]; tot*=2; t+=tot;  t+=(dpint)v10*p35;  c[9]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[11]*a[35]; tot+=(dpint)a[12]*a[34]; tot+=(dpint)a[13]*a[33]; tot+=(dpint)a[14]*a[32]; tot+=(dpint)a[15]*a[31]; tot+=(dpint)a[16]*a[30]; tot+=(dpint)a[17]*a[29]; tot+=(dpint)a[18]*a[28]; tot+=(dpint)a[19]*a[27]; tot+=(dpint)a[20]*a[26]; tot+=(dpint)a[21]*a[25]; tot+=(dpint)a[22]*a[24]; tot*=2; tot+=(dpint)a[23]*a[23]; t+=tot;  t+=(dpint)v11*p35;  c[10]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[12]*a[35]; tot+=(dpint)a[13]*a[34]; tot+=(dpint)a[14]*a[33]; tot+=(dpint)a[15]*a[32]; tot+=(dpint)a[16]*a[31]; tot+=(dpint)a[17]*a[30]; tot+=(dpint)a[18]*a[29]; tot+=(dpint)a[19]*a[28]; tot+=(dpint)a[20]*a[27]; tot+=(dpint)a[21]*a[26]; tot+=(dpint)a[22]*a[25]; tot+=(dpint)a[23]*a[24]; tot*=2; t+=tot;  t+=(dpint)v12*p35;  c[11]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[13]*a[35]; tot+=(dpint)a[14]*a[34]; tot+=(dpint)a[15]*a[33]; tot+=(dpint)a[16]*a[32]; tot+=(dpint)a[17]*a[31]; tot+=(dpint)a[18]*a[30]; tot+=(dpint)a[19]*a[29]; tot+=(dpint)a[20]*a[28]; tot+=(dpint)a[21]*a[27]; tot+=(dpint)a[22]*a[26]; tot+=(dpint)a[23]*a[25]; tot*=2; tot+=(dpint)a[24]*a[24]; t+=tot;  t+=(dpint)v13*p35;  c[12]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[14]*a[35]; tot+=(dpint)a[15]*a[34]; tot+=(dpint)a[16]*a[33]; tot+=(dpint)a[17]*a[32]; tot+=(dpint)a[18]*a[31]; tot+=(dpint)a[19]*a[30]; tot+=(dpint)a[20]*a[29]; tot+=(dpint)a[21]*a[28]; tot+=(dpint)a[22]*a[27]; tot+=(dpint)a[23]*a[26]; tot+=(dpint)a[24]*a[25]; tot*=2; t+=tot;  t+=(dpint)v14*p35;  c[13]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[15]*a[35]; tot+=(dpint)a[16]*a[34]; tot+=(dpint)a[17]*a[33]; tot+=(dpint)a[18]*a[32]; tot+=(dpint)a[19]*a[31]; tot+=(dpint)a[20]*a[30]; tot+=(dpint)a[21]*a[29]; tot+=(dpint)a[22]*a[28]; tot+=(dpint)a[23]*a[27]; tot+=(dpint)a[24]*a[26]; tot*=2; tot+=(dpint)a[25]*a[25]; t+=tot;  t+=(dpint)v15*p35;  c[14]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[16]*a[35]; tot+=(dpint)a[17]*a[34]; tot+=(dpint)a[18]*a[33]; tot+=(dpint)a[19]*a[32]; tot+=(dpint)a[20]*a[31]; tot+=(dpint)a[21]*a[30]; tot+=(dpint)a[22]*a[29]; tot+=(dpint)a[23]*a[28]; tot+=(dpint)a[24]*a[27]; tot+=(dpint)a[25]*a[26]; tot*=2; t+=tot;  t+=(dpint)v16*p35;  c[15]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[17]*a[35]; tot+=(dpint)a[18]*a[34]; tot+=(dpint)a[19]*a[33]; tot+=(dpint)a[20]*a[32]; tot+=(dpint)a[21]*a[31]; tot+=(dpint)a[22]*a[30]; tot+=(dpint)a[23]*a[29]; tot+=(dpint)a[24]*a[28]; tot+=(dpint)a[25]*a[27]; tot*=2; tot+=(dpint)a[26]*a[26]; t+=tot;  t+=(dpint)v17*p35;  c[16]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[18]*a[35]; tot+=(dpint)a[19]*a[34]; tot+=(dpint)a[20]*a[33]; tot+=(dpint)a[21]*a[32]; tot+=(dpint)a[22]*a[31]; tot+=(dpint)a[23]*a[30]; tot+=(dpint)a[24]*a[29]; tot+=(dpint)a[25]*a[28]; tot+=(dpint)a[26]*a[27]; tot*=2; t+=tot;  t+=(dpint)v18*p35;  c[17]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[19]*a[35]; tot+=(dpint)a[20]*a[34]; tot+=(dpint)a[21]*a[33]; tot+=(dpint)a[22]*a[32]; tot+=(dpint)a[23]*a[31]; tot+=(dpint)a[24]*a[30]; tot+=(dpint)a[25]*a[29]; tot+=(dpint)a[26]*a[28]; tot*=2; tot+=(dpint)a[27]*a[27]; t+=tot;  t+=(dpint)v19*p35;  c[18]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[20]*a[35]; tot+=(dpint)a[21]*a[34]; tot+=(dpint)a[22]*a[33]; tot+=(dpint)a[23]*a[32]; tot+=(dpint)a[24]*a[31]; tot+=(dpint)a[25]*a[30]; tot+=(dpint)a[26]*a[29]; tot+=(dpint)a[27]*a[28]; tot*=2; t+=tot;  t+=(dpint)v20*p35;  c[19]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[21]*a[35]; tot+=(dpint)a[22]*a[34]; tot+=(dpint)a[23]*a[33]; tot+=(dpint)a[24]*a[32]; tot+=(dpint)a[25]*a[31]; tot+=(dpint)a[26]*a[30]; tot+=(dpint)a[27]*a[29]; tot*=2; tot+=(dpint)a[28]*a[28]; t+=tot;  t+=(dpint)v21*p35;  c[20]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[22]*a[35]; tot+=(dpint)a[23]*a[34]; tot+=(dpint)a[24]*a[33]; tot+=(dpint)a[25]*a[32]; tot+=(dpint)a[26]*a[31]; tot+=(dpint)a[27]*a[30]; tot+=(dpint)a[28]*a[29]; tot*=2; t+=tot;  t+=(dpint)v22*p35;  c[21]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[23]*a[35]; tot+=(dpint)a[24]*a[34]; tot+=(dpint)a[25]*a[33]; tot+=(dpint)a[26]*a[32]; tot+=(dpint)a[27]*a[31]; tot+=(dpint)a[28]*a[30]; tot*=2; tot+=(dpint)a[29]*a[29]; t+=tot;  t+=(dpint)v23*p35;  c[22]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[24]*a[35]; tot+=(dpint)a[25]*a[34]; tot+=(dpint)a[26]*a[33]; tot+=(dpint)a[27]*a[32]; tot+=(dpint)a[28]*a[31]; tot+=(dpint)a[29]*a[30]; tot*=2; t+=tot;  t+=(dpint)v24*p35;  c[23]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[25]*a[35]; tot+=(dpint)a[26]*a[34]; tot+=(dpint)a[27]*a[33]; tot+=(dpint)a[28]*a[32]; tot+=(dpint)a[29]*a[31]; tot*=2; tot+=(dpint)a[30]*a[30]; t+=tot;  t+=(dpint)v25*p35;  c[24]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[26]*a[35]; tot+=(dpint)a[27]*a[34]; tot+=(dpint)a[28]*a[33]; tot+=(dpint)a[29]*a[32]; tot+=(dpint)a[30]*a[31]; tot*=2; t+=tot;  t+=(dpint)v26*p35;  c[25]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[27]*a[35]; tot+=(dpint)a[28]*a[34]; tot+=(dpint)a[29]*a[33]; tot+=(dpint)a[30]*a[32]; tot*=2; tot+=(dpint)a[31]*a[31]; t+=tot;  t+=(dpint)v27*p35;  c[26]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[28]*a[35]; tot+=(dpint)a[29]*a[34]; tot+=(dpint)a[30]*a[33]; tot+=(dpint)a[31]*a[32]; tot*=2; t+=tot;  t+=(dpint)v28*p35;  c[27]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[29]*a[35]; tot+=(dpint)a[30]*a[34]; tot+=(dpint)a[31]*a[33]; tot*=2; tot+=(dpint)a[32]*a[32]; t+=tot;  t+=(dpint)v29*p35;  c[28]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[30]*a[35]; tot+=(dpint)a[31]*a[34]; tot+=(dpint)a[32]*a[33]; tot*=2; t+=tot;  t+=(dpint)v30*p35;  c[29]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[31]*a[35]; tot+=(dpint)a[32]*a[34]; tot*=2; tot+=(dpint)a[33]*a[33]; t+=tot;  t+=(dpint)v31*p35;  c[30]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[32]*a[35]; tot+=(dpint)a[33]*a[34]; tot*=2; t+=tot;  t+=(dpint)v32*p35;  c[31]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[33]*a[35]; tot*=2; tot+=(dpint)a[34]*a[34]; t+=tot;  t+=(dpint)v33*p35;  c[32]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[34]*a[35]; tot*=2; t+=tot;  t+=(dpint)v34*p35;  c[33]=((spint)t & mask);  t>>=29;
	tot=(dpint)a[35]*a[35]; t+=tot;  t+=(dpint)v35*p35;  c[34]=((spint)t & mask);  t>>=29;
	c[35] = (spint)t;
}

//copy
static void modcpy(const spint *a,spint *c) {
	int i;
	for (i=0;i<36;i++) {
		c[i]=a[i];
	}
}

//square n times
static void modnsqr(spint *a,int n) {
	int i;
	for (i=0;i<n;i++) {
		modsqr(a,a);
	}
}

//Calculate progenitor
static void modpro(const spint *w,spint *z) {
	spint x[36];
	spint t0[36];
	spint t1[36];
	spint t2[36];
	spint t3[36];
	spint t4[36];
	spint t5[36];
	spint t6[36];
	spint t7[36];
	modcpy(w,x);
	modsqr(x,t1);
	modmul(x,t1,t0);
	modmul(t1,t0,z);
	modmul(t1,z,t1);
	modsqr(t1,t2);
	modmul(t0,t2,t0);
	modmul(z,t0,z);
	modsqr(z,t2);
	modmul(t0,t2,t0);
	modmul(z,t0,z);
	modsqr(z,t2);
	modcpy(t2,t3);
	modnsqr(t3,2);
	modmul(t2,t3,t3);
	modsqr(t3,t2);
	modmul(z,t2,t2);
	modsqr(t2,t4);
	modmul(t3,t4,t3);
	modsqr(t3,t4);
	modcpy(t4,t5);
	modnsqr(t5,2);
	modmul(t4,t5,t5);
	modmul(t3,t5,t5);
	modmul(t4,t5,t4);
	modsqr(t4,t6);
	modmul(t4,t6,t6);
	modsqr(t6,t6);
	modmul(t4,t6,t6);
	modnsqr(t6,2);
	modmul(t4,t6,t6);
	modmul(t5,t6,t5);
	modsqr(t5,t6);
	modcpy(t6,t7);
	modnsqr(t7,2);
	modmul(t6,t7,t6);
	modnsqr(t6,3);
	modmul(t4,t6,t4);
	modcpy(t4,t6);
	modnsqr(t6,3);
	modmul(t5,t6,t5);
	modmul(t4,t5,t4);
	modsqr(t4,t6);
	modmul(t5,t6,t5);
	modcpy(t5,t6);
	modnsqr(t6,3);
	modmul(t5,t6,t6);
	modmul(t4,t6,t4);
	modsqr(t4,t6);
	modmul(t5,t6,t5);
	modnsqr(t5,23);
	modmul(t4,t5,t4);
	modmul(t2,t4,t2);
	modmul(t3,t2,t3);
	modsqr(t3,t4);
	modmul(t3,t4,t4);
	modmul(t2,t4,t2);
	modmul(t3,t2,t3);
	modmul(t2,t3,t2);
	modsqr(t2,t4);
	modmul(t2,t4,t4);
	modmul(t3,t4,t3);
	modnsqr(t3,57);
	modmul(t2,t3,t2);
	modmul(t0,t2,t0);
	modmul(z,t0,z);
	modmul(t1,z,t1);
	modsqr(t1,t2);
	modmul(t1,t2,t3);
	modnsqr(t3,2);
	modmul(t2,t3,t2);
	modmul(t0,t2,t0);
	modmul(z,t0,z);
	modsqr(z,t2);
	modmul(z,t2,t2);
	modsqr(t2,t2);
	modmul(z,t2,t2);
	modmul(t1,t2,t1);
	modmul(t0,t1,t0);
	modnsqr(t1,128);
	modmul(t0,t1,t1);
	modnsqr(t1,128);
	modmul(t0,t1,t1);
	modnsqr(t1,128);
	modmul(t0,t1,t1);
	modnsqr(t1,128);
	modmul(t0,t1,t1);
	modnsqr(t1,128);
	modmul(t0,t1,t1);
	modnsqr(t1,128);
	modmul(t0,t1,t0);
	modnsqr(t0,125);
	modmul(z,t0,z);
}

//calculate inverse, provide progenitor h if available
static void modinv(const spint *x,const spint *h,spint *z) {
	spint s[36];
	spint t[36];
	if (h==NULL) {
		modpro(x,t);
	} else {
		modcpy(h,t);
	}
	modcpy(x,s);
	modnsqr(t,2);
	modmul(s,t,z);
}

//Convert m to n-residue form, n=nres(m) 
static void nres(const spint *m,spint *n) {
	const spint c[36]={0xb2fe560u,0x121fe090u,0x90fdbc0u,0x487ede0u,0x243f6f0u,0x121fb78u,0x90fdbcu,0x487edeu,0x10243f6fu,0x18121fb7u,0x1c090fdbu,0x1e0487edu,0xf0243f6u,0x178121fbu,0x1bc090fdu,0xde0487eu,0x16f0243fu,0x1b78121fu,0x1dbc090fu,0x1ede0487u,0x1f6f0243u,0x1fb78121u,0xfdbc090u,0x7ede048u,0x3f6f024u,0x1fb7812u,0x10fdbc09u,0x87ede04u,0x43f6f02u,0x121fb781u,0x90fdbc0u,0x487ede0u,0x243f6f0u,0x121fb78u,0x90fdbcu,0x1eu};
	modmul(m,c,n);
}

//Convert n back to normal form, m=redc(n) 
static void redc(const spint *n,spint *m) {
	int i;
	spint c[36];
	c[0]=1;
	for (i=1;i<36;i++) {
		c[i]=0;
	}
	modmul(n,c,m);
	(void)modfsb(m);
}

//is unity?
static int modis1(const spint *a) {
	int i;
	spint c[36];
	spint c0;
	spint d=0;
	redc(a,c);
	for (i=1;i<36;i++) {
		d|=c[i];
	}
	c0=(spint)c[0];
	return ((spint)1 & ((d-(spint)1)>>29u) & (((c0^(spint)1)-(spint)1)>>29u));
}

//is zero?
static int modis0(const spint *a) {
	int i;
	spint c[36];
	spint d=0;
	redc(a,c);
	for (i=0;i<36;i++) {
		d|=c[i];
	}
	return ((spint)1 & ((d-(spint)1)>>29u));
}

//set to zero
static void modzer(spint *a) {
	int i;
	for (i=0;i<36;i++) {
		a[i]=0;
	}
}

//set to one
static void modone(spint *a) {
	int i;
	a[0]=1;
	for (i=1;i<36;i++) {
		a[i]=0;
	}
	nres(a,a);
}

//set to integer
static void modint(int x,spint *a) {
	int i;
	a[0]=(spint)x;
	for (i=1;i<36;i++) {
		a[i]=0;
	}
	nres(a,a);
}

// Modular multiplication by an integer, c=a*b mod 2p
// uses special method for trinomials, otherwise Barrett-Dhem reduction
inline static void modmli(const spint *a, int b, spint *c) {
	spint t[36];
	modint(b, t);
	modmul(a, t, c);
}

//Test for quadratic residue 
static int modqr(const spint *h,const spint *x) {
	spint r[36];
	if (h==NULL) {
		modpro(x,r);
		modsqr(r,r);
	} else {
		modsqr(h,r);
	}
	modmul(r,x,r);
	return modis1(r) | modis0(x);
}

//conditional move g to f if d=1
//strongly recommend inlining be disabled using compiler specific syntax
static void __attribute__ ((noinline)) modcmv(int b,const spint *g,volatile spint *f) {
	int i;
	spint c0,c1,s,t,w,aux;
	static spint R=0;
	R+=0x5aa5a55au;
	w=R;
	c0=(~b)&(w+1);
	c1=b+w;
	for (i=0;i<36;i++) {
		s=g[i]; t=f[i];
		f[i] = aux = c0*t+c1*s;
		f[i] = aux - w*(t+s);
	}
}

//conditional swap g and f if d=1
//strongly recommend inlining be disabled using compiler specific syntax
static void __attribute__ ((noinline)) modcsw(int b,volatile spint *g,volatile spint *f) {
	int i;
	spint c0,c1,s,t,w,v,aux;
	static spint R=0;
	R+=0x5aa5a55au;
	w=R;
	c0=(~b)&(w+1);
	c1=b+w;
	for (i=0;i<36;i++) {
		s=g[i]; t=f[i];
		v=w*(t+s);
		f[i] = aux = c0*t+c1*s;
		f[i] = aux - v;
		g[i] = aux = c0*s+c1*t;
		g[i] = aux - v;
	}
}

//Modular square root, provide progenitor h if available, NULL if not
static void modsqrt(const spint *x,const spint *h,spint *r) {
	spint s[36];
	spint y[36];
	if (h==NULL) {
		modpro(x,y);
	} else {
		modcpy(h,y);
	}
	modmul(y,x,s);
	modcpy(s,r);
}

//shift left by less than a word
static void modshl(unsigned int n,spint *a) {
	int i;
	a[35]=((a[35]<<n)) + (a[34]>>(29u-n));
	for (i=34;i>0;i--) {
		a[i]=((a[i]<<n)&(spint)0x1fffffff) + (a[i-1]>>(29u-n));
	}
	a[0]=(a[0]<<n)&(spint)0x1fffffff;
}

//shift right by less than a word. Return shifted out part
static int modshr(unsigned int n,spint *a) {
	int i;
	spint r=a[0]&(((spint)1<<n)-(spint)1);
	for (i=0;i<35;i++) {
		a[i]=(a[i]>>n) + ((a[i+1]<<(29u-n))&(spint)0x1fffffff);
	}
	a[35]=a[35]>>n;
	return r;
}

//divide by 2. Shift right 1 bit (or add p and shift right one bit)
static void modhaf(spint *n) {
	int lsb;
	spint t[36];
	(void)prop(n);
	modcpy(n,t);
	lsb=modshr(1,t);
	n[0]-=(spint)1;
	n[35]+=((spint)0xe2u);
	(void)prop(n);
	modshr(1,n);
	modcmv(1-lsb,t,n);
}

//set a= 2^r
static void mod2r(unsigned int r,spint *a) {
	unsigned int n=r/29u;
	unsigned int m=r%29u;
	modzer(a);
	if (r>=128*8) return;
	a[n]=1; a[n]<<=m;
nres(a,a);
}

//export to byte array
static void modexp(const spint *a,char *b) {
	int i;
	spint c[36];
	redc(a,c);
	for (i=127;i>=0;i--) {
		b[i]=c[0]&(spint)0xff;
		(void)modshr(8,c);
	}
}

//import from byte array
//returns 1 if in range, else 0
static int modimp(const char *b, spint *a) {
	int i,res;
	for (i=0;i<36;i++) {
		a[i]=0;
	}
	for (i=0;i<128;i++) {
		modshl(8,a);
		a[0]+=(spint)(unsigned char)b[i];
	}
	res=(int)modfsb(a);
	nres(a,a);
	return res;
}

//determine sign
static int modsign(const spint *a) {
	spint c[36];
	redc(a,c);
	return c[0]%2;
}

//return true if equal
static int modcmp(const spint *a,const spint *b) {
	spint c[36],d[36];
	int i,eq=1;
	redc(a,c);
	redc(b,d);
	for (i=0;i<36;i++) {
		eq&=(((c[i]^d[i])-1)>>29)&1;
	}
	return eq;
}

// clang-format on
/******************************************************************************
 API functions calling generated code above
 ******************************************************************************/

#include <fp.h>

const digit_t ZERO[NWORDS_FIELD] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
const digit_t ONE[NWORDS_FIELD] = { 0x00243f6f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002 };
// Montgomery representation of 2^-1
static const digit_t TWO_INV[NWORDS_FIELD] = { 0x00121fb7, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000072 };
// Montgomery representation of 3^-1
static const digit_t THREE_INV[NWORDS_FIELD] = { 0x0ab6bfcf, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x15555555, 0x0aaaaaaa, 0x00000097 };
// Montgomery representation of 2^1024
static const digit_t R2[NWORDS_FIELD] = { 0x1fc120b2, 0x1fb78121, 0x0fdbc090, 0x07ede048, 0x03f6f024, 0x01fb7812, 0x10fdbc09, 0x087ede04, 0x043f6f02, 0x121fb781, 0x090fdbc0, 0x0487ede0, 0x0243f6f0, 0x0121fb78, 0x0090fdbc, 0x00487ede, 0x10243f6f, 0x18121fb7, 0x1c090fdb, 0x1e0487ed, 0x0f0243f6, 0x178121fb, 0x1bc090fd, 0x0de0487e, 0x16f0243f, 0x1b78121f, 0x1dbc090f, 0x1ede0487, 0x1f6f0243, 0x1fb78121, 0x0fdbc090, 0x07ede048, 0x03f6f024, 0x01fb7812, 0x10fdbc09, 0x000000e0 };

void
fp_set_small(fp_t *x, const digit_t val)
{
    modint((int)val, *x);
}

void
fp_mul_small(fp_t *x, const fp_t *a, const uint32_t val)
{
    modmli(*a, (int)val, *x);
}

void
fp_set_zero(fp_t *x)
{
    modzer(*x);
}

void
fp_set_one(fp_t *x)
{
    modone(*x);
}

uint32_t
fp_is_equal(const fp_t *a, const fp_t *b)
{
    return -(uint32_t)modcmp(*a, *b);
}

uint32_t
fp_is_zero(const fp_t *a)
{
    return -(uint32_t)modis0(*a);
}

void
fp_copy(fp_t *out, const fp_t *a)
{
    modcpy(*a, *out);
}

void
fp_cswap(fp_t *a, fp_t *b, uint32_t ctl)
{
    modcsw((int)(ctl & 0x1), *a, *b);
}

void
fp_add(fp_t *out, const fp_t *a, const fp_t *b)
{
    modadd(*a, *b, *out);
}

void
fp_sub(fp_t *out, const fp_t *a, const fp_t *b)
{
    modsub(*a, *b, *out);
}

void
fp_neg(fp_t *out, const fp_t *a)
{
    modneg(*a, *out);
}

void
fp_sqr(fp_t *out, const fp_t *a)
{
    modsqr(*a, *out);
}

void
fp_mul(fp_t *out, const fp_t *a, const fp_t *b)
{
    modmul(*a, *b, *out);
}

void
fp_inv(fp_t *x)
{
    modinv(*x, NULL, *x);
}

uint32_t
fp_is_square(const fp_t *a)
{
    return -(uint32_t)modqr(NULL, *a);
}

void
fp_sqrt(fp_t *a)
{
    modsqrt(*a, NULL, *a);
}

void
fp_half(fp_t *out, const fp_t *a)
{
    modmul(TWO_INV, *a, *out);
}

void
fp_exp3div4(fp_t *out, const fp_t *a)
{
    modpro(*a, *out);
}

void
fp_div3(fp_t *out, const fp_t *a)
{
    modmul(THREE_INV, *a, *out);
}

void
fp_encode(void *dst, const fp_t *a)
{
    // Modified version of modexp()
    int i;
    spint c[36];
    redc(*a, c);
    for (i = 0; i < 128; i++) {
        ((char *)dst)[i] = c[0] & (spint)0xff;
        (void)modshr(8, c);
    }
}

uint32_t
fp_decode(fp_t *d, const void *src)
{
    // Modified version of modimp()
    int i;
    spint res;
    const unsigned char *b = src;
    for (i = 0; i < 36; i++) {
        (*d)[i] = 0;
    }
    for (i = 127; i >= 0; i--) {
        modshl(8, *d);
        (*d)[0] += (spint)b[i];
    }
    res = (spint)-modfsb(*d);
    nres(*d, *d);
    // If the value was canonical then res = -1; otherwise, res = 0
    for (i = 0; i < 36; i++) {
        (*d)[i] &= res;
    }
    return (uint32_t)res;
}

static inline unsigned char
add_carry(unsigned char cc, spint a, spint b, spint *d)
{
    udpint t = (udpint)a + (udpint)b + cc;
    *d = (spint)t;
    return (unsigned char)(t >> Wordlength);
}

static void
partial_reduce(spint *out, const spint *src)
{
    spint h, l, quo, rem;
    unsigned char cc;

    h = src[31] >> 24;
    l = src[31] & 0x00ffffff;
    // 113*2^1016 = 1 mod q; add floor(h/113) + (h mod 113)*2^1016 to the low part.
    quo = (h * 0x91) >> 14;
    rem = h - (113 * quo);
    cc = add_carry(0, src[0], quo, &out[0]);
    cc = add_carry(cc, src[1], 0, &out[1]);
    cc = add_carry(cc, src[2], 0, &out[2]);
    cc = add_carry(cc, src[3], 0, &out[3]);
    cc = add_carry(cc, src[4], 0, &out[4]);
    cc = add_carry(cc, src[5], 0, &out[5]);
    cc = add_carry(cc, src[6], 0, &out[6]);
    cc = add_carry(cc, src[7], 0, &out[7]);
    cc = add_carry(cc, src[8], 0, &out[8]);
    cc = add_carry(cc, src[9], 0, &out[9]);
    cc = add_carry(cc, src[10], 0, &out[10]);
    cc = add_carry(cc, src[11], 0, &out[11]);
    cc = add_carry(cc, src[12], 0, &out[12]);
    cc = add_carry(cc, src[13], 0, &out[13]);
    cc = add_carry(cc, src[14], 0, &out[14]);
    cc = add_carry(cc, src[15], 0, &out[15]);
    cc = add_carry(cc, src[16], 0, &out[16]);
    cc = add_carry(cc, src[17], 0, &out[17]);
    cc = add_carry(cc, src[18], 0, &out[18]);
    cc = add_carry(cc, src[19], 0, &out[19]);
    cc = add_carry(cc, src[20], 0, &out[20]);
    cc = add_carry(cc, src[21], 0, &out[21]);
    cc = add_carry(cc, src[22], 0, &out[22]);
    cc = add_carry(cc, src[23], 0, &out[23]);
    cc = add_carry(cc, src[24], 0, &out[24]);
    cc = add_carry(cc, src[25], 0, &out[25]);
    cc = add_carry(cc, src[26], 0, &out[26]);
    cc = add_carry(cc, src[27], 0, &out[27]);
    cc = add_carry(cc, src[28], 0, &out[28]);
    cc = add_carry(cc, src[29], 0, &out[29]);
    cc = add_carry(cc, src[30], 0, &out[30]);
    (void)add_carry(cc, l, rem << 24, &out[31]);
}

// Little-endian encoding of a 32-bit integer.
static inline void
enc32le(void *dst, uint32_t x)
{
    uint8_t *buf = dst;
    buf[0] = (uint8_t)x;         buf[1] = (uint8_t)(x >> 8);
    buf[2] = (uint8_t)(x >> 16); buf[3] = (uint8_t)(x >> 24);
}

static inline uint32_t
dec32le(const void *src)
{
    const uint8_t *buf = src;
    return (spint)buf[0] | ((spint)buf[1] << 8) | ((spint)buf[2] << 16) | ((spint)buf[3] << 24);
}

void
fp_decode_reduce(fp_t *d, const void *src, size_t len)
{
    uint32_t t[32];
    uint8_t tmp[128]; // Nbytes
    const uint8_t *b = src;

    fp_set_zero(d);
    if (len == 0) {
        return;
    }

    size_t rem = len % 128;
    if (rem != 0) {
        size_t k = len - rem;
        memcpy(tmp, b + k, len - k);
        memset(tmp + len - k, 0, (sizeof tmp) - (len - k));
        fp_decode(d, tmp);
        len = k;
    }
    while (len > 0) {
        fp_mul(d, d, &R2);
        len -= 128;
        t[0] = dec32le(b + len);
        t[1] = dec32le(b + len + 4);
        t[2] = dec32le(b + len + 8);
        t[3] = dec32le(b + len + 12);
        t[4] = dec32le(b + len + 16);
        t[5] = dec32le(b + len + 20);
        t[6] = dec32le(b + len + 24);
        t[7] = dec32le(b + len + 28);
        t[8] = dec32le(b + len + 32);
        t[9] = dec32le(b + len + 36);
        t[10] = dec32le(b + len + 40);
        t[11] = dec32le(b + len + 44);
        t[12] = dec32le(b + len + 48);
        t[13] = dec32le(b + len + 52);
        t[14] = dec32le(b + len + 56);
        t[15] = dec32le(b + len + 60);
        t[16] = dec32le(b + len + 64);
        t[17] = dec32le(b + len + 68);
        t[18] = dec32le(b + len + 72);
        t[19] = dec32le(b + len + 76);
        t[20] = dec32le(b + len + 80);
        t[21] = dec32le(b + len + 84);
        t[22] = dec32le(b + len + 88);
        t[23] = dec32le(b + len + 92);
        t[24] = dec32le(b + len + 96);
        t[25] = dec32le(b + len + 100);
        t[26] = dec32le(b + len + 104);
        t[27] = dec32le(b + len + 108);
        t[28] = dec32le(b + len + 112);
        t[29] = dec32le(b + len + 116);
        t[30] = dec32le(b + len + 120);
        t[31] = dec32le(b + len + 124);
        partial_reduce(t, t);
        enc32le(tmp, t[0]);
        enc32le(tmp + 4, t[1]);
        enc32le(tmp + 8, t[2]);
        enc32le(tmp + 12, t[3]);
        enc32le(tmp + 16, t[4]);
        enc32le(tmp + 20, t[5]);
        enc32le(tmp + 24, t[6]);
        enc32le(tmp + 28, t[7]);
        enc32le(tmp + 32, t[8]);
        enc32le(tmp + 36, t[9]);
        enc32le(tmp + 40, t[10]);
        enc32le(tmp + 44, t[11]);
        enc32le(tmp + 48, t[12]);
        enc32le(tmp + 52, t[13]);
        enc32le(tmp + 56, t[14]);
        enc32le(tmp + 60, t[15]);
        enc32le(tmp + 64, t[16]);
        enc32le(tmp + 68, t[17]);
        enc32le(tmp + 72, t[18]);
        enc32le(tmp + 76, t[19]);
        enc32le(tmp + 80, t[20]);
        enc32le(tmp + 84, t[21]);
        enc32le(tmp + 88, t[22]);
        enc32le(tmp + 92, t[23]);
        enc32le(tmp + 96, t[24]);
        enc32le(tmp + 100, t[25]);
        enc32le(tmp + 104, t[26]);
        enc32le(tmp + 108, t[27]);
        enc32le(tmp + 112, t[28]);
        enc32le(tmp + 116, t[29]);
        enc32le(tmp + 120, t[30]);
        enc32le(tmp + 124, t[31]);
        fp_t a;
        fp_decode(&a, tmp);
        fp_add(d, d, &a);
    }
}
