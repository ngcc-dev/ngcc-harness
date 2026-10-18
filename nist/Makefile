PRIVATE_SOURCE ?= ../../nist-analysis
ALLOW_DIRTY ?= 0
SYNC_DIRTY_FLAG = $(if $(filter 1 yes true,$(ALLOW_DIRTY)),--allow-dirty,)

.PHONY: all check sync check-sync fetch reproduce clean

all: check

check:
	python3 tools/check.py
	python3 tools/test_reproduce.py

sync:
	python3 tools/sync.py $(SYNC_DIRTY_FLAG) "$(PRIVATE_SOURCE)"

check-sync:
	python3 tools/sync.py --check $(SYNC_DIRTY_FLAG) "$(PRIVATE_SOURCE)"

fetch:
	python3 tools/fetch_submissions.py

reproduce:
	@test -n "$(ID)" || { echo "set ID to a finding such as QR-UOV-3" >&2; exit 2; }
	python3 reproduce.py "$(ID)"

clean:
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
	$(MAKE) -C api clean
	@for scheme in FAEST HAWK MAYO MQOM QR-UOV SDitH SNOVA SQIsign UOV; do \
		$(MAKE) -C "$$scheme" clean >/dev/null 2>&1 || true; \
	done
