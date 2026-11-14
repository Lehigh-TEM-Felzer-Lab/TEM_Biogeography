#!/bin/bash
# Preprocess the case in the current (submission) directory.
set -euo pipefail

export TEM_ROOT="${TEM_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"

if [ -n "${TEM_MODULES:-}" ]; then
    for module in $TEM_MODULES; do module load "$module"; done
fi

exec python "$TEM_ROOT/src/driver.py" "$@"
