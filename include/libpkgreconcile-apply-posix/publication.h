// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file publication.h
 *  @brief Verified POSIX publication of projected rejected evidence.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgapply-posix/rejected_store.h>
#include <libpkgreconcile-apply/adapter.h>
#include <libpkgreconcile-posix/inventory_store.h>
#include <libpkgreconcile-apply-posix/export.h>

namespace pkgreconcile::apply_posix {

/** Stable semantic reason that verified publication was refused. */
enum class publication_error_code : std::uint8_t {
  target_binding_mismatch = 1,       ///< Reconciliation store names another target.
  locator_invalid = 2,               ///< Projected locator is not adapter-owned/valid.
  rejected_store_binding_mismatch = 3, ///< Locator names another routed rejected store.
  rejected_object_missing = 4,       ///< Exact rejected record cannot be reopened.
  rejected_object_identity_mismatch = 5, ///< Reopened object reports another identity.
  rejected_object_path_mismatch = 6, ///< Reopened observation names another path.
  rejected_object_side_mismatch = 7, ///< Reopened provenance disagrees with projection.
  publication_refused = 8,           ///< Durable inventory refused the verified batch.
  rejected_object_plan_mismatch = 9, ///< Reopened record belongs to another plan.
  rejected_object_attempt_mismatch = 10, ///< Reopened record belongs to another attempt.
};

/** Typed semantic refusal at the apply-POSIX reconciliation boundary. */
class PKGRECONCILE_APPLY_POSIX_API publication_error final
    : public std::runtime_error {
public:
  /**
   * Construct one typed publication refusal.
   * @param code Stable refusal category.
   * @param message Human-readable diagnostic.
   */
  publication_error(publication_error_code code, std::string message);

  /** Destroy the polymorphic refusal. */
  ~publication_error() override;

  /** @return Stable refusal category. */
  [[nodiscard]] publication_error_code code() const noexcept;

private:
  publication_error_code code_;
};

/**
 * Validate projected rejected evidence against one routed POSIX rejected store
 * and atomically publish the verified batch into one POSIX reconciliation store.
 *
 * @param projection Generic completed-application reconciliation projection.
 * @param routed_store_identity Exact rejected-store identity already resolved by
 *        orchestration to @p rejected_store.
 * @param rejected_store Already-authorized descriptor-anchored rejected store.
 * @param reconciliation_store Durable store bound to @p projection's target.
 * @return Durable publication receipt for the entire verified batch.
 * @throws publication_error for semantic target, routing, locator, object,
 *         provenance, path, or inventory-admission mismatch.
 * @throws pkgapply::posix::rejected_store_error when concrete rejected evidence
 *         cannot be validated because provider state is corrupt or unavailable.
 * @throws pkgreconcile::posix::store_error when reconciliation persistence
 *         cannot be read, locked, or durably updated.
 *
 * Every projected tuple is reopened and validated before any reconciliation
 * generation is published. Reopened records must belong to the exact operation
 * plan and physical application attempt retained by the projection. The
 * function does not derive store identity from a
 * pathname or descriptor: @p routed_store_identity is the caller's explicit
 * routing assertion for the already-authorized store handle.
 */
[[nodiscard]] PKGRECONCILE_APPLY_POSIX_API
pkgreconcile::posix::pending_publication_receipt publish_verified_projection(
    const pkgreconcile::apply_adapter::completed_reconciliation_projection& projection,
    const pkgapply::rejected_object_store_identity& routed_store_identity,
    const pkgapply::posix::application_rejected_object_store& rejected_store,
    pkgreconcile::posix::inventory_generation_store& reconciliation_store);

} // namespace pkgreconcile::apply_posix
