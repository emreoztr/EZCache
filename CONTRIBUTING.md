# Contributing to EZCache

Thank you for your interest in contributing to **ezcache** — a header-only, C++20 LRU + TTL memoization/cache library.

## Quick Start

```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Tests (mandatory)
ctest --test-dir build --output-on-failure
```

> **Mandatory:** All unit tests and CI checks must pass before a PR can be merged.

## Contribution Scope

- **Bugs:** Fix directly and open a PR (issues will not be used at this stage).
- **Features:** Small, focused features that do not break benchmarks are welcome.
- **Documentation:** Improvements to README and examples.
- **Performance:** Changes must be backed by benchmarks.

## Style and Design Guidelines

- **Language Standard:** C++20.
- **Thread-Safety:** Uses `std::shared_mutex`:
  - `get` → `std::shared_lock`
  - `put/clear` → `std::unique_lock`
  
  Do not break this model. Keep lock scopes minimal.

- **Naming:** Use snake_case for variables (e.g., `lru_index`, `expire_time`). Follow existing conventions.
- **Dependencies:** No additional runtime dependencies. Must remain header-only.
- **API Stability:** Avoid breaking public API. Add overloads/traits instead.
- **Performance:** Hot paths must be allocation-free. Avoid unnecessary copies (`std::move`/`std::forward` appropriately). LRU/expire clean-up complexity should not grow; preserve current batch-cleaning behavior.
- **Type Safety:** Preserve the `std::any` + `std::type_index` collision check. Increment collision counter correctly on mismatches.

## Tests and Benchmarking

- **Unit Tests (mandatory):** If you add or change functionality, you must add or update unit tests.
- **Benchmarks (optional but encouraged):** For scenarios like “hot/hit”, “cold/miss”, “expire”, “type-mismatch”, “collision”. Include results in PR description.

## Pull Request Rules

- **Single commit** per PR with a **clear and descriptive commit message**.
- Signed commits (GPG/SSH) are preferred but not required.
- PR description should include:
  - Summary of changes
  - Tests added/modified
  - (Optional) Benchmark results

> **Review:** All reviews are currently handled by the repository owner. Rebase-merging will be used.

## Code Practices

- Include order: STL → project internal (`"internal.hpp"`) → external (none currently).
- Use `constexpr`, `noexcept`, and `[[nodiscard]]` where appropriate.
- Preserve `LRU_CLEAR_RATE` logic and `MAX_BATCH` limit in LRU cleanup.

## License and Code of Conduct

- All contributions are accepted under the **MIT** license.
- Community interactions are governed by **Contributor Covenant v2.1** (`CODE_OF_CONDUCT.md`).
  - Violations can be reported to: `78965275+emreoztr@users.noreply.github.com`

## Checklist Before Submitting

- [ ] Builds with C++20
- [ ] All unit tests pass (`ctest`)
- [ ] CI is green
- [ ] Single commit with a clear message
- [ ] No performance regressions / lock scope changes
- [ ] Public API unchanged
- [ ] Code style and naming match existing conventions

Thank you! Small, focused, and non-breaking contributions will be merged quickly.
