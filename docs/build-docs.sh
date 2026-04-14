#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# whoops forgot to source
if [[ -f .venv/bin/activate ]]; then
  source .venv/bin/activate
fi

# clean generated trees; exhale + doxygen will regenerate them.
rm -rf _build doxygen api

sphinx-build -b html -j auto -W --keep-going . _build/html

echo "Built: docs/_build/html/index.html"
