# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Kittycat is implemented independently in several languages (Go, Rust,
TypeScript, Python, C, Luau). Unless a change is noted as language-specific,
it applies identically across all of them, since they are required to stay
behaviorally in sync.

## [Unreleased]

### Added

- `namespace.perm` permission model with wildcard (`*`) and negator (`~`)
  support, position-hierarchy resolution (`StaffPermissions.resolve()`), and
  `@clear`/`global.@clear` to reset accumulated permissions for a namespace
  (or everything) mid-resolution.
- `has_perm`/`has_perm_string` for checking a resolved permission set against
  a permission, and `check_patch_changes`/`check_patch_changes_string` for
  checking whether a manager's permissions allow a proposed permission-set
  change.

### Changed

- `resolve()` (Go/Rust/TypeScript) rewritten from O(n^2) to O(n) in the total
  number of permissions across all positions. The previous implementation
  did a full linear scan over every accumulated permission for each `@clear`
  and each wildcard-clears-negators operation; an auxiliary namespace index
  now answers "which applied permissions are in this namespace" in O(1)
  instead. The sequence of underlying map inserts/removals, and therefore
  the resolved output and its order, is unchanged — verified against each
  language's existing test suite, which asserts exact positional order.
