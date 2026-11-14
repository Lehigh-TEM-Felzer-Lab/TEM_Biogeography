#!/usr/bin/env bash
set -euo pipefail
export TEM_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python "$TEM_ROOT/launchers/case_submit.py"
