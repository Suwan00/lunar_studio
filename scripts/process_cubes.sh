#!/bin/bash
# ---------------------------------------------------------------------------
# Lunar Studio — ISIS preprocessing pipeline driver.
#
# Modes:
#   MODE=full  (default) — raw IMG -> calibrated full-cube -> bundle-adjusted
#                          network (stages 1..8).
#   MODE=site            — given a working directory that already contains
#                          site<N>/ sub-folders produced by crop_cubes.py,
#                          re-runs findfeatures + pointreg + jigsaw per site.
#
# All tunables are exposed as environment variables — override from the
# caller, from a `.env` file (`source .env && bash process_cubes.sh`), or by
# editing the block below. None of them are mandatory except SHAPE_MODEL for
# MODE=full (and ISISDATA, which is normally set by the Docker entrypoint).
#
# ---------------------------------------------------------------------------
# Environment variables
# ---------------------------------------------------------------------------
# MODE                    : "full" | "site"                        (default: full)
# WORKING_DIR             : pipeline working directory             (default: $(pwd))
# OUTPUT_SUBDIR           : sub-directory for full-mode outputs    (default: output)
# SHAPE_MODEL             : DEM shape model cube                   (required for full)
# ISISDATA                : ISIS data area                         (required)
#
# MIN_LAT,MAX_LAT,MIN_LON,MAX_LON : seedgrid ROI bounds (full mode, required)
# LATSTEP,LONSTEP         : seedgrid spacing in degrees            (default: 0.05)
#
# MAX_JIGSAW_ITERATIONS   : jigsaw maxits                          (default: 500000)
# SIGMA                   : jigsaw a-priori sigma                  (default: 1.0)
# ENABLE_OUTLIER_REJECTION: "true"|"false"                         (default: true)
#
# NUM_SITES               : site count for MODE=site auto-discover (default: 5)
# REFERENCE_ID            : reference product ID for findfeatures  (default: first)
# ---------------------------------------------------------------------------

set -e

source /opt/conda/etc/profile.d/conda.sh
conda activate lunar_studio

MODE="${MODE:-full}"
WORKING_DIR="${WORKING_DIR:-$(pwd)}"
OUTPUT_SUBDIR="${OUTPUT_SUBDIR:-output}"
LATSTEP="${LATSTEP:-0.05}"
LONSTEP="${LONSTEP:-0.05}"
MAX_JIGSAW_ITERATIONS="${MAX_JIGSAW_ITERATIONS:-500000}"
SIGMA="${SIGMA:-1.0}"
ENABLE_OUTLIER_REJECTION="${ENABLE_OUTLIER_REJECTION:-true}"
NUM_SITES="${NUM_SITES:-5}"

WORKING_DIR="$(cd "$WORKING_DIR" && pwd)"
cd "$WORKING_DIR"

if [ -z "$ISISDATA" ]; then
    echo "ERROR: ISISDATA is not set. Either run inside the Lunar Studio" \
         "Docker image (which wires it from the host) or export it manually." >&2
    exit 1
fi

print_header() {
    echo ""
    echo "============================================="
    echo "$1"
    echo "============================================="
}

print_step() {
    echo ">>> $1"
}

verify_file() {
    if [ -f "$1" ]; then
        echo "  ✓ $1"
        return 0
    fi
    echo "  ✗ missing: $1"
    return 1
}

write_validmeasure_def() {
    local target="$1"
    if [ -f "$target" ]; then return 0; fi
    cat > "$target" <<'EOF'
Object = ValidMeasure
  Group = MeasureValidation
    MinDN = 1
    MaxDN = 65535
    MinEmission = 0
    MaxEmission = 90
    MinIncidence = 0
    MaxIncidence = 90
  EndGroup
EndObject
EOF
}

write_pointreg_def_full() {
    local target="$1"
    if [ -f "$target" ]; then return 0; fi
    cat > "$target" <<'EOF'
Object = AutoRegistration
  Group = Algorithm
    Name = MaximumCorrelation
    Tolerance = 0.7
  EndGroup
  Group = PatternChip
    Samples = 31
    Lines = 31
    ValidPercent = 50
  EndGroup
  Group = SearchChip
    Samples = 101
    Lines = 101
    ValidPercent = 50
  EndGroup
EndObject
EOF
}

write_pointreg_def_site() {
    local target="$1"
    if [ -f "$target" ]; then return 0; fi
    cat > "$target" <<'EOF'
Object = AutoRegistration
  Group = Algorithm
    Name = MaximumCorrelation
    Tolerance = 0.5
    MinimumZScore = 1.5
  EndGroup
  Group = PatternChip
    Samples = 31
    Lines = 31
    ValidPercent = 50
  EndGroup
  Group = SearchChip
    Samples = 151
    Lines = 151
    ValidPercent = 50
  EndGroup
  Group = SurfaceModel
    DistanceTolerance = 2.0
    WindowSize = 7
  EndGroup
EndObject
EOF
}

run_jigsaw() {
    local fromlist=$1
    local cnet_in=$2
    local cnet_out=$3
    local held=$4
    local cmd="jigsaw fromlist=\"$fromlist\" cnet=\"$cnet_in\" onet=\"$cnet_out\" \
        update=yes file_prefix=jigsawErr sigma=$SIGMA \
        maxits=$MAX_JIGSAW_ITERATIONS camsolve=velocities spsolve=positions \
        heldlist=\"$held\" \
        control_point_coordinate_type_bundle=rectangular \
        control_point_coordinate_type_reports=rectangular \
        point_x_sigma=10 point_y_sigma=10 point_z_sigma=10"
    if [ "$ENABLE_OUTLIER_REJECTION" = "true" ]; then
        cmd="$cmd outlier_rejection=yes rejection_multiplier=3.0"
    fi
    eval $cmd
}

# ===========================================================================
# MODE: full
# ===========================================================================
process_full() {
    if [ -z "$SHAPE_MODEL" ]; then
        echo "ERROR: SHAPE_MODEL must be set for MODE=full" >&2
        exit 1
    fi
    : "${MIN_LAT:?MIN_LAT must be set for MODE=full}"
    : "${MAX_LAT:?MAX_LAT must be set for MODE=full}"
    : "${MIN_LON:?MIN_LON must be set for MODE=full}"
    : "${MAX_LON:?MAX_LON must be set for MODE=full}"

    local out_dir="$WORKING_DIR/$OUTPUT_SUBDIR"
    mkdir -p "$out_dir"

    print_header "Full-cube pipeline"
    echo "  working dir : $WORKING_DIR"
    echo "  output dir  : $out_dir"
    echo "  shape model : $SHAPE_MODEL"
    echo "  ROI         : lat[$MIN_LAT,$MAX_LAT] lon[$MIN_LON,$MAX_LON]"

    # Stage 1 — IMG -> raw cub
    print_step "Stage 1: lronac2isis (IMG -> raw.cub)"
    for img_file in "$WORKING_DIR"/*.IMG; do
        [ -f "$img_file" ] || continue
        local base="$(basename "$img_file" .IMG)"
        local raw_cub="$out_dir/${base}.raw.cub"
        if [ ! -f "$raw_cub" ]; then
            lronac2isis from="$img_file" to="$raw_cub"
            verify_file "$raw_cub"
        fi
    done

    cd "$out_dir"

    # Stage 2 — spiceinit
    print_step "Stage 2: spiceinit (attach SPICE + DEM shape model)"
    for raw_cub in *.raw.cub; do
        [ -f "$raw_cub" ] || continue
        local base="${raw_cub%.raw.cub}"
        if [ ! -f "${base}.spiceinit_done" ]; then
            spiceinit from="$raw_cub" shape=user model="$SHAPE_MODEL" \
                      CKSMITHED=True SPKSMITHED=True
            touch "${base}.spiceinit_done"
        fi
    done

    # Stage 3 — lronaccal
    print_step "Stage 3: lronaccal (radiometric calibration)"
    for raw_cub in *.raw.cub; do
        [ -f "$raw_cub" ] || continue
        local base="${raw_cub%.raw.cub}"
        local cal_cub="${base}.cal.cub"
        if [ ! -f "$cal_cub" ]; then
            lronaccal from="$raw_cub" to="$cal_cub"
            verify_file "$cal_cub"
        fi
    done

    # Stage 4 — lronacecho
    print_step "Stage 4: lronacecho (echo correction)"
    for cal_cub in *.cal.cub; do
        [ -f "$cal_cub" ] || continue
        local base="${cal_cub%.cal.cub}"
        local echo_cub="${base}.echo.cub"
        if [ ! -f "$echo_cub" ]; then
            lronacecho from="$cal_cub" to="$echo_cub"
            verify_file "$echo_cub"
        fi
    done

    # Stage 5 — footprintinit
    print_step "Stage 5: footprintinit"
    for echo_cub in *.echo.cub; do
        [ -f "$echo_cub" ] || continue
        local base="${echo_cub%.echo.cub}"
        if [ ! -f "${base}.footprint_done" ]; then
            footprintinit from="$echo_cub" linc=30 sinc=30 increaseprecision=true
            touch "${base}.footprint_done"
        fi
    done

    # Stage 6 — seedgrid / cnetadd / cnetedit
    print_step "Stage 6: control network (seedgrid + cnetadd + cnetedit)"
    ls -1 *.echo.cub > cube.lis
    local num_cubes
    num_cubes=$(wc -l < cube.lis)
    if [ "$num_cubes" -lt 2 ]; then
        echo "  ERROR: need at least 2 cubes for a control network" >&2
        exit 1
    fi

    write_validmeasure_def validmeasure.def
    if [ ! -f seed.net ]; then
        seedgrid target=moon \
                 minlat=$MIN_LAT maxlat=$MAX_LAT \
                 minlon=$MIN_LON maxlon=$MAX_LON \
                 spacing=latlon latstep=$LATSTEP lonstep=$LONSTEP \
                 networkid="lunar_studio" pointid="???????" \
                 description="lunar_studio" onet=seed.net
    fi
    if [ ! -f initial.net ]; then
        cnetadd addlist=cube.lis deffile=validmeasure.def retrieval=point \
                onet=initial.net polygon=false cnet=seed.net
    fi
    [ -f initial_edit.net ] || cnetedit cnet=initial.net onet=initial_edit.net

    # Stage 7 — pointreg (sub-pixel)
    print_step "Stage 7: pointreg (sub-pixel registration)"
    write_pointreg_def_full pointreg_P31x31_S101x101.def
    if [ ! -f control_pointreg_edit.net ]; then
        if pointreg fromlist=cube.lis cnet=initial_edit.net \
                    onet=control_pointreg.net \
                    deffile=pointreg_P31x31_S101x101.def; then
            cnetedit cnet=control_pointreg.net onet=control_pointreg_edit.net
        else
            echo "  ! pointreg failed, falling back to initial_edit.net"
            cp initial_edit.net control_pointreg_edit.net
        fi
    fi

    # Stage 8 — jigsaw
    print_step "Stage 8: jigsaw (bundle adjustment)"
    [ -f held.lis ] || head -n 1 cube.lis > held.lis
    if [ ! -f control_ba.net ]; then
        run_jigsaw cube.lis control_pointreg_edit.net control_ba.net held.lis
        verify_file control_ba.net
    fi

    print_header "Full-cube pipeline complete for $OUTPUT_SUBDIR"
    echo "Results in: $out_dir"
}

# ===========================================================================
# MODE: site  (assumes crop_cubes.py has populated site<N>/ sub-folders)
# ===========================================================================
process_site() {
    print_header "Site-mode post-crop pipeline"
    echo "  working dir : $WORKING_DIR"

    # Auto-discover site<N> folders
    local sites=()
    for dir in "$WORKING_DIR"/site*/; do
        [ -d "$dir" ] || continue
        sites+=("$(basename "$dir")")
    done
    if [ "${#sites[@]}" -eq 0 ]; then
        echo "ERROR: no site<N>/ directories found under $WORKING_DIR. Run" \
             "crop_cubes.py first." >&2
        exit 1
    fi
    echo "  sites       : ${sites[*]}"

    for site in "${sites[@]}"; do
        print_header "Processing $site"
        local site_dir="$WORKING_DIR/$site"
        cd "$site_dir"

        # Clean stale bookkeeping from previous attempts
        rm -f ./*.net cube.lis held.lis cube_no_ref.lis pointreg_relaxed.def 2>/dev/null || true

        ls -1 *.echo.${site}.cub > cube.lis 2>/dev/null || true
        if [ ! -s cube.lis ]; then
            echo "  ! no *.echo.${site}.cub files, skipping"
            continue
        fi
        local num_cubes
        num_cubes=$(wc -l < cube.lis)
        if [ "$num_cubes" -lt 2 ]; then
            echo "  ! need at least 2 cubes, skipping"
            continue
        fi

        # Pick the reference cube (explicit REFERENCE_ID or first entry)
        local reference_cub
        if [ -n "$REFERENCE_ID" ]; then
            reference_cub=$(grep "$REFERENCE_ID" cube.lis | head -n 1)
        fi
        [ -n "$reference_cub" ] || reference_cub=$(head -n 1 cube.lis)
        echo "  reference   : $reference_cub"

        grep -v "^${reference_cub}$" cube.lis > cube_no_ref.lis

        # findfeatures + cnetedit
        if [ ! -f initial_edit.net ]; then
            if findfeatures fromlist=cube_no_ref.lis onet=initial.net \
                            algorithm=sift match="$reference_cub" \
                            epitolerance=10.0 ratio=0.75 \
                            hmgtolerance=10.0 fastgeom=yes geomtype=camera \
                            fastgeompoints=25; then
                cnetedit cnet=initial.net onet=initial_edit.net
            else
                echo "  ! findfeatures failed for $site"
                rm -f cube_no_ref.lis
                continue
            fi
        fi
        rm -f cube_no_ref.lis

        # pointreg
        write_pointreg_def_site pointreg_relaxed.def
        if [ ! -f control_pointreg_edit.net ]; then
            if pointreg fromlist=cube.lis cnet=initial_edit.net \
                        onet=control_pointreg.net deffile=pointreg_relaxed.def; then
                cnetedit cnet=control_pointreg.net onet=control_pointreg_edit.net
            else
                cp initial_edit.net control_pointreg_edit.net
            fi
        fi

        # jigsaw
        [ -f held.lis ] || head -n 1 cube.lis > held.lis
        if [ ! -f control_ba.net ]; then
            run_jigsaw cube.lis control_pointreg_edit.net control_ba.net held.lis
        fi
    done

    cd "$WORKING_DIR"
    print_header "Site-mode pipeline complete"
}

case "$MODE" in
    full) process_full ;;
    site) process_site ;;
    *)
        echo "ERROR: unknown MODE='$MODE' (expected 'full' or 'site')" >&2
        exit 1
        ;;
esac
