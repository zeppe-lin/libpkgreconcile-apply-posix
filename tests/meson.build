# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

if get_option('tests').allowed()
  sh = find_program('sh')

  error_test = executable(
    'publication-error-test',
    'unit/error_model_test.cpp',
    dependencies: libpkgreconcile_apply_posix_dep,
    cpp_args: ['-UNDEBUG'],
    install: false,
  )
  test('publication-error', error_test, suite: 'unit')

  # This suite proves the real provider composition. Planner/build dependencies
  # are qualification-only and do not enter the product dependency boundary.
  libpkgsource_test_dep = dependency('libpkgsource', version: ['>=4.0.0', '<5.0.0'], required: true, static: want_static)
  libpkgcatalog_test_dep = dependency('libpkgcatalog', version: ['>=4.0.0', '<5.0.0'], required: true, static: want_static)
  libpkgresolve_test_dep = dependency('libpkgresolve', version: ['>=4.0.0', '<5.0.0'], required: true, static: want_static)
  libpkgbuild_test_dep = dependency('libpkgbuild', version: ['>=3.0.1', '<4.0.0'], required: true, static: want_static)
  libpkgimage_test_dep = dependency('libpkgimage', version: ['>=0.4.1', '<1.0.0'], required: true, static: want_static)
  libpkgplan_test_dep = dependency('libpkgplan', version: ['>=0.3.1', '<1.0.0'], required: true, static: want_static)
  libpkgbuild_image_test_dep = dependency('libpkgbuild-image', version: ['>=1.0.1', '<2.0.0'], required: true, static: want_static)
  libpkgsource_plan_test_dep = dependency('libpkgsource-plan', version: ['>=2.0.0', '<3.0.0'], required: true, static: want_static)
  libpkgbuild_plan_test_dep = dependency('libpkgbuild-plan', version: ['>=1.1.0', '<2.0.0'], required: true, static: want_static)

  integration_test = executable(
    'verified-publication-integration-test',
    'integration/verified_publication_test.cpp',
    dependencies: [
      libpkgreconcile_apply_posix_dep,
      libpkgsource_test_dep,
      libpkgcatalog_test_dep,
      libpkgresolve_test_dep,
      libpkgbuild_test_dep,
      libpkgimage_test_dep,
      libpkgplan_test_dep,
      libpkgbuild_image_test_dep,
      libpkgsource_plan_test_dep,
      libpkgbuild_plan_test_dep,
    ],
    cpp_args: ['-UNDEBUG'],
    install: false,
  )
  test('verified-publication', integration_test, suite: 'integration')

  public_headers = [
    ['export', 'libpkgreconcile-apply-posix/export.h'],
    ['publication', 'libpkgreconcile-apply-posix/publication.h'],
    ['umbrella', 'libpkgreconcile-apply-posix/libpkgreconcile-apply-posix.h'],
  ]
  foreach item : public_headers
    target = executable(
      'header-' + item[0] + '-test',
      'header/public_header_test.cpp',
      dependencies: libpkgreconcile_apply_posix_dep,
      cpp_args: ['-DLIBPKGRECONCILE_APPLY_POSIX_TEST_HEADER="' + item[1] + '"'],
      install: false,
    )
    test('header-' + item[0], target, suite: 'header')
  endforeach

  if library_kind == 'shared'
    nm = find_program('nm')
    test(
      'abi-surface',
      sh,
      args: [
        meson.current_source_dir() / 'contracts/check_abi_surface.sh',
        nm.full_path(),
        pkg_reconcile_apply_posix.full_path(),
        meson.project_source_root() / 'abi/libpkgreconcile-apply-posix.exports',
      ],
      suite: 'contract',
    )
  endif

  foreach contract : [
    'architecture_contract',
    'ci_qualification_source',
    'documentation_source',
    'doxygen_contract',
    'release_metadata',
    'repository_contract',
    'style_contract',
    'test_layout',
  ]
    test(
      contract.replace('_', '-'),
      sh,
      args: [
        meson.current_source_dir() / ('contracts/check_' + contract + '.sh'),
        meson.project_source_root(),
      ],
      suite: 'contract',
    )
  endforeach

  test(
    'pkgconfig-metadata',
    sh,
    args: [
      meson.current_source_dir() / 'contracts/check_pkgconfig_metadata.sh',
      meson.project_build_root(),
      meson.project_version(),
    ],
    suite: 'contract',
  )
endif
