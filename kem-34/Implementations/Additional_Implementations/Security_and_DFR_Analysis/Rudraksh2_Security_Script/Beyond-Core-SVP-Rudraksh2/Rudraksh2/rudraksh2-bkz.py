from sage.all import *
load("../framework/instance_gen.sage")

verbosity = 2


print("============= Rudraksh2-I-128")
n = 576 # l*n = 9*64=576
m = 576
q = 3329
D_s = build_centered_binomial_law(2)
D_e = D_s

_, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
                                          m, D_e, D_s)#, verbosity=verbosity)

# inst.integrate_q_vectors(q, report_every=20)
print(" Attack Estimation via GSA + Interesect model ")
inst.estimate_attack()
print(" Attack Estimation via simulation + probabilistic model ")

inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)

# print("============= Rudraksh2-II-128")
# n = 576 # l*n = 9*64=576
# m = 576
# q = 4001
# D_s = build_centered_binomial_law(1)
# D_e = D_s

# _, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
#                                           m, D_e, D_s)#, verbosity=verbosity)

# # inst.integrate_q_vectors(q, report_every=20)
# print(" Attack Estimation via GSA + Interesect model ")
# inst.estimate_attack()
# print(" Attack Estimation via simulation + probabilistic model ")

# inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)


# print("============= Rudraksh2-I-256")
# n = 1152 # l*n = 9*128=1152
# m = 1152
# q = 3329
# D_s = build_centered_binomial_law(1)
# D_e = D_s

# _, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
#                                           m, D_e, D_s)#, verbosity=verbosity)

# # inst.integrate_q_vectors(q, report_every=20)
# print(" Attack Estimation via GSA + Interesect model ")
# inst.estimate_attack()
# print(" Attack Estimation via simulation + probabilistic model ")

# inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)


# print("============= Rudraksh2-II-256")
# n = 1152 # l*n = 9*128=1152
# m = 1152
# q = 4001
# D_s = build_centered_binomial_law(1)
# D_e = D_s

# _, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
#                                           m, D_e, D_s)#, verbosity=verbosity)

# # inst.integrate_q_vectors(q, report_every=20)
# print(" Attack Estimation via GSA + Interesect model ")
# inst.estimate_attack()
# print(" Attack Estimation via simulation + probabilistic model ")

# inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)



# print("============= Rudraksh2-I-512")
# n = 2048 # l*n = 8*256=2048
# m = 2048
# q = 7681
# D_s = build_centered_binomial_law(2)
# D_e = D_s

# _, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
#                                           m, D_e, D_s)#, verbosity=verbosity)

# # inst.integrate_q_vectors(q, report_every=20)
# print(" Attack Estimation via GSA + Interesect model ")
# inst.estimate_attack()
# print(" Attack Estimation via simulation + probabilistic model ")

# inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)


# print("============= Rudraksh2-II-512")
# n = 2304 # l*n = 9*256=2304
# m = 2304
# q = 4001
# D_s = build_centered_binomial_law(1)
# D_e = D_s

# _, _, inst = initialize_from_LWE_instance(DBDD_predict_diag, n, q,
#                                           m, D_e, D_s)#, verbosity=verbosity)

# # inst.integrate_q_vectors(q, report_every=20)
# print(" Attack Estimation via GSA + Interesect model ")
# inst.estimate_attack()
# print(" Attack Estimation via simulation + probabilistic model ")

# inst.estimate_attack(probabilistic=True, lift_union_bound=True, silent=False)


