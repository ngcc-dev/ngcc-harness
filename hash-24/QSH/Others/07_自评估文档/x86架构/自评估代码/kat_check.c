/* Known-answer KAT checker for QSH (guideline 1-2 functional test).
 *
 * Reads a KAT file in the literal-message format (Msg_Len / Msg / Dst_Len / Dst),
 * recomputes each digest with the linked CryptHash, and compares it to the stored
 * Dst.  Used for KAT_2_12 (4097 vectors: every message bit-length 0..4096, i.e.
 * all padding / sub-byte edge cases) and KAT_Loop.  The seed-expanded large-message
 * KATs (2_23 / 2_33) are NOT in this literal format and are checked via the official
 * KAT_CryptHash harness instead.
 *
 *   gcc <FLAGS> -I<impl> kat_check.c <impl>/CryptHash_AlgorithmInstance.c -o kat_check
 *   ./kat_check Test_Vectors/KAT_2_12_QSH-512.txt
 *
 * Prints "<file>: matched M/N" and exits non-zero if any vector mismatches.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);

static int hexval(int c){ if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10; return -1; }

/* parse a "Key = VALUE" line; returns pointer to VALUE (after "= "), or NULL */
static char *val_after(char *line, const char *key){
    size_t kl=strlen(key);
    if(strncmp(line,key,kl)!=0) return NULL;
    char *p=line+kl; while(*p==' '||*p=='=') p++; return p;
}

int main(int argc, char **argv){
    if(argc<2){ fprintf(stderr,"usage: %s <kat_file>\n",argv[0]); return 2; }
    FILE *f=fopen(argv[1],"r"); if(!f){ fprintf(stderr,"cannot open %s\n",argv[1]); return 2; }

    size_t cap=1u<<16; char *line=(char*)malloc(cap);
    unsigned char msg[4096], dst_exp[256], dst_got[256];
    unsigned long long msg_len=0; int dst_len=0, have_msg=0, have_dlen=0;
    long total=0, matched=0; int firstfail=1, rc=0;

    while(getline(&line,&cap,f)!=-1){
        char *v;
        if((v=val_after(line,"Msg_Len"))){ msg_len=strtoull(v,NULL,10); have_msg=0; }
        else if((v=val_after(line,"Msg_Seed"))||((v=val_after(line,"Msg_Exp")))){
            fprintf(stderr,"%s: seed-expanded format -- use the official harness for this KAT\n",argv[1]);
            fclose(f); free(line); return 3;
        }
        else if(!strncmp(line,"Msg",3) && (line[3]==' '||line[3]=='=')){
            v=line+3; while(*v==' '||*v=='=') v++;
            size_t nb=0; while(hexval(v[0])>=0 && hexval(v[1])>=0 && nb<sizeof(msg)){ msg[nb++]=(unsigned char)((hexval(v[0])<<4)|hexval(v[1])); v+=2; }
            have_msg=1;
        }
        else if((v=val_after(line,"Dst_Len"))){ dst_len=atoi(v); have_dlen=1; }
        else if(!strncmp(line,"Dst",3) && (line[3]==' '||line[3]=='=')){
            v=line+3; while(*v==' '||*v=='=') v++;
            size_t nb=0; while(hexval(v[0])>=0 && hexval(v[1])>=0 && nb<sizeof(dst_exp)){ dst_exp[nb++]=(unsigned char)((hexval(v[0])<<4)|hexval(v[1])); v+=2; }
            if(have_msg && have_dlen){
                total++;
                CryptHash(dst_len, msg, msg_len, dst_got);
                if(memcmp(dst_got,dst_exp,(size_t)dst_len/8)==0) matched++;
                else { rc=1; if(firstfail){ fprintf(stderr,"  first mismatch at Msg_Len=%llu (Dst_Len=%d)\n",msg_len,dst_len); firstfail=0; } }
            }
            have_msg=have_dlen=0;
        }
    }
    printf("%s: matched %ld/%ld\n", argv[1], matched, total);
    fclose(f); free(line);
    return rc;
}
