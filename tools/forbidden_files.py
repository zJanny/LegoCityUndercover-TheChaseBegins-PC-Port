#!/usr/bin/env python3
"""The one source of the rules for files that never enter the repository: game-derived, build and binary files.

forbidden_reason(path, data_head) names why a path is refused (None: allowed); data_head is the start of the file's content, or
None when it is not known (only the path rule applies then). check_paths(paths, read_head) applies it to many paths.
Used by tools/merge_agent.py (merges and check-pr), tools/claude_hooks.py (commits) and the CTest no-game-files.
"""
from __future__ import annotations

import re
from typing import Callable, Iterable

FORBIDDEN = re.compile(
    r"(^|/)(artifacts|build(-[^/]*)?|graphify-out)/|\.(cci|3ds|cst|cia|bin|bmp|png|jpe?g|wav|bcstm|bcwav|fib|blvl|bfnmdl|dat|mem)$|(^|/)session\.json$",
    re.IGNORECASE,
)
ALLOWED_BINARY = re.compile(r"^third_party/fonts/[^/]+\.ttf$")
BINARY_PROBE = 8000


def is_binary(data_head: bytes) -> bool:
    return b"\0" in data_head[:BINARY_PROBE]


def forbidden_reason(path: str, data_head: bytes | None) -> str | None:
    if FORBIDDEN.search(path):
        return "game-derived or build path"
    if data_head is not None and is_binary(data_head) and not ALLOWED_BINARY.match(path):
        return "binary content"
    return None


def check_paths(paths: Iterable[str], read_head: Callable[[str], bytes | None]) -> list[tuple[str, str]]:
    found = []
    for path in paths:
        reason = forbidden_reason(path, None) or forbidden_reason(path, read_head(path))
        if reason:
            found.append((path, reason))
    return found
