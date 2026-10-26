#!/usr/bin/env bash
# Compile, then upload only if compile succeeds.
# Usage: ./scripts/flash.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

ENV="esp32dev"

echo "==> Env: $ENV"
echo "==> Compiling..."
if ! pio run -e "$ENV"; then
  echo "==> Compile failed — skip upload." >&2
  exit 1
fi

echo "==> Compile OK — uploading..."
pio run -e "$ENV" -t upload
echo "==> Done."
