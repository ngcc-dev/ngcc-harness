from cost import *


# Result of the DDGR20 BKZ prediction
#
# ======= Scabbard-128
# GSA Intersect:             dim=1153      δ=1.003743     β=437.24 
# Probabilistic simulation:  dim=1153                     β=446.29
# ======= Scabbard-256
# GSA Intersect:             dim=2305      δ=1.002185     β=915.13  
# Probabilistic simulation:  dim=2305                     β=938.46
# ======= Scabbard-512
# GSA Intersect:             dim=4097      δ=1.001309     β=1778.84  
# Probabilistic simulation:  dim= -                       β= -





print("           \t & n  \t&  β \t& β' \t& gates \t& memory ")
print("Scabbard-128  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(1153, 446))
print("Scabbard-256  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(2305, 938))
print("Scabbard-512  \t & %d\t& %d\t& %d\t& %.1f \t& %.1f "%summary(4097, 1779))




#                             & n    &  β    & β'    & gates         & memory 
# Scabbard-128     & 1153 & 446   & 406   & 161.0         & 100.4 
# Scabbard-256     & 2305 & 938   & 870   & 299.8         & 198.3 
# Scabbard-512     & 4097 & 1779  & 1668  & 534.1         & 365.3 
