#!/bin/bash
# ---------------------------------------------------------------------------
# Lunar Studio — Docker image build helper.
#
# Clones (or reuses a local checkout of) the Lunar Studio ISIS3 fork, stages
# it into the build context as ./isis_src, and runs `docker build` against
# ./Dockerfile.
#
# Environment variables
# ---------------------------------------------------------------------------
# IMAGE        : output image reference       (default: viewlab/lunar-studio:1.0.0-rc1)
# ISIS_REPO    : git URL of the ISIS fork     (default: https://github.com/Suwan00/lunar_studio.git)
# ISIS_REF     : git tag/branch/commit        (default: dev)
# ISIS_LOCAL   : path to an existing local    (default: /ssd01/lunar_studio)
#                clone to reuse instead of
#                fetching from git; set empty
#                to force a fresh clone.
# NO_CACHE     : set non-empty to pass        (default: empty)
#                --no-cache to `docker build`
# ---------------------------------------------------------------------------

set -euo pipefail

IMAGE="${IMAGE:-viewlab/lunar-studio:1.0.0-rc1}"
ISIS_REPO="${ISIS_REPO:-https://github.com/Suwan00/lunar_studio.git}"
ISIS_REF="${ISIS_REF:-dev}"
NO_CACHE="${NO_CACHE:-}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Default ISIS_LOCAL to the parent of this script's directory (the repo root
# when build_image.sh is invoked from inside docker/). This makes `git clone
# && cd docker && bash build_image.sh` just work for downstream users.
ISIS_LOCAL="${ISIS_LOCAL:-$(cd "$HERE/.." && pwd)}"
# Temporary clone location, used only when ISIS_LOCAL is empty.
CLONE_DIR="${ISIS_CLONE_DIR:-$HERE/.isis_src_clone}"

echo "============================================="
echo "Lunar Studio — Docker build"
echo "============================================="
echo "  image      : $IMAGE"
echo "  ISIS_REPO  : $ISIS_REPO"
echo "  ISIS_REF   : $ISIS_REF"
echo "  ISIS_LOCAL : ${ISIS_LOCAL:-(none)}"
echo ""

# Resolve where the ISIS3 fork lives on disk. Prefer a local checkout (fast,
# no network). Fall back to a shallow clone under $CLONE_DIR.
if [ -n "$ISIS_LOCAL" ] && [ -d "$ISIS_LOCAL/isis" ]; then
    ISIS_CTX="$ISIS_LOCAL"
    echo "Using local ISIS fork at $ISIS_CTX"
else
    ISIS_CTX="$CLONE_DIR"
    if [ ! -d "$ISIS_CTX/isis" ]; then
        echo "Cloning $ISIS_REPO (ref $ISIS_REF) into $ISIS_CTX"
        rm -rf "$ISIS_CTX"
        git clone --depth=1 --branch "$ISIS_REF" "$ISIS_REPO" "$ISIS_CTX"
    else
        echo "Reusing existing clone at $ISIS_CTX"
    fi
fi

echo ""
echo "Running docker build (isis_src build context = $ISIS_CTX)..."
build_args=(
    --file "$HERE/Dockerfile"
    --tag "$IMAGE"
    --build-context "isis_src=$ISIS_CTX"
)
if [ -n "$NO_CACHE" ]; then
    build_args+=(--no-cache)
fi

cd "$HERE"
docker build "${build_args[@]}" .

echo ""
echo "============================================="
echo "✓ Build complete: $IMAGE"
echo "============================================="
echo "Next steps:"
echo "  - smoke test: docker run --rm $IMAGE campt --help | head"
echo "  - push:       docker push $IMAGE"
