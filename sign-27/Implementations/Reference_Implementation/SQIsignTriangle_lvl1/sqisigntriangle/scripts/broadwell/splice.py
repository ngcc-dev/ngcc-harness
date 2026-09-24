import re,sys
src=open('out_lvl2/gf9309.c').read()
frag=open('safegcd_lvl2.c').read()

def remove_func(text, sig):
    i=text.find(sig)
    assert i>=0, 'not found: '+sig
    # find first { after sig, then match braces
    j=text.find('{', i); depth=0; k=j
    while k<len(text):
        if text[k]=='{': depth+=1
        elif text[k]=='}':
            depth-=1
            if depth==0: break
        k+=1
    # remove [i, k+1) plus trailing newline
    end=k+1
    while end<len(text) and text[end]=='\n': end+=1
    return text[:i]+text[end:]

# remove Fermat invert + legendre
src=remove_func(src,'uint32_t %s_invert('%'gf9309')
src=remove_func(src,'int32_t %s_legendre('%'gf9309')
# remove EXP_INV / EXP_LEG declarations
src=re.sub(r'^static const uint64_t EXP_INV\[\d+\][^\n]*\n','',src,flags=re.M)
src=re.sub(r'^static const uint64_t EXP_LEG\[\d+\][^\n]*\n','',src,flags=re.M)

# splice the safegcd fragment right before gf9309_div3 (so INVT/helpers precede users)
anchor='void %s_div3('%'gf9309'
i=src.find(anchor); assert i>=0
src=src[:i]+frag+'\n'+src[i:]

open('out_lvl2/gf9309.c','w').write(src)
print('spliced. gf9309.c now %d lines'%src.count(chr(10)))
# sanity: invert/legendre present once (safegcd), EXP_INV gone
print('gf9309_invert count:', src.count('gf9309_invert('))
print('gf9309_legendre count:', src.count('gf9309_legendre('))
print('EXP_INV present:', 'EXP_INV' in src, ' EXP_SQRT present:', 'EXP_SQRT' in src)
print('gf9309_div present:', 'gf9309_div(' in src, ' gf9309_lin present:', 'gf9309_lin(' in src)
