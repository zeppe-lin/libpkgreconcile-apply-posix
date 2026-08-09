// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/test.hpp"

#include <libpkgreconcile-apply-posix/publication.h>

#include <string>

int main()
{
  using namespace pkgreconcile::apply_posix;
  test_support::runner runner;

  runner.run("retain stable publication error code", [] {
    publication_error error(
        publication_error_code::rejected_object_side_mismatch,
        "side mismatch");
    TEST_CHECK(error.code() ==
               publication_error_code::rejected_object_side_mismatch);
    TEST_CHECK(std::string(error.what()) == "side mismatch");
  });

  runner.run("publication error is runtime failure", [] {
    bool caught = false;
    try {
      throw publication_error(publication_error_code::rejected_object_missing,
                              "missing");
    } catch (const std::runtime_error&) {
      caught = true;
    }
    TEST_CHECK(caught);
  });

  return runner.finish();
}
