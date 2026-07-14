#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 matthewdavidrichardanderson

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
libogc2_dir="$root/references/upstream/libogc2"
libfat_dir="$root/references/upstream/libfat"

fetch_revision() {
    local name=$1 directory=$2 url=$3 revision=$4
    if [[ -d "$directory/.git" ]]; then
        return
    fi

    echo "Fetching $name at $revision"
    mkdir -p "$(dirname "$directory")"
    git init -q "$directory"
    git -C "$directory" remote add origin "$url"
    git -C "$directory" fetch --depth 1 origin "$revision"
    git -C "$directory" checkout -q --detach FETCH_HEAD
}

check_revision() {
    local name=$1 directory=$2 expected=$3 actual
    actual="$(git -C "$directory" rev-parse HEAD)"
    if [[ "$actual" != "$expected" ]]; then
        echo "$name revision mismatch: expected $expected, found $actual" >&2
        exit 1
    fi
}

fetch_revision libogc2 "$libogc2_dir" \
    https://github.com/extremscorner/libogc2.git \
    c4a4f22dc09fc1c0df2cef4a21483a27e67423f1
fetch_revision libfat "$libfat_dir" \
    https://github.com/extremscorner/libfat.git \
    c756e1e3024e2ef2e037e99d3fd057b37ac02c5b

check_revision libogc2 "$libogc2_dir" c4a4f22dc09fc1c0df2cef4a21483a27e67423f1
check_revision libfat "$libfat_dir" c756e1e3024e2ef2e037e99d3fd057b37ac02c5b

: "${DEVKITPRO:?Set DEVKITPRO}"
: "${DEVKITPPC:?Set DEVKITPPC}"
make -C "$libogc2_dir" -j4
make -C "$libogc2_dir" install
make -C "$libfat_dir" wii-release
