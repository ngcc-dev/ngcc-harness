#ifndef HASH_HPP
#define HASH_HPP
#include "block.hpp"
#include "parameters.hpp"
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <cstdlib>
extern "C" {
#include "auxfunc.h"
}
namespace faest {
struct hash_state {
    uint8_t* buf = nullptr;
    size_t buf_len = 0, buf_cap = 0;
    bool finalized = false;
    uint8_t* out = nullptr;
    size_t out_have = 0, out_used = 0, out_cap = 0;
    bool is_prg = false;
    unsigned secpar_bits = 0;
    hash_state() {}
    ~hash_state() { free(buf); free(out); }
    hash_state(const hash_state& o) { copy_from(o); }
    hash_state(hash_state&& o) noexcept : buf(o.buf),buf_len(o.buf_len),buf_cap(o.buf_cap),finalized(o.finalized),out(o.out),out_have(o.out_have),out_used(o.out_used),out_cap(o.out_cap),is_prg(o.is_prg),secpar_bits(o.secpar_bits) { o.buf=nullptr;o.out=nullptr;o.buf_cap=0;o.out_cap=0; }
    hash_state& operator=(const hash_state& o) { free(buf);free(out);copy_from(o);return *this; }
    hash_state& operator=(hash_state&& o) noexcept { free(buf);free(out);buf=o.buf;buf_len=o.buf_len;buf_cap=o.buf_cap;finalized=o.finalized;out=o.out;out_have=o.out_have;out_used=o.out_used;out_cap=o.out_cap;is_prg=o.is_prg;secpar_bits=o.secpar_bits;o.buf=nullptr;o.out=nullptr;o.buf_cap=0;o.out_cap=0;return *this; }
    void copy_from(const hash_state& o) { buf_len=o.buf_len;buf_cap=o.buf_cap;finalized=o.finalized;out_have=o.out_have;out_used=o.out_used;out_cap=o.out_cap;is_prg=o.is_prg;secpar_bits=o.secpar_bits;buf=(uint8_t*)malloc(buf_cap);if(buf&&o.buf)memcpy(buf,o.buf,buf_len);out=(uint8_t*)malloc(out_cap);if(out&&o.out)memcpy(out,o.out,out_have); }
    inline int init(secpar s) { secpar_bits=(unsigned)s; free(buf);free(out);buf=(uint8_t*)malloc(256);buf_len=0;buf_cap=256;finalized=false;out=nullptr;out_have=0;out_used=0;out_cap=0;is_prg=false;return 0; }
    inline int init_prg(secpar s) { init(s); is_prg=true; return 0; }
    inline int update(const void* input, size_t bytes) { if(!finalized){if(buf_len+bytes>buf_cap){buf_cap=(buf_len+bytes)*2;buf=(uint8_t*)realloc(buf,buf_cap);}memcpy(buf+buf_len,input,bytes);buf_len+=bytes;}return 0; }
    inline int update_byte(uint8_t b) { return this->update(&b,1); }
    /* pseudohash size for hash domains: smallest supported digest with output >= 2*secpar */
    inline int ph_size() const { if(secpar_bits<=256)return 512; if(secpar_bits<=384)return 768; return 1024; }
    /* generate output via counter-separated pseudohash calls */
    inline void gen_hash(size_t need_total) {
        if(out_cap<need_total){out=(uint8_t*)realloc(out,need_total);out_cap=need_total;}
        int phsz=ph_size(); int phb=phsz/8; unsigned ctr=0; size_t gen=0;
        while(gen<need_total) {
            size_t blen=buf_len; uint8_t* tmp=(uint8_t*)malloc(blen+4);
            memcpy(tmp,buf,blen);
            tmp[blen]=(uint8_t)ctr;tmp[blen+1]=(uint8_t)(ctr>>8);tmp[blen+2]=(uint8_t)(ctr>>16);tmp[blen+3]=(uint8_t)(ctr>>24);
            uint8_t block[128];
            pseudohash(phsz,tmp,(unsigned long long)(blen+4)*8,block);
            free(tmp);
            size_t to_copy=need_total-gen; if(to_copy>(size_t)phb)to_copy=phb;
            memcpy(out+gen,block,to_copy); gen+=to_copy; ctr++;
        }
        out_have=need_total;
    }
    inline void gen_prg(size_t need_total) {
        if(out_cap<need_total){out=(uint8_t*)realloc(out,need_total);out_cap=need_total;}
        pseudoXOF((unsigned long long)need_total*8,buf,(unsigned long long)buf_len*8,out);
        out_have=need_total;
    }
    inline int finalize(void* digest, size_t bytes) {
        if(!finalized)finalized=true;
        if(bytes>0) {
            if(is_prg) pseudoXOF((unsigned long long)bytes*8,buf,(unsigned long long)buf_len*8,(uint8_t*)digest);
            else { gen_hash(bytes); memcpy(digest,out,bytes); out_used=bytes; }
        }
        return 0;
    }
    inline void squeeze(void* d, size_t bytes) {
        if(!finalized)finalized=true;
        size_t need=out_used+bytes;
        if(need>out_have) {
            if(is_prg) gen_prg(need);
            else gen_hash(need);
        }
        memcpy(d,out+out_used,bytes); out_used+=bytes;
    }
};
struct hash_state_x4 {
    hash_state instances[4];
    hash_state_x4() = default;
    hash_state_x4(const hash_state_x4& o) = default;
    hash_state_x4(hash_state_x4&& o) noexcept = default;
    hash_state_x4& operator=(const hash_state_x4& o) = default;
    hash_state_x4& operator=(hash_state_x4&& o) noexcept = default;
    inline void init(secpar s) { for(int i=0;i<4;++i)instances[i].init(s); }
    inline void update(const void** data, size_t sz) { for(int i=0;i<4;++i)instances[i].update(data[i],sz); }
    inline void update_4(const void* d0,const void* d1,const void* d2,const void* d3,size_t sz) { const void* d[4]={d0,d1,d2,d3};this->update(d,sz); }
    inline void update_1(const void* d, size_t sz) { this->update_4(d,d,d,d,sz); }
    inline void update_1_byte(uint8_t b) { this->update_1(&b,1); }
    inline void init_prefix(secpar s, const uint8_t p) { this->init(s); this->update_1(&p,1); }
    inline void finalize(void** b, size_t n) { for(int i=0;i<4;++i)instances[i].finalize(b[i],n); }
    inline void finalize_4(void* b0,void* b1,void* b2,void* b3, size_t n) { void* b[4]={b0,b1,b2,b3};this->finalize(b,n); }
    inline void squeeze(void** b, size_t n) { for(int i=0;i<4;++i)instances[i].squeeze(b[i],n); }
    inline void squeeze_4(void* b0,void* b1,void* b2,void* b3, size_t n) { void* b[4]={b0,b1,b2,b3};this->squeeze(b,n); }
};
template<secpar S, typename IV> inline void shake_prg_impl(const block_secpar<S>* __restrict__ keys, const IV& iv, const uint32_t& tweak, size_t num_keys, size_t num_bytes, uint8_t* __restrict__ output) {
    for(size_t i=0;i<num_keys;++i) { hash_state h; h.init_prg(S); h.update(&keys[i],sizeof(keys[i])); h.update(&iv,sizeof(iv)); h.update(&tweak,sizeof(tweak)); h.update_byte(0); h.finalize(output+i*num_bytes,num_bytes); }
}
}
#endif
