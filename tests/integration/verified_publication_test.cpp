// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/plan.h"
#include "../support/temp_directory.hpp"
#include "../support/test.hpp"

#include <libpkgapply-posix/capture_store.h>
#include <libpkgapply-posix/rejected_store.h>
#include <libpkgapply-posix/target_observer.h>
#include <libpkgreconcile-apply-posix/publication.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

template<class Identity>
Identity app_identity(std::uint8_t seed)
{
  std::string text = "v1:sha256:";
  constexpr char hex[] = "0123456789abcdef";
  for (std::size_t i = 0; i < 32; ++i) {
    const auto byte = static_cast<std::uint8_t>(seed + i);
    text.push_back(hex[(byte >> 4U) & 0x0fU]);
    text.push_back(hex[byte & 0x0fU]);
  }
  return Identity::parse(text);
}

template<class Identity>
Identity plan_identity(std::uint8_t seed)
{
  std::array<std::uint8_t, 32> bytes{};
  for (std::size_t i = 0; i < bytes.size(); ++i)
    bytes[i] = static_cast<std::uint8_t>(seed + i);
  return Identity::from_sha256(bytes);
}

pkgapply::application_target_context target(std::uint8_t seed = 1)
{
  return pkgapply::application_target_context::make(
      plan_identity<pkgplan::target_system_context_identity>(seed),
      app_identity<pkgapply::managed_target_identity>(seed + 1),
      app_identity<pkgapply::root_view_identity>(seed + 2),
      app_identity<pkgapply::observation_backend_identity>(seed + 3),
      app_identity<pkgapply::mutation_backend_identity>(seed + 4),
      app_identity<pkgapply::mutation_exclusion_domain_identity>(seed + 5),
      app_identity<pkgapply::active_object_namespace_identity>(seed + 6),
      app_identity<pkgapply::rejected_object_store_identity>(seed + 7),
      app_identity<pkgapply::staging_namespace_identity>(seed + 8),
      app_identity<pkgapply::journal_namespace_identity>(seed + 9),
      app_identity<pkgapply::execution_capability_profile_identity>(seed + 10));
}

pkgapply::application_execution_control control()
{
  return pkgapply::application_execution_control::make(
      pkgapply::application_recovery_requirement::best_effort,
      pkgapply::application_durability_requirement::all_application_domains,
      pkgapply::application_cancellation_policy::recover_after_target_mutation);
}

pkgapply::application_durability_profile durability()
{
  using domain = pkgapply::application_durability_domain;
  using status = pkgapply::application_durability_status;
  return pkgapply::application_durability_profile({
      {domain::journal, status::confirmed},
      {domain::incoming_staging, status::confirmed},
      {domain::recovery_staging, status::confirmed},
      {domain::active_namespace, status::confirmed},
      {domain::rejected_object_store, status::confirmed},
      {domain::completed_evidence, status::confirmed},
  });
}

pkgapply::application_attempt attempt(
    const pkgapply::application_request_identity& request,
    const pkgapply::application_target_context& context,
    std::uint8_t seed)
{
  pkgapply::application_attempt_nonce::byte_array nonce{};
  for (std::size_t i = 0; i < nonce.size(); ++i)
    nonce[i] = static_cast<std::uint8_t>(seed + i);
  return pkgapply::application_attempt::make(
      request, context.identity(), context.mutation_backend(),
      pkgapply::application_attempt_nonce::from_bytes(nonce));
}

pkgapply::completed_object_fact regular(
    const pkgplan::package_path& path, std::uint8_t content)
{
  return pkgapply::completed_object_fact(
      path,
      pkgapply::completed_object_kind::regular,
      pkgapply::qualified_fact<std::uint32_t>::known(0644),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(4),
      pkgapply::qualified_fact<pkgapply::completed_object_timestamp>::known(
          {10, 0}),
      pkgapply::qualified_fact<pkgapply::completed_regular_content_identity>::known(
          app_identity<pkgapply::completed_regular_content_identity>(content)),
      pkgapply::qualified_fact<std::string>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_device_number>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_hardlink_relation>::unknown(),
      pkgapply::object_fact_provenance::application_observation,
      pkgapply::object_fact_completeness::complete);
}

pkgimage::package_entry incoming_symlink_entry(
    std::string path, std::string target = "new-target")
{
  pkgimage::package_entry entry(
      pkgimage::package_path::parse(std::move(path)),
      pkgimage::entry_type::symlink);
  entry.mode = 0777;
  entry.uid = 0;
  entry.gid = 0;
  entry.mtime = 102;
  entry.mtime_nanoseconds = 202;
  entry.symlink_target = std::move(target);
  return entry;
}

pkgplan::filesystem_object_metadata symlink_object(std::string target)
{
  return pkgplan::filesystem_object_metadata(
      pkgplan::filesystem_object_kind::symlink,
      0777,
      0,
      0,
      std::nullopt,
      pkgplan::object_timestamp(101, 201),
      std::nullopt,
      std::move(target));
}

pkgapply::completed_object_fact symlink(
    const pkgplan::package_path& path, std::string target)
{
  return pkgapply::completed_object_fact(
      path,
      pkgapply::completed_object_kind::symlink,
      pkgapply::qualified_fact<std::uint32_t>::known(0777),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_object_timestamp>::known(
          {101, 201}),
      pkgapply::qualified_fact<pkgapply::completed_regular_content_identity>::not_applicable(),
      pkgapply::qualified_fact<std::string>::known(std::move(target)),
      pkgapply::qualified_fact<pkgapply::completed_device_number>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_hardlink_relation>::not_applicable(),
      pkgapply::object_fact_provenance::application_observation,
      pkgapply::object_fact_completeness::complete);
}

void write_file(const std::string& path, std::string_view bytes)
{
  const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                        0644);
  if (fd < 0)
    throw std::runtime_error("cannot create provider-composition fixture file");
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const ssize_t count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0) {
      ::close(fd);
      throw std::runtime_error("cannot write provider-composition fixture file");
    }
    offset += static_cast<std::size_t>(count);
  }
  if (::close(fd) != 0)
    throw std::runtime_error("cannot close provider-composition fixture file");
}

const pkgapply::application_path_observation& find_observation(
    const pkgapply::backend_observation_batch& batch,
    const pkgplan::package_path& path)
{
  const auto* value = batch.find(path);
  if (value == nullptr)
    throw std::runtime_error("provider-composition observation is missing");
  return *value;
}

class scoped_nofile_limit final {
public:
  explicit scoped_nofile_limit(rlim_t maximum)
  {
    if (::getrlimit(RLIMIT_NOFILE, &previous_) != 0)
      throw std::runtime_error("cannot read descriptor limit");

    if (previous_.rlim_cur <= maximum)
      return;

    auto limited = previous_;
    limited.rlim_cur = maximum;
    if (::setrlimit(RLIMIT_NOFILE, &limited) != 0)
      throw std::runtime_error("cannot lower descriptor limit");
    changed_ = true;
  }

  scoped_nofile_limit(const scoped_nofile_limit&) = delete;
  scoped_nofile_limit& operator=(const scoped_nofile_limit&) = delete;

  ~scoped_nofile_limit()
  {
    if (changed_)
      static_cast<void>(::setrlimit(RLIMIT_NOFILE, &previous_));
  }

private:
  struct rlimit previous_ {};
  bool changed_ = false;
};

struct incoming_case final {
  pkgreconcile::apply_adapter::completed_reconciliation_projection projection;
  pkgapply::rejected_object_record_identity record;
};

incoming_case make_incoming_case(
    const pkgapply::application_target_context& context,
    pkgapply::posix::application_rejected_object_store& store,
    std::string path_text,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  const auto path = pkgplan::package_path::parse(path_text);
  planning_authorities authorities(context.target());
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::retain(
          pkgplan::rejected_object_policy::stage,
          pkgplan::retained_active_ownership_policy::do_not_claim_operated_package)));
  std::vector<pkgimage::package_entry> entries{
      incoming_symlink_entry(path.string())};
  const auto active = symlink_object("old-target");
  const auto digest = archive_digest(static_cast<std::uint8_t>(seed + 2));
  const auto plan = upgrade_plan(
      authorities,
      entries,
      {pkgplan::target_path_observation::present(
          pkgplan::filesystem_object_fact(path, active))},
      {pkgplan::installed_ownership_claim(path, authorities.installed_package, active)},
      std::move(policy),
      digest);
  const auto request = pkgapply::upgrade_application_request::make(
      plan,
      incoming_package(entries, digest, "2.0"),
      context,
      control());
  const auto physical_attempt = attempt(request.identity(), context, seed);
  const pkgimage::package_image image(entries);
  const auto rejected_effect = pkgapply::test::fixture::rejected_request(plan, path);
  const auto published = store.publish_incoming(
      physical_attempt, plan.identity(), rejected_effect, image);
  if (!published.record())
    throw std::runtime_error("incoming rejected object was not published");

  const auto before = pkgapply::application_path_observation::present(
      symlink(path, "old-target"));
  const auto& decision = plan.paths().front();
  auto consequence = pkgapply::application_path_consequence(
      path,
      pkgapply::application_path_role::incoming_entry,
      decision.active(),
      decision.rejected(),
      decision.incoming_entry(),
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::completed,
      before,
      before,
      *published.record(),
      pkgapply::ownership_publication_status::eligible);
  const auto evidence = pkgapply::completed_application_evidence::upgrade(
      request,
      physical_attempt.identity(),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 4),
      app_identity<pkgapply::application_journal_identity>(seed + 5),
      std::vector<pkgapply::application_path_consequence>{std::move(consequence)},
      durability());
  return {
      pkgreconcile::apply_adapter::project_completed_application(context, evidence),
      *published.record()};
}

pkgreconcile::apply_adapter::completed_reconciliation_projection
make_incoming_batch_case(
    const pkgapply::application_target_context& context,
    pkgapply::posix::application_rejected_object_store& store,
    std::size_t count,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  planning_authorities authorities(context.target());
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::retain(
          pkgplan::rejected_object_policy::stage,
          pkgplan::retained_active_ownership_policy::do_not_claim_operated_package)));

  std::vector<pkgimage::package_entry> entries;
  std::vector<pkgplan::target_path_observation> observed;
  std::vector<pkgplan::installed_ownership_claim> claims;
  entries.reserve(count);
  observed.reserve(count);
  claims.reserve(count);

  const auto active = symlink_object("old-target");
  for (std::size_t i = 0; i < count; ++i) {
    const auto path = pkgplan::package_path::parse(
        "bulk-item-" + std::to_string(i));
    entries.push_back(incoming_symlink_entry(path.string()));
    observed.push_back(pkgplan::target_path_observation::present(
        pkgplan::filesystem_object_fact(path, active)));
    claims.emplace_back(path, authorities.installed_package, active);
  }

  const auto digest = archive_digest(static_cast<std::uint8_t>(seed + 2));
  const auto plan = upgrade_plan(
      authorities, entries, std::move(observed), std::move(claims),
      std::move(policy), digest);
  const auto request = pkgapply::upgrade_application_request::make(
      plan, incoming_package(entries, digest, "2.0"), context, control());
  const auto physical_attempt = attempt(request.identity(), context, seed);
  const pkgimage::package_image image(entries);

  std::vector<pkgapply::application_path_consequence> consequences;
  consequences.reserve(plan.paths().size());
  for (const auto& decision : plan.paths()) {
    const auto rejected_effect =
        pkgapply::test::fixture::rejected_request(plan, decision.path());
    const auto published = store.publish_incoming(
        physical_attempt, plan.identity(), rejected_effect, image);
    if (!published.record())
      throw std::runtime_error("batch rejected object was not published");

    const auto before = pkgapply::application_path_observation::present(
        symlink(decision.path(), "old-target"));
    consequences.emplace_back(
        decision.path(),
        pkgapply::application_path_role::incoming_entry,
        decision.active(),
        decision.rejected(),
        decision.incoming_entry(),
        decision.ownership(),
        pkgapply::application_effect_status::completed,
        pkgapply::application_effect_status::completed,
        before,
        before,
        *published.record(),
        pkgapply::ownership_publication_status::eligible);
  }

  const auto evidence = pkgapply::completed_application_evidence::upgrade(
      request,
      physical_attempt.identity(),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 4),
      app_identity<pkgapply::application_journal_identity>(seed + 5),
      std::move(consequences),
      durability());
  return pkgreconcile::apply_adapter::project_completed_application(
      context, evidence);
}

pkgreconcile::apply_adapter::completed_reconciliation_projection
project_incoming_record(
    const pkgapply::application_target_context& context,
    std::string path_text,
    pkgapply::rejected_object_record_identity record,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  const auto path = pkgplan::package_path::parse(path_text);
  planning_authorities authorities(context.target());
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::retain(
          pkgplan::rejected_object_policy::stage,
          pkgplan::retained_active_ownership_policy::do_not_claim_operated_package)));
  std::vector<pkgimage::package_entry> entries{
      incoming_symlink_entry(path.string())};
  const auto active = symlink_object("old-target");
  const auto digest = archive_digest(static_cast<std::uint8_t>(seed + 2));
  const auto plan = upgrade_plan(
      authorities,
      entries,
      {pkgplan::target_path_observation::present(
          pkgplan::filesystem_object_fact(path, active))},
      {pkgplan::installed_ownership_claim(path, authorities.installed_package, active)},
      std::move(policy),
      digest);
  const auto request = pkgapply::upgrade_application_request::make(
      plan,
      incoming_package(entries, digest, "2.0"),
      context,
      control());
  const auto before = pkgapply::application_path_observation::present(
      symlink(path, "old-target"));
  const auto& decision = plan.paths().front();
  auto consequence = pkgapply::application_path_consequence(
      path,
      pkgapply::application_path_role::incoming_entry,
      decision.active(),
      decision.rejected(),
      decision.incoming_entry(),
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::completed,
      before,
      before,
      std::move(record),
      pkgapply::ownership_publication_status::eligible);
  const auto evidence = pkgapply::completed_application_evidence::upgrade(
      request,
      app_identity<pkgapply::application_attempt_identity>(seed + 4),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 5),
      app_identity<pkgapply::application_journal_identity>(seed + 6),
      std::vector<pkgapply::application_path_consequence>{std::move(consequence)},
      durability());
  return pkgreconcile::apply_adapter::project_completed_application(context, evidence);
}


pkgreconcile::apply_adapter::completed_reconciliation_projection
project_incoming_records(
    const pkgapply::application_target_context& context,
    const std::vector<std::pair<std::string, pkgapply::rejected_object_record_identity>>& records,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  planning_authorities authorities(context.target());
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::retain(
          pkgplan::rejected_object_policy::stage,
          pkgplan::retained_active_ownership_policy::do_not_claim_operated_package)));

  std::vector<pkgimage::package_entry> entries;
  std::vector<pkgplan::target_path_observation> observed;
  std::vector<pkgplan::installed_ownership_claim> claims;
  entries.reserve(records.size());
  observed.reserve(records.size());
  claims.reserve(records.size());
  const auto active = symlink_object("old-target");
  for (const auto& item : records) {
    const auto path = pkgplan::package_path::parse(item.first);
    entries.push_back(incoming_symlink_entry(path.string()));
    observed.push_back(pkgplan::target_path_observation::present(
        pkgplan::filesystem_object_fact(path, active)));
    claims.emplace_back(path, authorities.installed_package, active);
  }

  const auto digest = archive_digest(static_cast<std::uint8_t>(seed + 2));
  const auto plan = upgrade_plan(
      authorities, entries, std::move(observed), std::move(claims),
      std::move(policy), digest);
  const auto request = pkgapply::upgrade_application_request::make(
      plan,
      incoming_package(entries, digest, "2.0"),
      context,
      control());

  std::vector<pkgapply::application_path_consequence> consequences;
  consequences.reserve(plan.paths().size());
  for (const auto& decision : plan.paths()) {
    const auto item = std::find_if(
        records.begin(), records.end(), [&](const auto& candidate) {
          return candidate.first == decision.path().string();
        });
    if (item == records.end())
      throw std::runtime_error("multi-path projection record is missing");
    const auto before = pkgapply::application_path_observation::present(
        symlink(decision.path(), "old-target"));
    consequences.emplace_back(
        decision.path(),
        pkgapply::application_path_role::incoming_entry,
        decision.active(),
        decision.rejected(),
        decision.incoming_entry(),
        decision.ownership(),
        pkgapply::application_effect_status::completed,
        pkgapply::application_effect_status::completed,
        before,
        before,
        item->second,
        pkgapply::ownership_publication_status::eligible);
  }

  const auto evidence = pkgapply::completed_application_evidence::upgrade(
      request,
      app_identity<pkgapply::application_attempt_identity>(seed + 4),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 5),
      app_identity<pkgapply::application_journal_identity>(seed + 6),
      std::move(consequences),
      durability());
  return pkgreconcile::apply_adapter::project_completed_application(context, evidence);
}

pkgreconcile::apply_adapter::completed_reconciliation_projection
project_old_record(
    const pkgapply::application_target_context& context,
    std::string path_text,
    pkgapply::rejected_object_record_identity record,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  const auto path = pkgplan::package_path::parse(path_text);
  planning_authorities authorities(context.target());
  const auto active = regular_object(static_cast<std::uint8_t>(seed + 1));
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::activate(),
                  pkgplan::obsolete_path_policy::remove(
                      pkgplan::rejected_object_policy::stage)));
  const auto plan = removal_plan(
      authorities,
      {pkgplan::installed_ownership_claim(path, authorities.installed_package, active)},
      {pkgplan::target_path_observation::present(
          pkgplan::filesystem_object_fact(path, active))},
      std::move(policy));
  const auto request = pkgapply::removal_application_request::make(
      plan, context, control());
  const auto before = pkgapply::application_path_observation::present(
      regular(path, static_cast<std::uint8_t>(seed + 2)));
  const auto& decision = plan.paths().front();
  auto consequence = pkgapply::application_path_consequence(
      path,
      pkgapply::application_path_role::installed_owned_path,
      decision.active(),
      decision.rejected(),
      std::nullopt,
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::completed,
      before,
      pkgapply::application_path_observation::absent(path),
      std::move(record),
      pkgapply::ownership_publication_status::eligible);
  const auto evidence = pkgapply::completed_application_evidence::removal(
      request,
      app_identity<pkgapply::application_attempt_identity>(seed + 3),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 4),
      app_identity<pkgapply::application_journal_identity>(seed + 5),
      std::vector<pkgapply::application_path_consequence>{std::move(consequence)},
      durability());
  return pkgreconcile::apply_adapter::project_completed_application(context, evidence);
}

struct old_case final {
  pkgreconcile::apply_adapter::completed_reconciliation_projection projection;
  pkgapply::rejected_object_record_identity record;
};

old_case make_old_case(
    const pkgapply::application_target_context& context,
    pkgapply::posix::application_rejected_object_store& rejected_store,
    const std::filesystem::path& target_root,
    const std::filesystem::path& capture_root,
    std::string path_text,
    std::uint8_t seed)
{
  using namespace pkgapply::test::fixture;
  const auto path = pkgplan::package_path::parse(path_text);
  const auto parent = target_root / std::filesystem::path(path_text).parent_path();
  std::filesystem::create_directories(parent);
  write_file((target_root / path_text).string(), "old bytes");

  auto observer = pkgapply::posix::application_target_observer::open(
      target_root.string());
  const auto observations = observer.observe({path});

  planning_authorities authorities(context.target());
  const auto active = regular_object(static_cast<std::uint8_t>(seed + 1));
  auto policy = policy_snapshot(
      authorities,
      path_policy(pkgplan::incoming_path_policy::activate(),
                  pkgplan::obsolete_path_policy::remove(
                      pkgplan::rejected_object_policy::stage)));
  const auto plan = removal_plan(
      authorities,
      {pkgplan::installed_ownership_claim(path, authorities.installed_package, active)},
      {pkgplan::target_path_observation::present(
          pkgplan::filesystem_object_fact(path, active))},
      std::move(policy));
  const auto request = pkgapply::removal_application_request::make(
      plan, context, control());
  const auto physical_attempt = attempt(request.identity(), context, seed);

  auto capture_store = pkgapply::posix::application_capture_store::open(
      capture_root.string(), target_root.string());
  const pkgapply::old_object_capture_request capture_request(path, true, true);
  const auto& admitted = find_observation(observations, path);
  const auto capture_result = capture_store.capture(
      physical_attempt, capture_request, admitted);
  if (capture_result.outcome() != pkgapply::backend_operation_outcome::completed)
    throw std::runtime_error("old-object capture did not complete");
  auto captured = capture_store.load(
      physical_attempt, capture_request, admitted);
  if (!captured)
    throw std::runtime_error("captured old object disappeared");

  const auto rejected_effect = pkgapply::test::fixture::rejected_request(plan, path);
  const auto published = rejected_store.publish_old(
      physical_attempt, plan.identity(), rejected_effect, *captured);
  if (!published.record())
    throw std::runtime_error("old rejected object was not published");

  const auto& decision = plan.paths().front();
  auto consequence = pkgapply::application_path_consequence(
      path,
      pkgapply::application_path_role::installed_owned_path,
      decision.active(),
      decision.rejected(),
      std::nullopt,
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::completed,
      admitted,
      pkgapply::application_path_observation::absent(path),
      *published.record(),
      pkgapply::ownership_publication_status::eligible);
  const auto evidence = pkgapply::completed_application_evidence::removal(
      request,
      physical_attempt.identity(),
      app_identity<pkgapply::lease_bound_state_projection_identity>(seed + 2),
      app_identity<pkgapply::application_journal_identity>(seed + 3),
      std::vector<pkgapply::application_path_consequence>{std::move(consequence)},
      durability());
  return {
      pkgreconcile::apply_adapter::project_completed_application(context, evidence),
      *published.record()};
}

void require_publication_error(
    const std::function<void()>& action,
    pkgreconcile::apply_posix::publication_error_code expected)
{
  try {
    action();
    TEST_CHECK(false);
  } catch (const pkgreconcile::apply_posix::publication_error& error) {
    TEST_CHECK(error.code() == expected);
  }
}

} // namespace

int main()
{
  using namespace pkgreconcile::apply_posix;
  test_support::runner runner;

  test_support::temp_directory rejected_root;
  test_support::temp_directory inventory_root;
  auto context = target(1);
  auto rejected_store = pkgapply::posix::application_rejected_object_store::open(
      rejected_root.path().string());
  auto incoming = make_incoming_case(
      context, rejected_store, "tool", 30);
  pkgreconcile::posix::inventory_generation_store inventory(
      inventory_root.path(), incoming.projection.target());

  runner.run("verified incoming evidence publishes atomically", [&] {
    const auto receipt = publish_verified_projection(
        incoming.projection, context.rejected_store(), rejected_store, inventory);
    TEST_CHECK(receipt.published() == 1U);
    TEST_CHECK(receipt.already_pending() == 0U);
    TEST_CHECK(receipt.suppressed_resolved() == 0U);
    TEST_CHECK(receipt.changed());
    const auto snapshot = inventory.read();
    const auto* record = snapshot.find(incoming.projection.pending().front());
    TEST_CHECK(record != nullptr);
    TEST_CHECK(record->status() == pkgreconcile::reconciliation_record_status::pending);
  });

  runner.run("exact republication is already pending", [&] {
    const auto receipt = publish_verified_projection(
        incoming.projection, context.rejected_store(), rejected_store, inventory);
    TEST_CHECK(receipt.published() == 0U);
    TEST_CHECK(receipt.already_pending() == 1U);
    TEST_CHECK(!receipt.changed());
  });

  runner.run("resolved tuple is not resurrected", [&] {
    TEST_CHECK(inventory.resolve(incoming.projection.pending().front()) ==
               pkgreconcile::posix::resolution_outcome::resolved);
    const auto receipt = publish_verified_projection(
        incoming.projection, context.rejected_store(), rejected_store, inventory);
    TEST_CHECK(receipt.published() == 0U);
    TEST_CHECK(receipt.suppressed_resolved() == 1U);
    TEST_CHECK(!receipt.changed());
    const auto snapshot = inventory.read();
    const auto* record = snapshot.find(incoming.projection.pending().front());
    TEST_CHECK(record != nullptr);
    TEST_CHECK(record->status() == pkgreconcile::reconciliation_record_status::resolved);
  });

  runner.run("foreign reconciliation target is refused", [&] {
    test_support::temp_directory other_root;
    const auto other_context = target(90);
    pkgreconcile::posix::inventory_generation_store other_inventory(
        other_root.path(),
        pkgreconcile::apply_adapter::project_managed_target(
            other_context.managed_target()));
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              incoming.projection, context.rejected_store(), rejected_store,
              other_inventory);
        },
        publication_error_code::target_binding_mismatch);
  });

  runner.run("foreign routed rejected store is refused", [&] {
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              incoming.projection,
              app_identity<pkgapply::rejected_object_store_identity>(200),
              rejected_store,
              inventory);
        },
        publication_error_code::rejected_store_binding_mismatch);
  });

  runner.run("missing rejected record is refused", [&] {
    test_support::temp_directory missing_inventory_root;
    const auto projection = project_incoming_record(
        context,
        "missing",
        app_identity<pkgapply::rejected_object_record_identity>(210),
        50);
    pkgreconcile::posix::inventory_generation_store missing_inventory(
        missing_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              missing_inventory);
        },
        publication_error_code::rejected_object_missing);
    TEST_CHECK(missing_inventory.read().size() == 0U);
  });


  runner.run("reopened plan must match completed projection", [&] {
    test_support::temp_directory mismatch_inventory_root;
    const auto projection = project_incoming_record(
        context, "tool", incoming.record, 60);
    TEST_CHECK(projection.plan() != incoming.projection.plan());
    pkgreconcile::posix::inventory_generation_store mismatch_inventory(
        mismatch_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              mismatch_inventory);
        },
        publication_error_code::rejected_object_plan_mismatch);
    TEST_CHECK(mismatch_inventory.read().size() == 0U);
  });

  runner.run("reopened attempt must match completed projection", [&] {
    test_support::temp_directory mismatch_inventory_root;
    const auto projection = project_incoming_record(
        context, "tool", incoming.record, 30);
    TEST_CHECK(projection.plan() == incoming.projection.plan());
    TEST_CHECK(projection.attempt() != incoming.projection.attempt());
    pkgreconcile::posix::inventory_generation_store mismatch_inventory(
        mismatch_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              mismatch_inventory);
        },
        publication_error_code::rejected_object_attempt_mismatch);
    TEST_CHECK(mismatch_inventory.read().size() == 0U);
  });

  runner.run("reopened path must match projected path", [&] {
    test_support::temp_directory mismatch_inventory_root;
    const auto projection = project_incoming_record(
        context, "other", incoming.record, 60);
    pkgreconcile::posix::inventory_generation_store mismatch_inventory(
        mismatch_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              mismatch_inventory);
        },
        publication_error_code::rejected_object_path_mismatch);
    TEST_CHECK(mismatch_inventory.read().size() == 0U);
  });

  runner.run("reopened source must match projected side", [&] {
    test_support::temp_directory mismatch_inventory_root;
    const auto projection = project_old_record(
        context, "tool", incoming.record, 70);
    pkgreconcile::posix::inventory_generation_store mismatch_inventory(
        mismatch_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              mismatch_inventory);
        },
        publication_error_code::rejected_object_side_mismatch);
    TEST_CHECK(mismatch_inventory.read().size() == 0U);
  });

  runner.run("invalid batch publishes nothing", [&] {
    test_support::temp_directory batch_inventory_root;
    const auto projection = project_incoming_records(
        context,
        {{"tool", incoming.record},
         {"unpublished",
          app_identity<pkgapply::rejected_object_record_identity>(220)}},
        75);
    pkgreconcile::posix::inventory_generation_store batch_inventory(
        batch_inventory_root.path(), projection.target());
    require_publication_error(
        [&] {
          (void)publish_verified_projection(
              projection, context.rejected_store(), rejected_store,
              batch_inventory);
        },
        publication_error_code::rejected_object_missing);
    TEST_CHECK(batch_inventory.read().size() == 0U);
  });

  runner.run("large verified batch has bounded descriptor footprint", [&] {
    test_support::temp_directory batch_inventory_root;
    constexpr std::size_t batch_size = 48U;
    const auto projection = make_incoming_batch_case(
        context, rejected_store, batch_size, 120);
    pkgreconcile::posix::inventory_generation_store batch_inventory(
        batch_inventory_root.path(), projection.target());

    scoped_nofile_limit descriptor_limit(32);
    const auto receipt = publish_verified_projection(
        projection, context.rejected_store(), rejected_store, batch_inventory);
    TEST_CHECK(receipt.published() == batch_size);
    TEST_CHECK(batch_inventory.read().size() == batch_size);
  });

  runner.run("old rejected evidence publishes as prior installed", [&] {
    test_support::temp_directory old_target_root;
    test_support::temp_directory capture_root;
    test_support::temp_directory old_inventory_root;
    auto old = make_old_case(
        context, rejected_store, old_target_root.path(), capture_root.path(),
        "etc/old.conf", 80);
    pkgreconcile::posix::inventory_generation_store old_inventory(
        old_inventory_root.path(), old.projection.target());
    const auto receipt = publish_verified_projection(
        old.projection, context.rejected_store(), rejected_store, old_inventory);
    TEST_CHECK(receipt.published() == 1U);
    TEST_CHECK(old.projection.pending().front().side() ==
               pkgreconcile::rejected_object_side::prior_installed);
    TEST_CHECK(old_inventory.read().find(old.projection.pending().front()) != nullptr);
  });

  return runner.finish();
}
