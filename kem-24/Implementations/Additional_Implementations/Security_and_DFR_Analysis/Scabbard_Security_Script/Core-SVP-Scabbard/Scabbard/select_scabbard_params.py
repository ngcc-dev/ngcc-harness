#/*---------------------------------------------------------------------
#This file has been adapted from the implementation
#(available at, Public Domain https://github.com/pq-crystals/kyber)
#of "CRYSTALS - Kyber: a CCA-secure module-lattice-based KEM"
#by : Joppe Bos, Leo Ducas, Eike Kiltz, Tancrede Lepoint,
#Vadim Lyubashevsky, John M. Schanck, Peter Schwabe & Damien stehle
#----------------------------------------------------------------------*/ 

import numpy as np
import matplotlib.pyplot as plt
from math import sqrt, exp, log, floor
from proba_util import *
from sage.all import *


load("../framework/instance_gen.sage")

# core SVP cost models
cost_model_c = lambda beta: 0.292 * beta
cost_model_q = lambda beta: 0.265 * beta

# estimate cost using leaky-LWE-Estimator https://eprint.iacr.org/2020/292.pdf
def estimate(n, q, p, m, ks):
    D_s = build_centered_binomial_law(ks)
    D_e = build_mod_switching_error_law(q, p)
    A, b, dbdd = initialize_from_LWE_instance(DBDD_predict, m*n, q, m*n, D_e, D_s, verbosity=0)
    _ = dbdd.integrate_q_vectors(q, report_every=100)
    beta, delta = dbdd.estimate_attack()
    print('-- security --')
    print('quantum: ', round(cost_model_q(beta)), 'classical: ', round(cost_model_c(beta)))
    # return beta


def main():
    # search_params(binomial coins, q, k, n, log2(classical_security), log2(failure probability)
    # script looks for optimal p and t
    # parameter for binomial distribution in Kyber is (binomial coins)/2

    # Low security
    print('-Scabbard_low-')
    estimate(n = 64, q = 2**14, p = 2**10, m = 9, ks = 2) 

    # Medium security
    # print('-Scabbard_medium-')
    # estimate(n = 128, q = 2**13, p = 2**11, m = 9, ks = 2) 


    # High security
    # print('-Scabbard_high-')
    # estimate(n = 256, q = 2**13, p = 2**11, m = 8, ks = 2) 



if __name__ == '__main__':
    main()
