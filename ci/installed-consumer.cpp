// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgreconcile-apply-posix/libpkgreconcile-apply-posix.h>

#include <type_traits>

int main()
{
  static_assert(std::is_base_of_v<std::runtime_error,
                                  pkgreconcile::apply_posix::publication_error>);
  return 0;
}
