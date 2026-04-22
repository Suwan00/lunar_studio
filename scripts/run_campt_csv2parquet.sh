#!/bin/bash
# ---------------------------------------------------------------------------
# Lunar Studio — parallel campt + csv2parquet driver.
#
# For each site<N>/ sub-folder under BASE_DIR, runs `campt` on every
# *.echo.site<N>.cub inside it (in parallel across containers) to emit a
# per-pixel CSV, then converts each site's CSVs to parquet + PNG + PLY with
# scripts/csv2parquet.py.
#
# Designed to be run from the *host* (not inside a container) — each site
# gets its own `docker run` so N sites can progress concurrently with
# MAX_PARALLEL workers.
#
# ---------------------------------------------------------------------------
# Environment variables
# ---------------------------------------------------------------------------
# BASE_DIR       : parent of the site<N>/ folders        (default: $(pwd))
# SCRIPT_DIR     : directory containing csv2parquet.py   (default: BASE_DIR)
# DOCKER_IMAGE   : Lunar Studio image tag                (default: viewlab/lunar-studio:1.0.0)
# ISIS_DATA_DIR  : host path of the ISIS data area       (default: /data/isis_data)
# MAX_PARALLEL   : max concurrent `docker run` campt jobs (default: 4)
# SITES          : newline- or space-separated list of   (default: auto-discover
#                  site relative paths such as              site*/  under BASE_DIR)
#                  "sldem/site1 sldem/site2 ..."
# CROP_WINDOW    : campt window spec as
#                  "line=1 nlines=512 sample=1 nsamples=512"
#                  (default: full 512x512)
# EXTRA_MOUNTS   : extra `-v` flags forwarded to docker   (default: empty)
# ---------------------------------------------------------------------------

set -u

BASE_DIR="${BASE_DIR:-$(pwd)}"
BASE_DIR="$(cd "$BASE_DIR" && pwd)"
SCRIPT_DIR="${SCRIPT_DIR:-$BASE_DIR}"
DOCKER_IMAGE="${DOCKER_IMAGE:-viewlab/lunar-studio:1.0.0}"
ISIS_DATA_DIR="${ISIS_DATA_DIR:-/data/isis_data}"
MAX_PARALLEL="${MAX_PARALLEL:-4}"
CROP_WINDOW="${CROP_WINDOW:-line=1 nlines=512 sample=1 nsamples=512}"
EXTRA_MOUNTS="${EXTRA_MOUNTS:-}"

# Auto-discover site<N>/ folders if SITES is not provided.
if [ -z "${SITES:-}" ]; then
    mapfile -t SITES_ARR < <(cd "$BASE_DIR" && find . -mindepth 1 -maxdepth 2 \
        -type d -name 'site*' | sed 's|^\./||' | sort)
else
    # Allow newline- or whitespace-separated lists.
    read -r -a SITES_ARR <<< "${SITES//$'\n'/ }"
fi

if [ "${#SITES_ARR[@]}" -eq 0 ]; then
    echo "ERROR: no site<N>/ folders found under $BASE_DIR and SITES is empty." >&2
    exit 1
fi

echo "============================================="
echo "Lunar Studio — run_campt_csv2parquet.sh"
echo "============================================="
echo "  BASE_DIR     : $BASE_DIR"
echo "  SCRIPT_DIR   : $SCRIPT_DIR"
echo "  DOCKER_IMAGE : $DOCKER_IMAGE"
echo "  ISIS_DATA_DIR: $ISIS_DATA_DIR"
echo "  sites        : ${#SITES_ARR[@]} (${SITES_ARR[*]})"
echo "  MAX_PARALLEL : $MAX_PARALLEL"
echo ""

# ---------------------------------------------------------------------------
# Step 1: campt in parallel (one container per site)
# ---------------------------------------------------------------------------
run_campt_for_site() {
    local site_path=$1
    local full_path="${BASE_DIR}/${site_path}"
    local cname="campt_$(echo "$site_path" | tr '/' '_' | tr -cd 'A-Za-z0-9_-')"

    echo "[$(date '+%H:%M:%S')] start campt: $site_path"

    docker run --rm --name "$cname" \
        -v "$ISIS_DATA_DIR:$ISIS_DATA_DIR" \
        -v "$BASE_DIR:$BASE_DIR" \
        $EXTRA_MOUNTS \
        -e ISISDATA="$ISIS_DATA_DIR" \
        "$DOCKER_IMAGE" \
        bash -c "
            set -e
            cd '$full_path'
            shopt -s nullglob
            for cub_file in *.echo.site*.cub; do
                csv_file=\"\${cub_file%.cub}.csv\"
                if [ -f \"\$csv_file\" ]; then
                    echo \"  ✓ \$csv_file already exists\"
                    continue
                fi
                echo \"  campt: \$cub_file\"
                campt from=\"\$cub_file\" to=\"\$csv_file\" useoffset=true \
                      $CROP_WINDOW format=flat allowerror=true
            done
        "

    local rc=$?
    if [ $rc -eq 0 ]; then
        echo "[$(date '+%H:%M:%S')] ✓ campt done: $site_path"
    else
        echo "[$(date '+%H:%M:%S')] ✗ campt failed ($rc): $site_path"
    fi
    return $rc
}

echo ">>> Step 1: parallel campt (up to $MAX_PARALLEL concurrent)"
echo ""

running=0
pids=()
for site in "${SITES_ARR[@]}"; do
    run_campt_for_site "$site" &
    pids+=($!)
    running=$((running + 1))
    if [ $running -ge $MAX_PARALLEL ]; then
        wait -n 2>/dev/null || true
        running=$((running - 1))
    fi
done

for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null || true
done

echo ""
echo "============================================="
echo "✓ Step 1 complete (all campt jobs finished)"
echo "============================================="

# ---------------------------------------------------------------------------
# Step 2: csv2parquet per site
# ---------------------------------------------------------------------------
echo ""
echo ">>> Step 2: csv2parquet"
echo ""

for site in "${SITES_ARR[@]}"; do
    full_path="${BASE_DIR}/${site}"
    echo ""
    echo "site: $site"
    csv_count=$(ls -1 "$full_path"/M*.echo.site*.csv 2>/dev/null | wc -l)
    if [ "$csv_count" -eq 0 ]; then
        echo "  ! no CSVs, skipping"
        continue
    fi
    echo "  found $csv_count CSV(s) -> parquet + PNG + PLY"
    python3 "${SCRIPT_DIR}/csv2parquet.py" --directory "$full_path" || {
        echo "  ✗ csv2parquet failed for $site"
    }
done

echo ""
echo "============================================="
echo "All processing complete."
echo "============================================="
