#!/usr/bin/env bash
# Rebuild devkitPro's 3ds-sdl (SDL 1.2.15 + devkitPro's 3DS patch) with the stereoscopic
# 3D patch in this folder, and install it over the 3ds-sdl package in portlibs.
#
#   3ds/build-sdl-stereo.sh
#
# Needs devkitARM with dkp-toolchain-vars (3dsvars.sh), curl, patch, autoconf,
# automake and libtool. To undo, reinstall the package: dkp-pacman -S 3ds-sdl
# Without this SDL the game still builds and runs, in 2D only.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
if [ -z "${PORTLIBS_PREFIX:-}" ]; then
	# shellcheck disable=SC1091
	source "${DEVKITPRO:-/opt/devkitpro}/3dsvars.sh"
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cd "$work"

# Same sources and checksums as devkitPro's pacman-packages 3ds/SDL (3ds-sdl 1.2.15-19)
curl -sSL -o SDL-1.2.15.tar.gz https://libsdl.org/release/SDL-1.2.15.tar.gz
curl -sSL -o SDL-1.2.15.patch https://raw.githubusercontent.com/devkitPro/pacman-packages/13a6a53b/3ds/SDL/SDL-1.2.15.patch
shasum -a 256 -c - <<SUMS
d6d316a793e5e348155f0dd93b979798933fb98aa1edebcc108829d6474aad00  SDL-1.2.15.tar.gz
83941419c8fc87a280b1d45db595b720fe190959a0094c727a566d2b9de20804  SDL-1.2.15.patch
SUMS

tar xzf SDL-1.2.15.tar.gz
cd SDL-1.2.15
patch -p1 -s -i ../SDL-1.2.15.patch
patch -p1 -s -i "$here/sdl-n3ds-stereo.patch"
./autogen.sh >/dev/null
./configure --prefix="$PORTLIBS_PREFIX" --host=arm-none-eabi --disable-shared --enable-static --enable-n3ds >/dev/null
make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" >/dev/null
make install >/dev/null
rm -rf "${PORTLIBS_PREFIX:?}/share/doc/SDL" 2>/dev/null || true
echo "Installed SDL with stereoscopic 3D to $PORTLIBS_PREFIX"
