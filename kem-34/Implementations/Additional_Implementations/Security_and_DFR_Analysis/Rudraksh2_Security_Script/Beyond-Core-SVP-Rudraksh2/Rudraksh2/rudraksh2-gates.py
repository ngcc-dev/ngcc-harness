from cost import *


# Result of the DDGR20 BKZ prediction
#
# ======= Rudraksh2-I-128
# GSA Intersect:             dim=1153      δ=1.003719     β=441.36 
# Probabilistic simulation:  dim=1153                     β=450
# ======= Rudraksh2-II-128
# GSA Intersect:             dim=1153      δ=1.004049     β=390.62  
# Probabilistic simulation:  dim=1153                     β=398.73
# ======= Rudraksh2-I-256
# GSA Intersect:             dim=2305      δ=1.002166     β=925.47  
# Probabilistic simulation:  dim=2305                     β=949.08
# ======= Rudraksh2-II-256
# GSA Intersect:             dim=2305      δ=1.002202     β=905.79 
# Probabilistic simulation:  dim=2305                     β=928.85
# ======= Rudraksh2-I-512
# GSA Intersect:             dim=3722      δ=1.001324     β=1753.12 
# Probabilistic simulation:  dim= -                       β= -
# ======= Rudraksh2-II-512
# GSA Intersect:             dim=3982      δ=1.001206     β=1972.39 
# Probabilistic simulation:  dim= -                       β= -




print("           \t & n  \t&  β \t& β' \t& gates \t& memory ")
print("Rudraksh2-I-128  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(1153, 450))
print("Rudraksh2-II-128  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(1153, 399))
print("Rudraksh2-I-256  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(2305, 949))
print("Rudraksh2-II-256  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(2305, 929))
print("Rudraksh2-I-512  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(3722, 1753))
print("Rudraksh2-II-512  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(3982, 1972))



#                             & n    &  β    & β'    & gates         & memory 
# Rudraksh2-I-128          & 1153 & 450   & 410   & 162.2         & 101.2 
# Rudraksh2-II-128         & 1153 & 398   & 361   & 147.6         & 90.8 
# Rudraksh2-I-256          & 2305 & 949   & 881   & 303.0         & 200.6
# Rudraksh2-II-256         & 2305 & 929   & 862   & 297.4         & 196.6 
# Rudraksh2-I-512          & 3722 & 1753  & 1644  & 526.9         & 360.3 
# Rudraksh2-II-512         & 3982 & 1972  & 1852  & 587.4         & 403.7
