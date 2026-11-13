#!/usr/bin/env bash
# Build NovaGL (OpenGL ES 1.1 -> citro3d), which D1's GPU renderer draws through,
# and install it into portlibs (or $1, if given).
#
#   3ds/build-novagl.sh [install-prefix]
#
# Needs devkitARM with libctru and citro3d, dkp-toolchain-vars (3dsvars.sh), git,
# cmake and picasso. If you install somewhere other than portlibs, build the game
# with NOVAGL_PREFIX=<that prefix>.
#
# NovaGL is pinned to the revision this was tested with, plus one patch:
#   novagl-fan-nearclip.patch  The PICA200 has no near-plane clipping, and NovaGL's
#                              CPU clipper only handled GL_TRIANGLES. DXX draws every
#                              polygon as a GL_TRIANGLE_FAN, so walls crossing the
#                              near plane were culled or smeared.
# Built without NovaGL's splash screen (2.2 s at start) and with its per-draw
# diagnostics off (thousands of console lines at the start of a level).
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
if [ -z "${PORTLIBS_PREFIX:-}" ]; then
	# shellcheck disable=SC1091
	source "${DEVKITPRO:-/opt/devkitpro}/3dsvars.sh"
fi
prefix="${1:-$PORTLIBS_PREFIX}"

NOVAGL_REPO=https://github.com/efimandreev0/NovaGL.git
NOVAGL_REV=9cabf853fb57a1037bea55dbec81eea073b5ee6c

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cd "$work"

git clone -q "$NOVAGL_REPO" NovaGL
cd NovaGL
git checkout -q "$NOVAGL_REV"
git apply "$here/novagl-fan-nearclip.patch"
# src/CMakeLists.txt at this revision lists gas.c, which isn't in the repository.
sed -i.orig '/^[[:space:]]*gas\.c[[:space:]]*$/d' src/CMakeLists.txt

cmake -S . -B build \
	-DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO:-/opt/devkitpro}/cmake/3DS.cmake" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$prefix" \
	-DGL2CITRO3D_BUILD_EXAMPLES=OFF \
	-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	-DCMAKE_C_FLAGS="-DNOVAGL_NO_SPLASHSCREEN=1 -DNOVA_DIAG_DRAW_LIMIT=0" >/dev/null
cmake --build build -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" >/dev/null
cmake --install build >/dev/null
echo "Installed NovaGL to $prefix"
