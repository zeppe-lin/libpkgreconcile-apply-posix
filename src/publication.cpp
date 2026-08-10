// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgreconcile-apply-posix/publication.h>

#include <filesystem>
#include <utility>
#include <vector>

namespace pkgreconcile::apply_posix {
namespace {

publication_error refuse(publication_error_code code, const char* message)
{
  return publication_error(code, message);
}

pkgapply::posix::rejected_object_source expected_source(rejected_object_side side)
{
  switch (side) {
    case rejected_object_side::incoming:
      return pkgapply::posix::rejected_object_source::incoming;
    case rejected_object_side::prior_installed:
      return pkgapply::posix::rejected_object_source::old;
  }
  throw refuse(publication_error_code::rejected_object_side_mismatch,
               "projected rejected side is not supported by the POSIX provider");
}

bool same_path(const std::filesystem::path& projected,
               const pkgplan::package_path& observed)
{
  return projected.native() == observed.string();
}

} // namespace

publication_error::publication_error(publication_error_code code,
                                     std::string message)
    : std::runtime_error(std::move(message)), code_(code)
{
}

publication_error::~publication_error() = default;

publication_error_code publication_error::code() const noexcept
{
  return code_;
}

pkgreconcile::posix::pending_publication_receipt publish_verified_projection(
    const pkgreconcile::apply_adapter::completed_reconciliation_projection& projection,
    const pkgapply::rejected_object_store_identity& routed_store_identity,
    const pkgapply::posix::application_rejected_object_store& rejected_store,
    pkgreconcile::posix::inventory_generation_store& reconciliation_store)
{
  if (reconciliation_store.target_binding() != projection.target()) {
    throw refuse(publication_error_code::target_binding_mismatch,
                 "reconciliation store is bound to another managed target");
  }

  std::vector<pkgapply::posix::identified_rejected_object> verified;
  verified.reserve(projection.pending().size());

  for (const auto& pending : projection.pending()) {
    if (pending.target() != projection.target()) {
      throw refuse(publication_error_code::target_binding_mismatch,
                   "projected pending value names another managed target");
    }

    pkgreconcile::apply_adapter::rejected_object_reference reference = [&] {
      try {
        return pkgreconcile::apply_adapter::decode_rejected_object_locator(
            pending.object());
      } catch (const pkgreconcile::apply_adapter::projection_error&) {
        throw refuse(publication_error_code::locator_invalid,
                     "projected rejected-object locator is invalid");
      }
    }();

    if (reference.store() != routed_store_identity) {
      throw refuse(publication_error_code::rejected_store_binding_mismatch,
                   "projected object names another rejected store");
    }

    auto object = rejected_store.load_identified(reference.record());
    if (!object) {
      throw refuse(publication_error_code::rejected_object_missing,
                   "projected rejected object is not present in the routed store");
    }

    if (object->identity() != reference.record()) {
      throw refuse(publication_error_code::rejected_object_identity_mismatch,
                   "reopened rejected object reports another record identity");
    }

    if (!same_path(pending.path(), object->observation().path())) {
      throw refuse(publication_error_code::rejected_object_path_mismatch,
                   "reopened rejected object names another path");
    }

    if (object->source() != expected_source(pending.side())) {
      throw refuse(publication_error_code::rejected_object_side_mismatch,
                   "reopened rejected-object provenance disagrees with projection");
    }

    verified.push_back(std::move(*object));
  }

  for (const auto& object : verified) {
    if (object.plan() != projection.plan()) {
      throw refuse(publication_error_code::rejected_object_plan_mismatch,
                   "reopened rejected object belongs to another operation plan");
    }

    if (object.attempt() != projection.attempt()) {
      throw refuse(publication_error_code::rejected_object_attempt_mismatch,
                   "reopened rejected object belongs to another application attempt");
    }
  }

  try {
    return reconciliation_store.publish_pending(projection.pending());
  } catch (const std::invalid_argument&) {
    throw refuse(publication_error_code::publication_refused,
                 "verified projection was refused by the reconciliation inventory");
  }
}

} // namespace pkgreconcile::apply_posix
