#!/bin/bash
# ---------------------------------------------------------------------------
# Lunar Studio — 30-second Docker image smoke test.
#
# Runs a battery of quick (≤5 s each) checks against a freshly-built image to
# catch packaging problems before you commit to the multi-hour end-to-end run.
# Verifies:
#
#   1. Image metadata (labels, tag reachable)
#   2. Conda env `lunar_studio` activates on container start
#   3. Core ISIS binaries are on $PATH (campt, jigsaw, lronac2isis, …)
#   4. Our campt has the Lunar-Studio extensions (useoffset / format=flat /
#      line-range / allowerror flags)
#   5. KPLO LUTI camera plugin + translation files are in place
#   6. Ames Stereo Pipeline apps on $PATH (parallel_stereo)
#   7. ISISDATA wiring works when the host passes -e ISISDATA=...
#
# Usage:
#   bash smoke_test.sh                                   # default tag
#   IMAGE=viewlab/lunar-studio:1.0.0 bash smoke_test.sh  # custom tag
# ---------------------------------------------------------------------------

set -u

IMAGE="${IMAGE:-viewlab/lunar-studio:1.0.0-rc1}"
ISIS_DATA_HOST="${ISIS_DATA_HOST:-/ssd00/datasets/isis/data}"

pass=0
fail=0
warn=0

run_check() {
    local label="$1"
    local cmd="$2"
    local expect="${3:-}"
    local output
    output=$(eval "$cmd" 2>&1)
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "  ✗ $label"
        echo "      cmd : $cmd"
        echo "      exit: $rc"
        echo "      out : $(echo "$output" | head -3)"
        fail=$((fail + 1))
        return
    fi
    if [ -n "$expect" ] && ! echo "$output" | grep -qE "$expect"; then
        echo "  ⚠ $label — ran but expected output pattern not seen"
        echo "      cmd    : $cmd"
        echo "      expect : $expect"
        echo "      got    : $(echo "$output" | head -3)"
        warn=$((warn + 1))
        return
    fi
    echo "  ✓ $label"
    pass=$((pass + 1))
}

echo "============================================="
echo "Lunar Studio — smoke test for $IMAGE"
echo "============================================="
echo ""

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "✗ Docker image $IMAGE not found locally." >&2
    exit 1
fi

echo ">>> 1. Image metadata"
run_check "image labels include Lunar Studio" \
    "docker inspect --format '{{json .Config.Labels}}' $IMAGE" \
    "Lunar Studio"

echo ""
echo ">>> 2. Conda env activation"
run_check "CONDA_DEFAULT_ENV=lunar_studio on shell start" \
    "docker run --rm $IMAGE bash -lc 'echo ENV=\$CONDA_DEFAULT_ENV'" \
    "ENV=lunar_studio"

echo ""
echo ">>> 3. Core ISIS binaries on PATH (bundled probe)"
run_check "campt/jigsaw/lronac*/spiceinit/… all resolvable in one shell" \
    "docker run --rm $IMAGE bash -lc '
        set -e
        missing=0
        for b in campt jigsaw lronac2isis lronaccal lronacecho spiceinit \\
                 footprintinit seedgrid cnetadd cnetedit pointreg \\
                 findfeatures crop camrange isis2std; do
            path=\$(command -v \$b) || { echo MISSING:\$b; missing=\$((missing+1)); continue; }
            echo OK:\$b=\$path
        done
        [ \$missing -eq 0 ] && echo ALL_PRESENT || { echo MISSING_COUNT=\$missing; exit 1; }
    '" \
    "ALL_PRESENT"

echo ""
echo ">>> 4. Lunar Studio campt extensions"
# Bare `campt` triggers Qt GUI init (which fails without X), so the
# reliable check is that our modified campt.xml declares the Lunar
# Studio flags (USEOFFSET / ALLOWERROR / NLINES / NSAMPLES).
run_check "campt.xml declares Lunar Studio extensions (USEOFFSET / ALLOWERROR / NLINES)" \
    "docker run --rm $IMAGE bash -lc 'grep -iE \"useoffset|allowerror|nlines|nsamples\" /opt/ISIS3/build/bin/xml/campt.xml | head -10'" \
    "USEOFFSET|ALLOWERROR|NLINES|NSAMPLES"

echo ""
echo ">>> 5. KPLO LUTI camera plugin + translations"
run_check "LutiCamera library present" \
    "docker run --rm $IMAGE bash -lc 'ls /opt/ISIS3/build/lib/ 2>/dev/null | grep -i luti || ls /opt/conda/envs/lunar_studio/lib/ | grep -i luti'" \
    "luti"
run_check "KploLuti*.trn translations present" \
    "docker run --rm $IMAGE bash -lc 'ls /opt/ISIS3/build/appdata/translations/ | grep -i KploLuti'" \
    "KploLutiInstrument|KploLutiArchive|KploLutiBandBin|KploLutiSerialNumber"

echo ""
echo ">>> 6. Ames Stereo Pipeline on PATH"
run_check "which parallel_stereo" \
    "docker run --rm $IMAGE bash -lc 'which parallel_stereo'" \
    "StereoPipeline.*/bin/parallel_stereo"

echo ""
echo ">>> 7. ISISDATA host→container wiring"
if [ -d "$ISIS_DATA_HOST" ]; then
    run_check "host ISISDATA mount surfaces inside container" \
        "docker run --rm -v $ISIS_DATA_HOST:/isis_data -e ISISDATA=/isis_data $IMAGE bash -lc 'echo ISISDATA=\$ISISDATA && ls /isis_data | head -3'" \
        "ISISDATA=/isis_data"
else
    echo "  ⊖ skipped (ISIS_DATA_HOST=$ISIS_DATA_HOST not present)"
fi

echo ""
echo "============================================="
echo "Smoke test summary: $pass passed, $warn warnings, $fail failed"
echo "============================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
if [ $warn -gt 0 ]; then
    echo "(warnings are non-fatal but worth reviewing before E2E)"
    exit 0
fi
echo "✅ Image passes all smoke checks — ready for E2E."
