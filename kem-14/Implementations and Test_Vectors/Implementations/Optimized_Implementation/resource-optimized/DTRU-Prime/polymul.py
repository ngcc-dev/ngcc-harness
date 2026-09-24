def poly_mul_q1(a, b, DTRU_N, DTRU_Q):
    temp = [0] * (2 * DTRU_N - 1)
    c = [0] * DTRU_N

    for i in range(DTRU_N):
        for j in range(DTRU_N):
            temp[i + j] += (a[i] * b[j]) % DTRU_Q

    for i in range(DTRU_N, 2 * DTRU_N - 1):
        temp[i - DTRU_N] += temp[i]
        temp[i - DTRU_N + 1] += temp[i]

    for i in range(DTRU_N):
        c[i] = temp[i] % DTRU_Q
    return c

if __name__ == "__main__":
    DTRU_N = 1087
    DTRU_Q = 2017

    with open("pke.txt", "r") as f:
        lines = f.readlines()
        f = [int(x) for x in lines[1].strip().split(',') if x.strip()]
        finv = [int(x) for x in lines[3].strip().split(',') if x.strip()]
        g = [int(x) for x in lines[5].strip().split(',') if x.strip()]
        h = [int(x) for x in lines[7].strip().split(',') if x.strip()]

    c = poly_mul_q1(f, finv, DTRU_N, DTRU_Q)

    print("Resultant polynomial coefficients:")
    print(c)