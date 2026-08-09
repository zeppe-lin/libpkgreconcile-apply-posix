// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file export.h
 *  @brief Shared-library visibility contract.
 */
#pragma once

#if defined _WIN32 || defined __CYGWIN__
#  ifdef LIBPKGRECONCILE_APPLY_POSIX_BUILD
#    define PKGRECONCILE_APPLY_POSIX_API __declspec(dllexport)
#  else
#    define PKGRECONCILE_APPLY_POSIX_API __declspec(dllimport)
#  endif
#else
#  define PKGRECONCILE_APPLY_POSIX_API __attribute__((visibility("default")))
#endif
