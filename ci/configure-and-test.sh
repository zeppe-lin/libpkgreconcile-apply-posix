#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

usage() { echo "usage: $0 BUILD-DIR {shared|static} [MESON-ARG ...]" >&2; exit 2; }
[ "$#" -ge 2 ] || usage
build_dir=$1
link_mode=$2
shift 2
case $link_mode in shared|static) ;; *) usage ;; esac

root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
case $build_dir in /*) build=$build_dir ;; *) build=$(pwd)/$build_dir ;; esac
dependency_prefix=$build/dependencies
install_prefix=$build/install
rm -rf "$build"
mkdir -p "$build"

setup_dependency()
{
  source_dir=$1
  output_dir=$2
  shift 2
  meson setup "$output_dir" "$source_dir" \
    --wrap-mode=nofallback --fatal-meson-warnings \
    --prefix="$dependency_prefix" --libdir=lib \
    -Ddefault_library="$link_mode" -Dlink_mode="$link_mode" \
    -Dtests=disabled -Dman_pages=disabled -Dwerror=true \
    ${MESON_SANITIZE:+-Db_sanitize="$MESON_SANITIZE"} \
    ${MESON_SANITIZE:+-Db_lundef=false} \
    "$@"
  meson compile -C "$output_dir"
  meson install -C "$output_dir"
}

for variable in \
  LIBPKGSOURCE_SOURCE LIBPKGSTATE_SOURCE LIBPKGIMAGE_SOURCE LIBPKGCATALOG_SOURCE \
  LIBPKGRESOLVE_SOURCE LIBPKGBUILD_SOURCE LIBPKGPLAN_SOURCE \
  LIBPKGBUILD_IMAGE_SOURCE LIBPKGSOURCE_PLAN_SOURCE LIBPKGBUILD_PLAN_SOURCE \
  LIBPKGAPPLY_SOURCE LIBPKGRECONCILE_SOURCE LIBPKGAPPLY_POSIX_SOURCE \
  LIBPKGRECONCILE_APPLY_SOURCE LIBPKGRECONCILE_POSIX_SOURCE; do
  eval "value=\${$variable-}"
  [ -n "$value" ] || { echo "set $variable" >&2; exit 2; }
done

setup_dependency "$LIBPKGSOURCE_SOURCE" "$build/libpkgsource" -Dhtml_docs=disabled
export PKG_CONFIG_PATH="$dependency_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$dependency_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
setup_dependency "$LIBPKGSTATE_SOURCE" "$build/libpkgstate" -Dhtml_docs=disabled
setup_dependency "$LIBPKGIMAGE_SOURCE" "$build/libpkgimage" -Dhtml_docs=disabled
setup_dependency "$LIBPKGCATALOG_SOURCE" "$build/libpkgcatalog" -Dhtml_docs=disabled
setup_dependency "$LIBPKGRESOLVE_SOURCE" "$build/libpkgresolve"
setup_dependency "$LIBPKGBUILD_SOURCE" "$build/libpkgbuild"
setup_dependency "$LIBPKGPLAN_SOURCE" "$build/libpkgplan" -Dreference_tools=disabled -Dhtml_docs=disabled
setup_dependency "$LIBPKGBUILD_IMAGE_SOURCE" "$build/libpkgbuild-image"
setup_dependency "$LIBPKGSOURCE_PLAN_SOURCE" "$build/libpkgsource-plan" -Dhtml_docs=disabled
setup_dependency "$LIBPKGBUILD_PLAN_SOURCE" "$build/libpkgbuild-plan"
setup_dependency "$LIBPKGAPPLY_SOURCE" "$build/libpkgapply" -Dhtml_docs=disabled
setup_dependency "$LIBPKGRECONCILE_SOURCE" "$build/libpkgreconcile"
setup_dependency "$LIBPKGAPPLY_POSIX_SOURCE" "$build/libpkgapply-posix" -Dhtml_docs=disabled
setup_dependency "$LIBPKGRECONCILE_APPLY_SOURCE" "$build/libpkgreconcile-apply"
setup_dependency "$LIBPKGRECONCILE_POSIX_SOURCE" "$build/libpkgreconcile-posix"

meson setup "$build/product" "$root" \
  --wrap-mode=nofallback --fatal-meson-warnings \
  --prefix="$install_prefix" --libdir=lib \
  -Ddefault_library="$link_mode" -Dlink_mode="$link_mode" \
  -Dtests=enabled -Dwerror=true "$@"
meson compile -C "$build/product"
for suite in unit integration header contract; do
  meson test -C "$build/product" --no-rebuild --suite "$suite" --print-errorlogs
done
meson test -C "$build/product" --no-rebuild --print-errorlogs
meson install -C "$build/product"
if [ "$link_mode" = shared ]; then
  "$root/ci/audit-shared-boundary.sh" "$install_prefix/lib/libpkgreconcile-apply-posix.so.0"
fi
printf '%s\n' "$dependency_prefix" >"$build/ci-dependency-prefix"
printf '%s\n' "$install_prefix" >"$build/ci-install-prefix"
