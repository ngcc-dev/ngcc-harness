#!/bin/bash
# Unattended campaign on several cores: the instances are divided into one shard
# per core in CPUS (campaign.py --shard; best one core per CPU cluster, so the
# shards do not share an L2 cache), and the deferred very slow instances run on
# DEFERRED_CPU at the same time (--deferred-only). Every step resumes, so the
# script can simply be started again after an interruption.
#
#   performance/campaign_parallel.sh RUN_DIR "CPUS" DEFERRED_CPU [campaign.py options]
#   performance/campaign_parallel.sh performance/runs/vivo-campaign-X "3 7 11" 10 \
#       --trial-target 0.5 --op-budget 300
#
# Order: build (a no-op once built), the ICCS baseline on the first CPU, then per
# shard calibrate -> measure -> profile. Progress: RUN_DIR/campaign.log; each
# process's own output: RUN_DIR/shard-K.out and RUN_DIR/deferred.out.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
cd "$R"
[ $# -ge 3 ] || { sed -n '2,15p' "$0"; exit 1; }
run=$1; read -r -a cpus <<<"$2"; dcpu=$3; shift 3
n=${#cpus[@]}
campaign() { python3 performance/campaign.py "$@"; }

# Keep this script, the campaign.py orchestrators and their helpers (make,
# readelf, the clock probe's parent) off the benchmark cores; every measured
# process is pinned to its own core with taskset by campaign.py.
housekeeping=$(python3 -c 'import os, sys
bench = {int(c) for c in sys.argv[1:]}
print(",".join(str(c) for c in sorted(os.sched_getaffinity(0) - bench)))' "${cpus[@]}" "$dcpu")
[ -n "$housekeeping" ] && taskset -cp "$housekeeping" $$ >/dev/null

campaign build --run-dir "$run" || exit
campaign baseline --cpu "${cpus[0]}" --run-dir "$run" "$@" || exit

pids=()
for k in $(seq 1 "$n"); do
    (for phase in calibrate measure profile; do
         campaign "$phase" --cpu "${cpus[k-1]}" --shard "$k/$n" --defer --run-dir "$run" "$@" || exit
     done) >"$run/shard-$k.out" 2>&1 &
    pids+=($!)
done
(for phase in calibrate measure profile; do
     campaign "$phase" --cpu "$dcpu" --deferred-only --run-dir "$run" "$@" || exit
 done) >"$run/deferred.out" 2>&1 &
dpid=$!

status=0
for pid in "${pids[@]}"; do wait "$pid" || status=1; done
echo "$(date '+%F %T') shards finished (status $status); deferred instances still running on CPU $dcpu" \
    | tee -a "$run/campaign.log"
wait "$dpid" || status=1
echo "$(date '+%F %T') deferred instances finished (status $status)" | tee -a "$run/campaign.log"
exit $status
