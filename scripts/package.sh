#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ARCHIVE="Projet-Mezrhab-Tripnaux"
DEST="$ROOT/${ARCHIVE}.tar.gz"

cd "$ROOT"

{ git ls-files --cached; git ls-files --others --exclude-standard; } | sort -u | \
    tar -czf "$DEST" -T - --transform "s|^|${ARCHIVE}/|"

echo "Archive créée : $(realpath "$DEST")"
