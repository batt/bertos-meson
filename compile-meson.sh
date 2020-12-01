#!/bin/bash

set -euo pipefail

OBJDIR="obj/meson"
PRJDIR="boot"

if [ ! -d "$OBJDIR" ]; then
    mkdir -p "$OBJDIR"
    meson --cross-file="$PRJDIR"/cross.build "$OBJDIR" "$PRJDIR"
fi

ninja -C "$OBJDIR" "${@:-all}"


