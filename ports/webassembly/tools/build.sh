#!/usr/bin/env bash
#
# Build the webassembly MicroPython for the badgeware simulator and deploy the
# resulting micropython.mjs / micropython.wasm into the badgeware-web simulator.
#
# Usage:
#   tools/build.sh              # build VARIANT=pyscript and deploy
#   tools/build.sh --clean      # make clean first, then build and deploy
#   tools/build.sh --no-deploy  # build only, don't copy into badgeware-web
#   VARIANT=standard tools/build.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"            # ports/webassembly
WEB_DIR="$(cd "$PORT_DIR/../../.." && pwd)"          # .../web
EMSDK_ENV="$WEB_DIR/emsdk/emsdk_env.sh"
DEPLOY_DIR="$WEB_DIR/badgeware-web/simulator"

VARIANT="${VARIANT:-pyscript}"
BUILD_DIR="$PORT_DIR/build-$VARIANT"

clean=0
deploy=1
for arg in "$@"; do
  case "$arg" in
    --clean)     clean=1 ;;
    --no-deploy) deploy=0 ;;
    *) echo "unknown argument: $arg" >&2; exit 2 ;;
  esac
done

# Activate emscripten if emcc isn't already on PATH.
if ! command -v emcc >/dev/null 2>&1; then
  if [ -f "$EMSDK_ENV" ]; then
    # shellcheck disable=SC1090
    source "$EMSDK_ENV" >/dev/null
  else
    echo "error: emcc not on PATH and $EMSDK_ENV not found" >&2
    exit 1
  fi
fi

if [ "$clean" = 1 ]; then
  echo ">> make clean (VARIANT=$VARIANT)"
  make -C "$PORT_DIR" VARIANT="$VARIANT" clean
fi

echo ">> building (VARIANT=$VARIANT)"
make -C "$PORT_DIR" VARIANT="$VARIANT"

if [ "$deploy" = 1 ]; then
  echo ">> deploying to $DEPLOY_DIR"
  cp "$BUILD_DIR/micropython.mjs"  "$DEPLOY_DIR/micropython.mjs"
  cp "$BUILD_DIR/micropython.wasm" "$DEPLOY_DIR/micropython.wasm"
fi

echo ">> done"
