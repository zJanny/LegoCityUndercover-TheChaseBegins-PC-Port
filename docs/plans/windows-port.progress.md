# Progress — Windows port

## Status

- Overall: IN_PROGRESS
- Current Step: S1
- Last Updated: 2026-11-12T20:54:37+01:00
- Plan: docs/plans/windows-port.md

## Decisions

### Decision: Windows toolchain and scope (2026-11-12T20:54:37+01:00)
MSYS2 UCRT64 GCC, MSYS2 packages, Ninja, no packaging, `%APPDATA%` saves, skip Linux-only tests with a reason. Windows CI is an open question. See [windows-toolchain.md](../design/windows-toolchain.md#ts-20261112-205437).

## Blockers

The user is on Linux; implementation starts when the user switches to Windows (S1 is theirs).

## Next Action

S1: the user sets up MSYS2 UCRT64 on the Windows partition. Then the first Windows session runs S2 (configure and build) and fixes the package list.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `windows-port.progress-archive.md`.

- 2026-11-12T20:54:37+01:00 — Planner: opened the windows-port plan after scouting the Linux checkout. See [windows-build](features/windows-build.md).
