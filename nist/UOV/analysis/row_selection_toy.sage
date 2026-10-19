# SageMath witness for the partial divided-power rank assumption in UOV-4.
#
# The default GF(256) planted-UOV instance is a counterexample to the proposed
# partial-tower Hilbert series. Optional arguments are e v a m trials seed.

import itertools
import random
import sys
from math import comb


values = [8, 4, 10, 1, 3, 20261018]
for index, value in enumerate(sys.argv[1:7]):
    values[index] = int(value)
e, vinegar, oil, equations, trials, seed = values
n = vinegar + oil
field = GF(2**e, "z")
subsets = list(itertools.combinations(range(n), oil))
column = {item: index for index, item in enumerate(subsets)}
columns = len(subsets)
set_random_seed(seed)
random.seed(int(seed))


def convolve(left, right, maximum):
    return [sum(left[i] * right[d - i] for i in range(d + 1))
            for d in range(maximum + 1)]


def predicted_dimension(level):
    base = [comb(n, d) for d in range(oil + 1)]
    if level == 0:
        factor = [0] * (oil + 1)
        for j in range(oil // 2 + 1):
            factor[2 * j] = (-1)**j * comb(equations + j - 1, j)
        coefficients = convolve(base, factor, oil)
    else:
        numerator = [0] * (oil + 1)
        for j in range(oil // 2 + 1):
            numerator[2 * j] = (-1)**j * comb(equations, j)
        first = convolve(base, numerator, oil)
        coefficients = [
            sum(comb(equations + j - 1, j) * first[d - 8 * j]
                for j in range(d // 8 + 1))
            for d in range(oil + 1)
        ]
    return 1 if any(value <= 0 for value in coefficients) else max(coefficients[oil], 1)


def planted_instance():
    transform = random_matrix(field, n, n)
    while not transform.is_invertible():
        transform = random_matrix(field, n, n)
    forms = []
    for _ in range(equations):
        form = matrix(field, n, n)
        for i in range(n):
            for j in range(i + 1, n):
                if i >= vinegar and j >= vinegar:
                    continue
                form[i, j] = field.random_element()
                form[j, i] = form[i, j]
        forms.append(transform.transpose() * form * transform)
    oil_basis = transform.inverse()[:, vinegar:]
    return forms, oil_basis


def plain_rows(form):
    rows = []
    for fixed in itertools.combinations(range(n), oil - 2):
        fixed_set = set(fixed)
        row = {}
        for i, j in itertools.combinations(
                [x for x in range(n) if x not in fixed_set], 2):
            if form[i, j]:
                row[column[tuple(sorted(fixed + (i, j)))]] = form[i, j]
        rows.append(row)
    return rows


def divided_square_rows(form):
    rows = []
    for fixed in itertools.combinations(range(n), oil - 4):
        fixed_set = set(fixed)
        row = {}
        rest = [x for x in range(n) if x not in fixed_set]
        for i, j, k, ell in itertools.combinations(rest, 4):
            value = (form[i, j] * form[k, ell]
                     + form[i, k] * form[j, ell]
                     + form[i, ell] * form[j, k])
            if value:
                row[column[tuple(sorted(fixed + (i, j, k, ell)))]] = value
        rows.append(row)
    return rows


def as_matrix(rows):
    result = matrix(field, len(rows), columns, sparse=True)
    for i, row in enumerate(rows):
        for j, value in row.items():
            result[i, j] = value
    return result


h0 = predicted_dimension(0)
h1 = predicted_dimension(1)
print(f"GF(2^{e}) (v,a,m)=({vinegar},{oil},{equations}) N={columns} "
      f"predicted nullities h0={h0} h1={h1}; "
      f"strict filter {equations * (oil - 1)} > {2 * vinegar}")

observed = []
for trial in range(trials):
    forms, basis = planted_instance()
    level0 = [row for form in forms for row in plain_rows(form)]
    level1 = [row for form in forms for row in divided_square_rows(form)]
    plain = as_matrix(level0)
    combined = as_matrix(level0 + level1)
    plain_rank = plain.rank()
    combined_rank = combined.rank()
    pluecker = vector(field, [
        basis.matrix_from_rows(list(indices)).det() for indices in subsets
    ])
    if combined * pluecker != 0:
        raise AssertionError("planted oil Pluecker vector is not in the kernel")
    observed.append((plain_rank, combined_rank))
    print(f"trial {trial}: plain rank={plain_rank} (pred {columns-h0}); "
          f"plain+div-square rank={combined_rank} (pred {columns-h1}); "
          f"kernel dimension={columns-combined_rank}")

if (e, vinegar, oil, equations) == (8, 4, 10, 1):
    if not all(item == (895, 959) for item in observed):
        raise AssertionError(f"default counterexample ranks changed: {observed}")
    print("PASS: partial-tower prediction is false for the default planted-UOV instance")
else:
    print("OBSERVATION: compare measured ranks with the partial-tower prediction")
