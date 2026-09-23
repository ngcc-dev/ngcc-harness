# NGCC uniform build: hash-25 TaiChi (reference implementation)
NGCC_ID   := hash-25
NGCC_TYPE := hash
NGCC_ALG  := TaiChi
include ../api/link_rules.mk

REF := TaiChi/Implementations/Reference_Implementation
NGCC_KAT_DIR := TaiChi/Test_Vectors

# CryptHash_AlgorithmInstance.c + drng.c (identical to api/drng.c); default glob.
# The KAT_*_TaiChi-*-op-per/op-res.txt files belong to the Optimized_Implementation
# trees and are not exercised here; Others/ (self-evaluation) is ignored.
$(eval $(call ngcc_instance,TaiChi-512,$(REF)/TaiChi-512))
$(eval $(call ngcc_instance,TaiChi-768,$(REF)/TaiChi-768))
$(eval $(call ngcc_instance,TaiChi-1024,$(REF)/TaiChi-1024))

include ../api/link_finish.mk

.PHONY: exploit
exploit: lib/libTaiChi-512.so
	$(MAKE) -C ../security hash_oom_false_success
	../security/hash_oom_false_success lib/libTaiChi-512.so taichi hash-25-1
