#!/usr/bin/env bash
# Flash firmware, then open serial monitor.
# Usage: ./scripts/monitor.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

ENV="esp32dev"

"$ROOT/scripts/flash.sh"

echo "==> Opening serial monitor (Ctrl+C to quit)..."
echo "    Expect MOV/OCC distance lines when a target is present."
pio device monitor -e "$ENV"
