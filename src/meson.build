# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

inc = include_directories('../include')

elf_export_generator = find_program(
  meson.project_source_root() / 'scripts/generate-elf-export-script.sh',
)
elf_export_script_name = 'libpkgreconcile-apply-posix.exports.map'
elf_export_script_path = meson.current_build_dir() / elf_export_script_name
configure_file(
  input: meson.project_source_root() / 'abi/libpkgreconcile-apply-posix.exports',
  output: elf_export_script_name,
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)

link_args = []
if library_kind == 'shared'
  link_args += ['-Wl,--version-script=' + elf_export_script_path]
endif

pkg_reconcile_apply_posix = library(
  'pkgreconcile-apply-posix',
  'publication.cpp',
  include_directories: inc,
  dependencies: [
    libpkgreconcile_apply_dep,
    libpkgapply_posix_dep,
    libpkgreconcile_posix_dep,
  ],
  cpp_args: ['-DLIBPKGRECONCILE_APPLY_POSIX_BUILD'] + visibility_cpp_args,
  gnu_symbol_visibility: 'hidden',
  link_args: link_args,
  soversion: '0',
  version: meson.project_version(),
  install: true,
)

libpkgreconcile_apply_posix_dep = declare_dependency(
  link_with: pkg_reconcile_apply_posix,
  include_directories: inc,
  dependencies: [
    libpkgreconcile_apply_dep,
    libpkgapply_posix_dep,
    libpkgreconcile_posix_dep,
  ],
)

meson.override_dependency(
  'libpkgreconcile-apply-posix',
  libpkgreconcile_apply_posix_dep,
)

install_headers(
  '../include/libpkgreconcile-apply-posix/export.h',
  '../include/libpkgreconcile-apply-posix/publication.h',
  '../include/libpkgreconcile-apply-posix/libpkgreconcile-apply-posix.h',
  subdir: 'libpkgreconcile-apply-posix',
)

pkgconf.generate(
  pkg_reconcile_apply_posix,
  name: 'libpkgreconcile-apply-posix',
  description: 'Verified POSIX publication of completed rejected evidence',
  version: meson.project_version(),
  requires: [
    libpkgreconcile_apply_dep,
    libpkgapply_posix_dep,
    libpkgreconcile_posix_dep,
  ],
)
