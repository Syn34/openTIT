#!/usr/bin/env bash
# Build the tools, recompile your EBOOT and build the game (Linux / macOS).
#   tools/build.sh "/path/to/Sly 4 dump"
set -euo pipefail
dump="${1:?usage: tools/build.sh <path to disc dump>}"
gen="${2:-gen}"
cd "$(dirname "$0")/.."

eboot="$dump/PS3_GAME/USRDIR/EBOOT.elf"
if [[ ! -f "$eboot" ]]; then
  echo "Decrypted executable not found: $eboot" >&2
  echo "Decrypt EBOOT.BIN with RPCS3 (Utilities > Decrypt PS3 Binaries) and save it as EBOOT.elf." >&2
  exit 1
fi

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/opentit_tests

./build/opentit-recomp recompile "$eboot" -o "$gen"

# Optional LLE firmware modules (decrypted with RPCS3), e.g. firmware/libsre.prx
base=$((0x01800000))
for prx in "$dump"/firmware/*.prx; do
  [[ -f "$prx" ]] || continue
  name="$(basename "$prx" .prx)"
  ./build/opentit-recomp recompile "$prx" -o "$gen/modules/$name" --base "$(printf '%X' $base)" --name "$name"
  base=$((base + 0x00400000))
done

cmake -S . -B build-game -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENTIT_BUILD_TESTS=OFF \
      -DOPENTIT_GENERATED_DIR="$(cd "$gen" && pwd)"
cmake --build build-game
echo
echo "Done. Run:  ./build-game/opentit \"$dump\" --log info"
