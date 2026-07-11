#!/usr/bin/env bash

#
#
# Zips the project source code to desktop
#
#

set -euo pipefail

OUT="/c/Users/Dev/Desktop/game.zip"

mkdir -p "$(dirname "$OUT")"

# Overwrite existing archive if present
rm -f "$OUT"

zip -r "$OUT" . \
  -x ".git/*" ".git" \
     ".vscode/*" ".vscode" \
     ".agents/*" ".agents" \
     "assets/*" "assets"

echo
echo "Created: $OUT"
echo
read -rp "Press Enter to exit..."