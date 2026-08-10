# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

project(
  'libpkgreconcile-apply-posix',
  'cpp',
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=1.6.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'default_library=shared',
    'b_ndebug=if-release'
  ],
)

library_kind = get_option('default_library')
if library_kind == 'both'
  error('libpkgreconcile-apply-posix requires separate shared and static builds')
endif

want_static = get_option('link_mode') == 'static'
if (library_kind == 'static') != want_static
  error('default_library and link_mode must both select shared or static')
endif

cpp = meson.get_compiler('cpp')

visibility_cpp_args = []
if cpp.has_argument('-fvisibility-inlines-hidden')
  visibility_cpp_args += ['-fvisibility-inlines-hidden']
endif

pkgconf = import('pkgconfig')

libpkgreconcile_apply_dep = dependency(
  'libpkgreconcile-apply',
  version: '>=0.1.0',
  required: true,
  static: want_static
)

libpkgapply_posix_dep = dependency(
  'libpkgapply-posix',
  version: '>=3.2.0',
  required: true,
  static: want_static
)

libpkgreconcile_posix_dep = dependency(
  'libpkgreconcile-posix',
  version: '>=0.1.0',
  required: true,
  static: want_static
)

subdir('src')
subdir('man')
subdir('tests')
