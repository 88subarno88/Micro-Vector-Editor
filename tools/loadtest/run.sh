#!/usr/bin/env bash
# Runs one load-test scenario in a node:24 container, against the stack on BASE, while sampling the stack's
# containers' CPU and memory. Results go to tools/loadtest/results/<label>.{json,stats}.
#
#   PROJECT=vecload BASE=http://127.0.0.1:8100 tools/loadtest/run.sh A-e20 --docs 1 --editors 20 --opRate 25
#
# PROJECT is the Compose project to sample (its containers are <project>-api-1-1 and so on).
set -u
dir="$(cd "$(dirname "$0")" && pwd)"
project="${PROJECT:-vecload}"
base="${BASE:-http://127.0.0.1:8100}"
label="$1"; shift
stats="$dir/results/$label.stats"
mkdir -p "$dir/results"
: > "$stats"
( while true; do
    docker stats --no-stream --format '{{.Name}} {{.CPUPerc}} {{.MemUsage}}' \
      "$project-api-1-1" "$project-api-2-1" "$project-postgres-1" "$project-redis-1" "$project-web-1" \
      >> "$stats" 2>/dev/null
  done ) &
sampler=$!
docker run --rm --network host -v "$dir":/lt:ro node:24-slim \
  node /lt/loadtest.mjs --base "$base" --label "$label" "$@" | tee "$dir/results/$label.json"
kill $sampler 2>/dev/null; wait $sampler 2>/dev/null
# Mean and peak CPU per container over the run (docker stats' CPU% is per core: 100% is one core).
awk '{gsub("%","",$2); $2=$2+0; n[$1]++; s[$1]+=$2; if ($2>m[$1]) m[$1]=$2}
     END {for (k in n) printf "  %-22s mean %6.1f%%  peak %6.1f%%\n", k, s[k]/n[k], m[k]}' "$stats" | sort
