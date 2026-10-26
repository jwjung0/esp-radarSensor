#!/usr/bin/env bash
# Compile only (no upload).
# Usage: ./scripts/build.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

ENV="esp32dev"

echo "==> Env: $ENV"
echo "==> Compiling..."
if ! pio run -e "$ENV"; then
  echo "==> Compile failed." >&2
  exit 1
fi

echo "==> Build OK."
