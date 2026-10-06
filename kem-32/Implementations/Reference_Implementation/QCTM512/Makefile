CC = gcc
CFLAGS = -W -Wall -O3
OPENSSL_PREFIX ?= $(firstword $(wildcard /opt/homebrew/opt/openssl@3 /opt/homebrew/opt/openssl /usr/local/opt/openssl@3 /usr/local/opt/openssl))
CPPFLAGS ?=
ifeq ($(OPENSSL_PREFIX),)
LDFLAGS ?= -lssl -lcrypto
else
CPPFLAGS += -I$(OPENSSL_PREFIX)/include
LDFLAGS ?= -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
endif
ALGORITHM = QCTM512
KAT_TOOL = $(ALGORITHM)_KAT
API_PKC_KAT_TOOL = $(ALGORITHM)_API_PKC_KAT
API_PKC_COUNT_TOOL = $(ALGORITHM)_API_PKC_KAT_Count
API_PKC_ZIP = ../../../Supporting_Documentation/API_PKC.zip
API_PKC_TEMPLATE_DIR = API_PKC/Implementations/Reference_Implementation/AlgorithmInstance
API_PKC_KAT_DIR = .api-pkc-kat
API_PKC_TEMPLATE_STAMP = $(API_PKC_KAT_DIR)/.templates-extracted
REQUEST_RESPONSE_KAT_TOOL = $(ALGORITHM)_Request_Response_KAT
TRACE_REPORT_TOOL = qctm512_trace_report
SPEED_TOOL = qctm512_speed
VERIFY_PUBLIC_T_TOOL = verify_public_t
AUDIT_MATGEN_TOOL = audit_matgen
ROUNDTRIP_TOOL = hybrid_roundtrip
RECOMPUTE_PK_TOOL = recompute_public_key

OBJECTS = keccak.o kem.o m2e.o permutation.o gf.o poly.o goppa.o seeded_keygen.o qctm512_kem_api.o decode.o roots.o

.SECONDARY:

.PHONY: force all clean veryclean trace-report roundtrip verify-decode-map verify-recompute-pk verify-public-t audit-matgen api-pkc-kat api-pkc-kat-count speed

all: $(KAT_TOOL)

api-pkc-kat: $(API_PKC_KAT_TOOL)

api-pkc-kat-count: $(API_PKC_COUNT_TOOL)


$(KAT_TOOL): $(OBJECTS) rng.o qctm512_kat.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(API_PKC_TEMPLATE_STAMP): $(API_PKC_ZIP)
	/bin/mkdir -p $(API_PKC_KAT_DIR)
	unzip -j -o -q $(API_PKC_ZIP) $(API_PKC_TEMPLATE_DIR)/KAT_KEM.c $(API_PKC_TEMPLATE_DIR)/drng.c $(API_PKC_TEMPLATE_DIR)/drng.h $(API_PKC_TEMPLATE_DIR)/auxfunc.c $(API_PKC_TEMPLATE_DIR)/auxfunc.h -d $(API_PKC_KAT_DIR)
	touch $@

$(API_PKC_KAT_DIR)/KAT_KEM.o: $(API_PKC_TEMPLATE_STAMP) KEM_AlgorithmInstance.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(API_PKC_KAT_DIR) -c $(API_PKC_KAT_DIR)/KAT_KEM.c -o $@

$(API_PKC_KAT_DIR)/drng.o: $(API_PKC_TEMPLATE_STAMP)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(API_PKC_KAT_DIR) -c $(API_PKC_KAT_DIR)/drng.c -o $@

$(API_PKC_KAT_DIR)/KEM_AlgorithmInstance.o: KEM_AlgorithmInstance.c KEM_AlgorithmInstance.h scheme_api.h sizes.h rng.h $(API_PKC_TEMPLATE_STAMP)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DAPI_PKC_KAT_DRNG_BRIDGE -I$(API_PKC_KAT_DIR) -c KEM_AlgorithmInstance.c -o $@

$(API_PKC_KAT_DIR)/api_pkc_kat_count.o: api_pkc_kat_count.c KEM_AlgorithmInstance.h $(API_PKC_TEMPLATE_STAMP)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(API_PKC_KAT_DIR) -c api_pkc_kat_count.c -o $@

$(API_PKC_KAT_TOOL): $(OBJECTS) rng.o $(API_PKC_KAT_DIR)/KAT_KEM.o $(API_PKC_KAT_DIR)/drng.o $(API_PKC_KAT_DIR)/KEM_AlgorithmInstance.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(API_PKC_COUNT_TOOL): $(OBJECTS) rng.o $(API_PKC_KAT_DIR)/drng.o $(API_PKC_KAT_DIR)/KEM_AlgorithmInstance.o $(API_PKC_KAT_DIR)/api_pkc_kat_count.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(REQUEST_RESPONSE_KAT_TOOL): $(OBJECTS) rng.o qctm512_request_response_kat.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(TRACE_REPORT_TOOL): $(OBJECTS) rng.o qctm512_trace_report.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(SPEED_TOOL): $(OBJECTS) rng.o qctm512_speed.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(VERIFY_PUBLIC_T_TOOL): $(OBJECTS) rng.o verify_public_t.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(AUDIT_MATGEN_TOOL): $(OBJECTS) rng.o audit_matgen.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(ROUNDTRIP_TOOL): $(OBJECTS) rng.o hybrid_roundtrip.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(RECOMPUTE_PK_TOOL): $(OBJECTS) rng.o recompute_public_key.o
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

verify-public-t: $(VERIFY_PUBLIC_T_TOOL)
	./$(VERIFY_PUBLIC_T_TOOL)

audit-matgen: $(AUDIT_MATGEN_TOOL)
	./$(AUDIT_MATGEN_TOOL)

roundtrip: $(ROUNDTRIP_TOOL)
	./$(ROUNDTRIP_TOOL)

verify-decode-map: $(ROUNDTRIP_TOOL)
	LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_VERIFY_DECODE_MAP=1 ./$(ROUNDTRIP_TOOL)

verify-recompute-pk: $(RECOMPUTE_PK_TOOL)
	./$(RECOMPUTE_PK_TOOL)

speed: $(SPEED_TOOL)
	./$(SPEED_TOOL)

trace-report: $(TRACE_REPORT_TOOL)
	./$(TRACE_REPORT_TOOL)
	latexmk -g -xelatex -interaction=nonstopmode -halt-on-error $(ALGORITHM)_trace_report.tex


clean:
	- /bin/rm -f *.o $(API_PKC_KAT_DIR)/*.o

veryclean: clean
	- /bin/rm -f $(KAT_TOOL) $(API_PKC_KAT_TOOL) $(API_PKC_COUNT_TOOL) $(REQUEST_RESPONSE_KAT_TOOL) $(TRACE_REPORT_TOOL) $(SPEED_TOOL) $(VERIFY_PUBLIC_T_TOOL) $(AUDIT_MATGEN_TOOL) $(ROUNDTRIP_TOOL) $(RECOMPUTE_PK_TOOL) KAT_KEM_$(ALGORITHM).txt KAT_KEM_$(ALGORITHM).req KAT_KEM_$(ALGORITHM).rsp output/KAT_KEM_$(ALGORITHM).txt $(ALGORITHM)_trace_report.tex $(ALGORITHM)_trace_report.pdf $(ALGORITHM)_trace_report.aux $(ALGORITHM)_trace_report.log $(ALGORITHM)_trace_report.out $(ALGORITHM)_trace_report.fls $(ALGORITHM)_trace_report.fdb_latexmk $(ALGORITHM)_trace_report.xdv
	- /bin/rm -rf $(API_PKC_KAT_DIR)


decode.o: goppa.h gf.h poly.h permutation.h
gf.o: gf.h rng.h
goppa.o: goppa.h gf.h poly.h permutation.h
qctm512_kem_api.o: qctm512_kem_api.h scheme_api.h sizes.h gf.h
seeded_keygen.o: seeded_keygen.h goppa.h gf.h poly.h permutation.h
kem.o: kem.h sizes.h gf.h scheme_api.h rng.h goppa.h poly.h permutation.h
kem.o: m2e.h seeded_keygen.h
m2e.o: m2e.h sizes.h gf.h scheme_api.h
permutation.o: permutation.h rng.h
poly.o: gf.h poly.h
qctm512_kat.o: rng.h qctm512_kem_api.h scheme_api.h sizes.h gf.h
qctm512_request_response_kat.o: rng.h scheme_api.h sizes.h gf.h
qctm512_trace_report.o: scheme_api.h seeded_keygen.h kem.h m2e.h rng.h sizes.h gf.h
qctm512_speed.o: rng.h scheme_api.h sizes.h gf.h
rng.o: rng.h
roots.o: gf.h poly.h
verify_public_t.o: scheme_api.h sizes.h gf.h rng.h
audit_matgen.o: scheme_api.h seeded_keygen.h goppa.h sizes.h gf.h rng.h
hybrid_roundtrip.o: scheme_api.h sizes.h gf.h rng.h
recompute_public_key.o: goppa.h scheme_api.h sizes.h gf.h rng.h
