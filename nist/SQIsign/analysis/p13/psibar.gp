\\ mean of psi(s)/s, weighted by s, over B-smooth s <= N (B = N^(1/u)); also sqrt-weighted
N = 2*10^6;
{
for (k = 2, 8,
  u = k/2.; B = floor(N^(1/u));
  sw = 0.; spsi = 0.; cnt = 0;
  forfactored(F = 1, N,
    s = F[1]; f = F[2];
    if (s == 1 || vecmax(f[,1]) <= B,
      r = 1.; for (t = 1, #f[,1], r *= (1 + 1/f[t,1]));
      sw += s; spsi += s*r; cnt++));
  print("u=", u, " B=", B, " smooth_frac=", cnt/N*1., " psibar=", spsi/sw));
}
