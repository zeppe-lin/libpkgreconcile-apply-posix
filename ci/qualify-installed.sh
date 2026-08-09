#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 2 ] || { echo "usage: $0 BUILD-DIR {shared|static}" >&2; exit 2; }
build=$1
mode=$2
case $mode in shared|static) ;; *) exit 2 ;; esac
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
dependency_prefix=$(cat "$build/ci-dependency-prefix")
install_prefix=$(cat "$build/ci-install-prefix")
export PKG_CONFIG_PATH="$install_prefix/lib/pkgconfig:$dependency_prefix/lib/pkgconfig"
export LD_LIBRARY_PATH="$install_prefix/lib:$dependency_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
flags=$(pkg-config --cflags --libs libpkgreconcile-apply-posix)
if [ "$mode" = static ]; then
  flags=$(pkg-config --cflags --static --libs --static libpkgreconcile-apply-posix)
fi
# shellcheck disable=SC2086
${CXX:-c++} -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$root/ci/installed-consumer.cpp" $flags -o "$build/installed-consumer"
"$build/installed-consumer"
