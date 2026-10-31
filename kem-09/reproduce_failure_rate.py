#!/usr/bin/env python3
"""kem-09-5: exact per-coefficient CheetahKEM decryption-noise law and DFR,
with and without the public-key rounding term e_pk (requires numpy)."""
# Exact per-coefficient decryption-noise law for CheetahKEM (code-faithful compression).
import numpy as np, math, sys
from math import comb
Q=7681
class D:
    def __init__(s, lo, p): s.lo=lo; s.p=np.array(p,dtype=np.float64)
    def conv(a,b): return D(a.lo+b.lo, np.convolve(a.p,b.p))
    def trim(s, eps=2.0**-400):
        nz=np.nonzero(s.p>eps)[0]; a,b=nz[0],nz[-1]
        return D(s.lo+a, s.p[a:b+1])
def cbd(eta): return D(-eta,[comb(2*eta,k+eta)/4**eta for k in range(-eta,eta+1)])
def unif(lo,hi): return D(lo,[1/(hi-lo+1)]*(hi-lo+1))
def prod(X,Y):
    d={}
    for i,px in enumerate(X.p):
        for j,py in enumerate(Y.p):
            v=(X.lo+i)*(Y.lo+j); d[v]=d.get(v,0)+px*py
    lo=min(d); hi=max(d); p=[d.get(v,0) for v in range(lo,hi+1)]
    return D(lo,p)
def power(X,n):
    R=None; B=X
    while n:
        if n&1: R=B if R is None else R.conv(B).trim()
        n>>=1
        if n: B=B.conv(B).trim()
    return R
def tail(X,t):  # P(|X| >= t)
    v=np.arange(X.lo,X.lo+len(X.p)); return X.p[np.abs(v)>=t].sum()
def run(name,k,eta,db,du,dv,lam,with_epk=True):
    N=640
    sb,su,sv=13-db,13-du,13-dv
    epk=unif(-(2**(sb-1))+1, 2**(sb-1)) if with_epk else D(0,[1.0])
    eu =unif(-(2**(su-1))+1, 2**(su-1))
    ev =unif(-(2**(sv-1))+1, 2**(sv-1))
    c=cbd(eta)
    T1=power(prod(c.conv(epk),c), k*N)
    T2=power(prod(c.conv(eu),c), k*N)
    Z=T1.conv(T2).trim().conv(c).conv(ev).trim()
    # failure iff |2*noise + m| > (Q-1)/2 ; for m=0: |noise|>=1921 ; m=1: 2noise+1 in [-3840,3840] -> noise in [-1920,1919]
    v=np.arange(Z.lo,Z.lo+len(Z.p))
    p0=Z.p[np.abs(2*v)>(Q-1)//2].sum(); p1=Z.p[np.abs(2*v+1)>(Q-1)//2].sum()
    pc=(p0+p1)/2
    var=(Z.p*v*v).sum()
    print(f"{name} epk={with_epk}: sigma={math.sqrt(var):.1f} per-coef log2 p={math.log2(pc):.2f}  union over {lam}: log2 DFR={math.log2(pc*lam):.2f}",flush=True)
for args in (("Cheetah128",1,5,10,10,4,128),("Cheetah256",2,4,10,10,4,256),("Cheetah384",3,3,11,11,4,384),("Cheetah512",4,2,11,11,8,512)):
    run(*args,with_epk=False); run(*args,with_epk=True)
