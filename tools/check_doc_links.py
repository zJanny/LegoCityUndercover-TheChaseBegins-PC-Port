#!/usr/bin/env python3
"""Check (and with --fix repair) relative links and anchors in the repository's Markdown files.

Every dated entry in docs/re/*.md (a "## YYYY-MM-DDTHH:MM:SS..." heading) gets an explicit,
stable anchor line "<a id="ts-YYYYMMDD-HHMMSS"></a>" in front of it. Every link whose text
contains "§YYYY-MM-DDTHH:MM" must point at "file.md#ts-..." of the entry with that timestamp.

Size and shape rules (CLAUDE.md "Documentation rules"): a docs/re file is at most 40 KB, a feature file at most
16 KB, CLAUDE.md at most 20 KiB, the session log of a progress file at most
20 entry lines. With --fix, older log lines move to the progress archive (a line the archive already holds, as
after a merge of two branches that both archived it, is dropped instead of copied again) and closed items ("- [x] ID") of a feature
file move to <feature>.closed.md. An item or stand-in ID defined twice in a feature and its closed file is an
error. Every feature file has exactly one row (at most 300 characters, table preceded by a blank line) in the feature index, which groups its tables under "## <Area>" headings; closed-items files have none. Item lines keep the field order gap text, needs:, blocks:, uses:, evidence:. A timestamp later than the current time is an error; a ":00" second in a line new compared to HEAD is a warning. --next PREFIX [COUNT] prints free item IDs, --reserve PREFIX COUNT WHO reserves a range in docs/plans/features/id-reservations.md, --release WHO drops it (an error when WHO holds none); an unknown PREFIX needs --new-prefix. An entry with a "Superseded by" line under its heading moves with --fix to docs/re/<topic>.history.md and links follow. A link whose text starts with "Corrects" needs a "Corrected by" line under the corrected entry's heading
that links back, and every "Corrected by" line needs the matching "Corrects" link.
"""
import argparse
import re
import subprocess
import sys
import unicodedata
from datetime import datetime, timezone
from pathlib import Path

EXCLUDED_DIRS = {"artifacts", "build", ".claude", "graphify-out", ".git", "node_modules"}
ENTRY_HEADING = re.compile(r"^## (\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d)(?::(\d\d))?")
ANCHOR_LINE = re.compile(r'^<a id="(ts-\d{8}-\d{4,6})"></a>\s*$')
LINK = re.compile(r"(!?\[(?:[^\]\\]|\\.)*\]\()([^)\s]*)((?:\s+\"[^\"]*\")?\))")
SECTION_STAMP = re.compile(r"§\s*(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d)(?::(\d\d))?")
INLINE_CODE = re.compile(r"`[^`\n]*`")
EXPLICIT_ID = re.compile(r'<a\s+[^>]*?\b(?:id|name)="([^"]+)"')
FENCE = re.compile(r"^\s*(```|~~~)")
ITEM_ID = re.compile(r"^- (?:\[[ x]\] )?([A-Z][A-Z0-9]*-S?\d+)\b")
CLOSED_ITEM_LINE = re.compile(r"^- \[x\] [A-Z][A-Z0-9]*-\d+\b")
SOURCE_LINE_TARGET = re.compile(r"^(.*)\?display=source#L(\d+)$")
INDEX_ROW = re.compile(r"^\|\s*\[[^\]]*\]\(([^)#\s]+\.md)\)")
LOG_HEADING = "## Session Log"
CLOSED_SUFFIX = ".closed.md"
ARCHIVE_SUFFIX = "-archive.md"
MAX_RE_BYTES = 40960
MAX_FEATURE_BYTES = 16384
MAX_INDEX_ROW = 300
RESERVATIONS = "id-reservations.md"
HISTORY_SUFFIX = ".history.md"
FIELD_ORDER = {"needs": 0, "blocks": 1, "uses": 2, "evidence": 3}
FIELD = re.compile(r"(?<![\w-])(needs|blocks|uses|evidence):")
TIMESTAMP = re.compile(r"\b(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)([+-]\d\d:\d\d|Z)")
IDENTIFIER = re.compile(r"^([A-Z][A-Z0-9]*)-(S?)(\d+)")
PREFIX = re.compile(r"^[A-Z][A-Z0-9]*(-S)?$")
CURRENT_STATE = "## Current state"
SUBTOPICS = "Subtopics:"
SUPERSEDED = "Superseded by"
MAX_LOG_LINES = 20
MAX_CLAUDE_BYTES = 20480


def markdown_files(root):
    if (root / ".git").exists():
        out = subprocess.run(
            ["git", "-C", str(root), "ls-files", "--cached", "--others", "--exclude-standard", "*.md"], capture_output=True, text=True, check=True
        ).stdout.splitlines()
        return sorted(root / p for p in set(out) if not set(Path(p).parts) & EXCLUDED_DIRS and (root / p).is_file())
    return sorted(
        p for p in root.rglob("*.md") if not set(p.relative_to(root).parts) & EXCLUDED_DIRS and p.is_file()
    )


def code_line_flags(lines):
    flags = []
    inside = False
    for line in lines:
        if FENCE.match(line):
            inside = not inside
            flags.append(True)
        else:
            flags.append(inside)
    return flags


def github_slug(text):
    text = re.sub(r"<[^>]+>", "", text)
    text = re.sub(r"!?\[([^\]]*)\]\([^)]*\)", r"\1", text)
    text = text.replace("`", "").replace("*", "").strip().lower()
    out = []
    for ch in text:
        if ch in " ":
            out.append("-")
        elif ch == "-" or ch == "_" or unicodedata.category(ch)[0] in "LN" or unicodedata.category(ch) == "Mn":
            out.append(ch)
    return "".join(out)


def entry_anchor(match):
    year, month, day, hour, minute, second = match.groups()
    return f"ts-{year}{month}{day}-{hour}{minute}{second or ''}"


class Doc:
    def __init__(self, path):
        self.path = path
        self.lines = path.read_text(encoding="utf-8").split("\n")
        self.in_code = code_line_flags(self.lines)

    def entries(self):
        for number, line in enumerate(self.lines):
            if not self.in_code[number]:
                match = ENTRY_HEADING.match(line)
                if match:
                    yield number, entry_anchor(match)

    def anchors(self):
        found = set()
        counts = {}
        for number, line in enumerate(self.lines):
            if self.in_code[number]:
                continue
            for match in EXPLICIT_ID.finditer(line):
                found.add(match.group(1))
            heading = re.match(r"^#{1,6}\s+(.*?)\s*#*\s*$", line)
            if heading:
                slug = github_slug(heading.group(1))
                count = counts.get(slug, 0)
                counts[slug] = count + 1
                found.add(slug if count == 0 else f"{slug}-{count}")
        return found

    def entry_anchors(self):
        return [anchor for _, anchor in self.entries()]

    def add_missing_anchors(self):
        added = 0
        out = []
        for number, line in enumerate(self.lines):
            if not self.in_code[number]:
                if ANCHOR_LINE.match(line) and out and out[-1].strip() == line.strip():
                    added += 1
                    continue
                match = ENTRY_HEADING.match(line)
                if match:
                    anchor = entry_anchor(match)
                    if out and out[-1].strip() == f'<a id="{anchor}"></a>':
                        out.append("")
                        added += 1
                    elif len(out) < 2 or out[-2].strip() != f'<a id="{anchor}"></a>':
                        out.append(f'<a id="{anchor}"></a>')
                        out.append("")
                        added += 1
            out.append(line)
        if added:
            self.lines = out
            self.in_code = code_line_flags(self.lines)
        return added

    def save(self):
        self.path.write_text("\n".join(self.lines), encoding="utf-8")


def stamp_prefix(match):
    year, month, day, hour, minute, second = match.groups()
    return f"ts-{year}{month}{day}-{hour}{minute}{second or ''}"


def resolve_stamp(target_doc, match):
    prefix = stamp_prefix(match)
    hits = [a for a in target_doc.entry_anchors() if a.startswith(prefix)]
    return hits


def external(target):
    return re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target) or target.startswith("//")


def entry_anchor_at(doc, number):
    current = None
    for entry_number, anchor in doc.entries():
        if entry_number > number:
            break
        current = anchor
    return current


def source_line_link(path, text, file_part, line_number):
    """(error or None, corrected line number) of a link to one line of the source view of a Markdown file."""
    lines = path.read_text(encoding="utf-8").split("\n")
    if not IDENTIFIER.match(text):
        if 1 <= line_number <= len(lines):
            return None, line_number
        return f"line {line_number} is outside {file_part}", line_number
    for number, line in enumerate(lines, 1):
        match = ITEM_ID.match(line)
        if match and match.group(1) == text:
            return None, number
    return f"no item {text} in {file_part}", line_number


def link_targets(path, line, masked):
    for match in LINK.finditer(masked):
        head = line[match.start(1):match.end(1)]
        target = line[match.start(2):match.end(2)]
        if not target or external(target) or "#" not in target:
            continue
        file_part, _, fragment = target.partition("#")
        dest = (path.parent / file_part).resolve() if file_part else path.resolve()
        yield head[1:-2], dest, fragment


def correction_errors(root, files, load):
    errors = []
    corrects = {}
    corrected_by = []
    for path in files:
        doc = load(path)
        rel = path.relative_to(root)
        in_re = path.is_relative_to(root / "docs" / "re")
        for number, line in enumerate(doc.lines):
            if doc.in_code[number]:
                continue
            masked = INLINE_CODE.sub(lambda m: "\0" * len(m.group(0)), line)
            is_corrected_by = line.startswith("Corrected by")
            source = entry_anchor_at(doc, number) if in_re else None
            for text, dest, fragment in link_targets(path, line, masked):
                if dest.suffix != ".md" or not dest.is_file() or fragment not in load(dest).entry_anchors():
                    continue
                if text.startswith("Corrects"):
                    corrects[(path.resolve(), source, dest, fragment)] = f"{rel}:{number + 1}"
                if is_corrected_by:
                    corrected_by.append((path, number, dest, fragment))
    for path in files:
        if not path.is_relative_to(root / "docs" / "re"):
            continue
        doc = load(path)
        entries = list(doc.entries())
        for index, (start, _) in enumerate(entries):
            end = entries[index + 1][0] if index + 1 < len(entries) else len(doc.lines)
            leading = True
            for number in range(start + 1, end):
                line = doc.lines[number]
                if not line.strip():
                    continue
                if line.startswith("Corrected by") or line.startswith(SUPERSEDED):
                    if not leading:
                        errors.append(f"{path.relative_to(root)}:{number + 1}: correction-pairing: '{line.split(' [')[0]}' must stand directly under the heading of its entry")
                    elif line.startswith(SUPERSEDED) and not LINK.search(line):
                        errors.append(f"{path.relative_to(root)}:{number + 1}: superseded-link: 'Superseded by' needs a link to the entry that replaces it")
                else:
                    leading = False
    for (source_path, source_anchor, dest, fragment), where in corrects.items():
        found = any(
            path.resolve() == dest
            and back_dest == source_path
            and entry_anchor_at(load(path), number) == fragment
            and (source_anchor is None or back_fragment == source_anchor)
            for path, number, back_dest, back_fragment in corrected_by
        )
        if not found:
            errors.append(f"{where}: correction-pairing: no 'Corrected by' line linking back under {dest.relative_to(root)}#{fragment}")
    for path, number, dest, fragment in corrected_by:
        entry = entry_anchor_at(load(path), number)
        matched = any(
            key[2] == path.resolve() and key[3] == entry and key[0] == dest and (key[1] is None or key[1] == fragment)
            for key in corrects
        )
        if not matched:
            errors.append(f"{path.relative_to(root)}:{number + 1}: correction-pairing: 'Corrected by' line has no matching 'Corrects' link in {dest.relative_to(root)}#{fragment}")
    return errors


def feature_dir(root):
    return root / "docs" / "plans" / "features"


def feature_files(root):
    directory = feature_dir(root)
    if not directory.is_dir():
        return []
    return [p for p in sorted(directory.glob("*.md")) if p.name not in ("README.md", RESERVATIONS) and not p.name.endswith(CLOSED_SUFFIX)]


def closed_file_of(path):
    return path.with_name(path.name[: -len(".md")] + CLOSED_SUFFIX)


def feature_base(stem, stems):
    best = stem
    for other in stems:
        if stem.startswith(other + "-") and (best == stem or len(other) < len(best)):
            best = other
    return best


def duplicate_id_errors(root):
    errors = []
    paths = feature_files(root)
    stems = [p.stem for p in paths]
    seen = {}
    for path in paths:
        base = feature_base(path.stem, stems)
        for member in (path, closed_file_of(path)):
            if not member.is_file():
                continue
            for number, line in enumerate(member.read_text(encoding="utf-8").split("\n")):
                match = ITEM_ID.match(line)
                if not match:
                    continue
                key = (base, match.group(1))
                where = f"{member.relative_to(root)}:{number + 1}"
                if key in seen:
                    errors.append(f"{where}: duplicate-id: {match.group(1)} is already defined at {seen[key]}; IDs are never reused")
                else:
                    seen[key] = where
    return errors


def index_errors(root):
    index = feature_dir(root) / "README.md"
    if not index.is_file():
        return []
    errors = []
    rows = {}
    rel = index.relative_to(root)
    index_lines = index.read_text(encoding="utf-8").split("\n")
    for number, line in enumerate(index_lines):
        match = INDEX_ROW.match(line)
        if match:
            if match.group(1) in rows:
                errors.append(f"{rel}:{number + 1}: index-duplicate-row: {match.group(1)} already has a row at line {rows[match.group(1)]}")
            rows[match.group(1)] = number + 1
            if len(line) > MAX_INDEX_ROW:
                errors.append(f"{rel}:{number + 1}: index-row-length: {len(line)} characters, limit {MAX_INDEX_ROW}; keep the top open items short")
        if line.startswith("|") and (number == 0 or not index_lines[number - 1].startswith("|")) and number > 0 and index_lines[number - 1].strip():
            errors.append(f"{rel}:{number + 1}: index-table-spacing: a table needs a blank line before it")
    for name, number in rows.items():
        if name.endswith(CLOSED_SUFFIX):
            errors.append(f"{rel}:{number}: index-closed-file: {name} is a closed-items file and has no index row")
    for path in feature_files(root):
        if path.name not in rows:
            errors.append(f"{path.relative_to(root)}:1: index-missing-row: {path.name} has no row in {rel}; add one")
    return errors


def item_format_errors(root):
    errors = []
    for path in feature_files(root):
        for member in (path, closed_file_of(path)):
            if not member.is_file():
                continue
            for number, line in enumerate(member.read_text(encoding="utf-8").split("\n")):
                if not ITEM_ID.match(line):
                    continue
                ranks = [FIELD_ORDER[m.group(1)] for m in FIELD.finditer(line)]
                if ranks != sorted(set(ranks)):
                    errors.append(f"{member.relative_to(root)}:{number + 1}: item-field-order: fields must follow gap text, needs:, blocks:, uses:, evidence: (each at most once, evidence last)")
    return errors


def current_time():
    return datetime.now(timezone.utc)


def parse_timestamp(match):
    year, month, day, hour, minute, second, zone = match.groups()
    zone = "+00:00" if zone == "Z" else zone
    try:
        return datetime.fromisoformat(f"{year}-{month}-{day}T{hour}:{minute}:{second}{zone}")
    except ValueError:
        return None


def line_selector(root, path):
    if path.is_relative_to(root / "docs" / "re") and not path.name.endswith(HISTORY_SUFFIX):
        return lambda line: ENTRY_HEADING.match(line) is not None
    if path.is_relative_to(feature_dir(root)):
        return lambda line: True
    if path in progress_files(root):
        return lambda line: line.startswith("- ")
    return None


def dated_paths(root):
    re_dir = root / "docs" / "re"
    paths = list(re_dir.glob("*.md")) if re_dir.is_dir() else []
    features = feature_dir(root)
    if features.is_dir():
        paths += [p for p in features.glob("*.md") if p.name not in ("README.md", RESERVATIONS)]
    return paths + progress_files(root)


def future_selector(root, path):
    if path.is_relative_to(root / "docs" / "re") and not path.name.endswith(HISTORY_SUFFIX):
        return lambda line: True
    return line_selector(root, path)


def timestamp_errors(root):
    errors = []
    now = current_time()
    for path in dated_paths(root):
        selects = future_selector(root, path)
        if selects is None:
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").split("\n")):
            if not selects(line):
                continue
            for match in TIMESTAMP.finditer(line):
                moment = parse_timestamp(match)
                if moment is not None and moment > now:
                    errors.append(f"{path.relative_to(root)}:{number + 1}: future-timestamp: {match.group(0)} is later than the current time; take times from date -Iseconds")
    return errors


def current_state_errors(root):
    errors = []
    re_dir = root / "docs" / "re"
    if not re_dir.is_dir():
        return errors
    for path in sorted(re_dir.glob("*.md")):
        if path.name.endswith(HISTORY_SUFFIX) or path.name == "README.md":
            continue
        lines = path.read_text(encoding="utf-8").split("\n")
        state = next((i for i, line in enumerate(lines) if line.strip() == CURRENT_STATE), None)
        first = next((i for i, line in enumerate(lines) if ENTRY_HEADING.match(line)), None)
        if state is None and first is not None:
            errors.append(f"{path.relative_to(root)}:1: current-state-missing: every topic file needs '{CURRENT_STATE}' before its first dated entry")
        if state is not None and first is not None and state > first:
            errors.append(f"{path.relative_to(root)}:{state + 1}: current-state-position: '{CURRENT_STATE}' must come before the first dated entry")
    return errors


def split_leftover_warnings(root):
    warnings = []
    re_dir = root / "docs" / "re"
    if not re_dir.is_dir():
        return warnings
    changed = {root / name for name in added_lines(root)}
    for path in sorted(re_dir.glob("*.md")):
        if path.name.endswith(HISTORY_SUFFIX) or path.name == "README.md" or path not in changed:
            continue
        lines = path.read_text(encoding="utf-8").split("\n")
        state = next((i for i, line in enumerate(lines) if line.strip() == CURRENT_STATE), None)
        if state is None:
            continue
        end = next((i for i in range(state + 1, len(lines)) if lines[i].startswith("<a id=") or lines[i].startswith("## ")), len(lines))
        subtopics = {name for line in lines if line.startswith(SUBTOPICS) for name in re.findall(r"\]\(([\w.-]+\.md)\)", line)}
        for number in range(state + 1, end):
            for name in re.findall(r"\]\(([\w.-]+\.md)#ts-", lines[number]):
                if name in subtopics:
                    warnings.append(f"{path.relative_to(root)}:{number + 1}: warning: current-state-split: links an entry of the split subtopic {name}; after a split that bullet belongs in {name}'s '{CURRENT_STATE}'")
    return warnings


def added_lines(root):
    if not (root / ".git").exists():
        return {}
    try:
        diff = subprocess.run(["git", "-C", str(root), "diff", "HEAD", "-U0", "--no-color", "--", "docs"], capture_output=True, text=True, check=True).stdout
        untracked = subprocess.run(["git", "-C", str(root), "ls-files", "--others", "--exclude-standard", "docs"], capture_output=True, text=True, check=True).stdout.split()
    except (OSError, subprocess.CalledProcessError):
        return {}
    added = {}
    current = None
    number = 0
    previous = ""
    for line in diff.split("\n"):
        header = line.startswith("+++ ") and previous.startswith("--- ")
        previous = line
        if header:
            current = line[6:] if line.startswith("+++ b/") else None
        elif line.startswith("@@"):
            number = int(re.match(r"@@ -\S+ \+(\d+)", line).group(1))
        elif line.startswith("+") and current:
            added.setdefault(current, []).append((number, line[1:]))
            number += 1
    for name in untracked:
        path = root / name
        if path.suffix == ".md" and path.is_file():
            added[name] = [(i + 1, text) for i, text in enumerate(path.read_text(encoding="utf-8").split("\n"))]
    return added


def stale_current_state(path, lines):
    text_lines = path.read_text(encoding="utf-8").split("\n")
    state = next((i for i, line in enumerate(text_lines) if line.strip() == CURRENT_STATE), None)
    if state is None:
        return None
    end = next((i for i in range(state + 1, len(text_lines)) if text_lines[i].startswith("<a id=") or ENTRY_HEADING.match(text_lines[i])), len(text_lines))
    new_entry = any(ENTRY_HEADING.match(text) for _, text in lines)
    touched = any(state < number <= end for number, _ in lines)
    return state + 1 if new_entry and not touched else None


def timestamp_warnings(root):
    warnings = []
    for name, lines in added_lines(root).items():
        path = root / name
        selects = line_selector(root, path)
        if selects is None:
            continue
        for number, text in lines:
            if selects(text):
                for match in TIMESTAMP.finditer(text):
                    if match.group(6) == "00":
                        warnings.append(f"{name}:{number}: warning: round-seconds-timestamp: {match.group(0)} ends in :00; take times from date -Iseconds")
        if path.is_relative_to(root / "docs" / "re") and not name.endswith(HISTORY_SUFFIX):
            stale = stale_current_state(path, lines)
            if stale:
                warnings.append(f"{name}:{stale}: warning: current-state-stale: new dated entries but '{CURRENT_STATE}' unchanged")
    return warnings


def defined_numbers(root):
    numbers = {}
    directory = feature_dir(root)
    if not directory.is_dir():
        return numbers
    for path in sorted(directory.glob("*.md")):
        if path.name in ("README.md", RESERVATIONS):
            continue
        for line in path.read_text(encoding="utf-8").split("\n"):
            match = ITEM_ID.match(line)
            if match:
                prefix, stand_in, number = IDENTIFIER.match(match.group(1)).groups()
                numbers.setdefault(prefix + ("-S" if stand_in else ""), set()).add(int(number))
    return numbers


def reservation_cells(line):
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    return cells if len(cells) == 5 and cells[1].isdigit() and cells[2].isdigit() else None


def reservation_rows(root):
    path = feature_dir(root) / RESERVATIONS
    rows = []
    if path.is_file():
        for line in path.read_text(encoding="utf-8").split("\n"):
            cells = reservation_cells(line)
            if cells:
                rows.append((cells[0], int(cells[1]), int(cells[2]), cells[3], cells[4]))
    return rows


def free_numbers(root, key, count):
    taken = defined_numbers(root).get(key, set())
    reserved = [(first, last) for prefix, first, last, _, _ in reservation_rows(root) if prefix == key]
    result = []
    candidate = max(taken, default=0) + 1
    while len(result) < count:
        hit = next((last for first, last in reserved if first <= candidate <= last), None)
        if hit is not None:
            candidate = hit + 1
        else:
            result.append(candidate)
            candidate += 1
    return result


def known_prefix(root, key):
    base = key[:-2] if key.endswith("-S") else key
    known = set(defined_numbers(root)) | {prefix for prefix, _, _, _, _ in reservation_rows(root)}
    return base in known or base + "-S" in known or base in {k[:-2] for k in known if k.endswith("-S")}


def known_prefixes(root):
    return sorted({k[:-2] if k.endswith("-S") else k for k in defined_numbers(root)})


def format_id(key, number):
    return f"{key}{number}" if key.endswith("-S") else f"{key}-{number}"


def next_ids(root, key, count):
    return [format_id(key, n) for n in free_numbers(root, key, count)]


RESERVATION_HEADER = ("# ID reservations\n\nRanges of item IDs held by parallel agents; the lead reserves them with "
                      "`tools/check_doc_links.py --reserve PREFIX COUNT WHO` and drops them with `--release WHO`. "
                      "`--next` skips reserved ranges.\n\n| Prefix | First | Last | Holder | Date |\n|---|---|---|---|---|\n")


def reserve_ids(root, key, count, holder):
    first = free_numbers(root, key, 1)[0]
    first = max(first, max((last for prefix, _, last, _, _ in reservation_rows(root) if prefix == key), default=0) + 1)
    last = first + count - 1
    when = current_time().astimezone().isoformat(timespec="seconds")
    appended(feature_dir(root) / RESERVATIONS, RESERVATION_HEADER, [f"| {key} | {first} | {last} | {holder} | {when} |"])
    return f"{key} {first}..{last} for {holder} ({when})"


def release_ids(root, holder):
    path = feature_dir(root) / RESERVATIONS
    if not path.is_file():
        return 0
    kept = []
    removed = 0
    for line in path.read_text(encoding="utf-8").split("\n"):
        cells = reservation_cells(line)
        if cells and cells[3] == holder:
            removed += 1
        else:
            kept.append(line)
    path.write_text("\n".join(kept), encoding="utf-8")
    return removed


def entry_start(doc, heading, anchor):
    if heading >= 2 and doc.lines[heading - 2].strip() == f'<a id="{anchor}"></a>':
        return heading - 2
    return heading


def superseded_entries(doc):
    entries = list(doc.entries())
    spans = []
    for index, (heading, anchor) in enumerate(entries):
        start = entry_start(doc, heading, anchor)
        end = entry_start(doc, *entries[index + 1]) if index + 1 < len(entries) else len(doc.lines)
        body = next((doc.lines[n] for n in range(heading + 1, end) if doc.lines[n].strip()), "")
        if body.startswith(SUPERSEDED):
            spans.append((start, end, anchor))
    return spans


def retarget_links(line, path, topic, history, moved, leaving):
    masked = INLINE_CODE.sub(lambda m: "\0" * len(m.group(0)), line)
    pieces = []
    last = 0
    for match in LINK.finditer(masked):
        target = line[match.start(2):match.end(2)]
        if not target or external(target) or "#" not in target:
            continue
        file_part, _, fragment = target.partition("#")
        dest = (path.parent / file_part).resolve() if file_part else path.resolve()
        new_target = target
        if leaving:
            if not file_part and fragment not in moved:
                new_target = f"{topic.name}#{fragment}"
        elif dest == topic.resolve() and fragment in moved:
            new_target = file_part[: len(file_part) - len(topic.name)] + f"{history.name}#{fragment}"
        if new_target != target:
            pieces.append(line[last:match.start(2)])
            pieces.append(new_target)
            last = match.end(2)
    pieces.append(line[last:])
    return "".join(pieces)


def move_superseded(root):
    re_dir = root / "docs" / "re"
    if not re_dir.is_dir():
        return 0
    total = 0
    for topic in sorted(re_dir.glob("*.md")):
        if topic.name.endswith(HISTORY_SUFFIX) or topic.name == "README.md":
            continue
        doc = Doc(topic)
        spans = superseded_entries(doc)
        if not spans:
            continue
        history = topic.with_name(topic.name[: -len(".md")] + HISTORY_SUFFIX)
        moved = {anchor for _, _, anchor in spans}
        block = []
        for start, end, _ in spans:
            chunk = [line if doc.in_code[start + i] else retarget_links(line, topic, topic, history, moved, True)
                     for i, line in enumerate(doc.lines[start:end])]
            while chunk and not chunk[-1].strip():
                chunk.pop()
            block.extend(chunk + [""])
        header = f"# History of {topic.stem}\n\nSuperseded entries moved from [{topic.name}]({topic.name}) by tools/check_doc_links.py --fix, unchanged.\n\n"
        appended(history, header, block[:-1])
        drop = {n for start, end, _ in spans for n in range(start, end)}
        topic.write_text("\n".join(line for n, line in enumerate(doc.lines) if n not in drop), encoding="utf-8")
        for path in markdown_files(root):
            current = Doc(path)
            lines = [line if current.in_code[n] else retarget_links(line, path, topic, history, moved, False) for n, line in enumerate(current.lines)]
            if lines != current.lines:
                path.write_text("\n".join(lines), encoding="utf-8")
        total += len(spans)
    return total


def progress_files(root):
    plans = root / "docs" / "plans"
    if not plans.is_dir():
        return []
    return sorted(plans.glob("*.progress.md"))


def log_span(lines):
    heading = next((i for i, line in enumerate(lines) if line.strip() == LOG_HEADING), None)
    if heading is None:
        return None
    entries = [i for i in range(heading + 1, len(lines)) if lines[i].startswith("- ")]
    return heading, entries


def session_log_errors(root):
    errors = []
    for path in progress_files(root):
        span = log_span(path.read_text(encoding="utf-8").split("\n"))
        if span and len(span[1]) > MAX_LOG_LINES:
            errors.append(f"{path.relative_to(root)}:{span[1][0] + 1}: session-log-length: {len(span[1])} entry lines, limit {MAX_LOG_LINES}; run tools/check_doc_links.py --fix to archive the older ones")
    return errors


def archive_path(path):
    return path.with_name(path.name[: -len(".md")] + ARCHIVE_SUFFIX)


def appended(path, header, new_lines):
    if path.is_file():
        text = path.read_text(encoding="utf-8")
        text = text if text.endswith("\n") else text + "\n"
    else:
        text = header
    path.write_text(text + "\n".join(new_lines) + "\n", encoding="utf-8")


def archived_log_lines(path):
    if not path.is_file():
        return set()
    return {line for line in path.read_text(encoding="utf-8").split("\n") if line.startswith("- ")}


def rotate_session_logs(root):
    moved = 0
    for path in progress_files(root):
        lines = path.read_text(encoding="utf-8").split("\n")
        span = log_span(lines)
        if not span:
            continue
        archived = archived_log_lines(archive_path(path))
        drop = {i for i in span[1] if lines[i] in archived}
        live = [i for i in span[1] if i not in drop]
        old = live[: max(0, len(live) - MAX_LOG_LINES)]
        if old:
            header = (f"# Session log archive\n\nSession log lines that left [{path.name}]({path.name}), oldest first, unchanged. "
                      "tools/check_doc_links.py --fix appends new lines at the end.\n\n")
            appended(archive_path(path), header, [lines[i] for i in old])
        drop |= set(old)
        if drop:
            path.write_text("\n".join(line for i, line in enumerate(lines) if i not in drop), encoding="utf-8")
        moved += len(old)
    return moved


def remove_duplicate_archive_lines(root):
    removed = 0
    for path in progress_files(root):
        archive = archive_path(path)
        if not archive.is_file():
            continue
        seen = set()
        kept = []
        lines = archive.read_text(encoding="utf-8").split("\n")
        for line in lines:
            if line.startswith("- ") and line in seen:
                continue
            seen.add(line)
            kept.append(line)
        if len(kept) < len(lines):
            archive.write_text("\n".join(kept), encoding="utf-8")
            removed += len(lines) - len(kept)
    return removed


def move_closed_items(root):
    moved = 0
    for path in feature_files(root):
        lines = path.read_text(encoding="utf-8").split("\n")
        closed = [line for line in lines if CLOSED_ITEM_LINE.match(line)]
        if not closed:
            continue
        header = (f"# Closed items of {path.stem}\n\nDone items of [{path.name}]({path.name}), moved here by "
                  "tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.\n\n")
        appended(closed_file_of(path), header, closed)
        path.write_text("\n".join(line for line in lines if not CLOSED_ITEM_LINE.match(line)), encoding="utf-8")
        moved += len(closed)
    return moved


def size_errors(root):
    errors = []
    re_dir = root / "docs" / "re"
    if re_dir.is_dir():
        for path in sorted(re_dir.glob("*.md")):
            if path.name.endswith(HISTORY_SUFFIX):
                continue
            size = path.stat().st_size
            if size > MAX_RE_BYTES:
                errors.append(f"{path.relative_to(root)}:1: re-file-size: {size} bytes, limit {MAX_RE_BYTES}; split it by subtopic")
    features = feature_dir(root)
    if features.is_dir():
        for path in feature_files(root):
            size = path.stat().st_size
            if size > MAX_FEATURE_BYTES:
                errors.append(f"{path.relative_to(root)}:1: feature-file-size: {size} bytes, limit {MAX_FEATURE_BYTES}; split it by subtopic as <feature>-<subtopic>.md")
    errors.extend(duplicate_id_errors(root))
    errors.extend(index_errors(root))
    errors.extend(item_format_errors(root))
    errors.extend(timestamp_errors(root))
    errors.extend(current_state_errors(root))
    errors.extend(session_log_errors(root))
    claude = root / "CLAUDE.md"
    if claude.is_file() and claude.stat().st_size > MAX_CLAUDE_BYTES:
        errors.append(f"CLAUDE.md:1: claude-md-size: {claude.stat().st_size} bytes, limit {MAX_CLAUDE_BYTES}")
    return errors


def process(root, fix, messages=None):
    docs = {}
    if fix:
        for what, count in (("duplicate archive log lines removed", remove_duplicate_archive_lines(root)),
                            ("log lines archived", rotate_session_logs(root)), ("superseded entries moved", move_superseded(root)), ("closed items moved", move_closed_items(root))):
            if count and messages is not None:
                messages.append(f"{count} {what}")

    def load(path):
        key = path.resolve()
        if key not in docs:
            docs[key] = Doc(path)
        return docs[key]

    files = markdown_files(root)
    anchors_added = 0
    if fix:
        for path in files:
            if path.is_relative_to(root / "docs" / "re"):
                doc = load(path)
                n = doc.add_missing_anchors()
                if n:
                    doc.save()
                    anchors_added += n
    errors = []
    links_rewritten = 0
    for path in files:
        if path.is_relative_to(root / "docs" / "re"):
            doc = load(path)
            for number, anchor in doc.entries():
                if number == 0 or doc.lines[number - 2].strip() != f'<a id="{anchor}"></a>':
                    errors.append(f"{path.relative_to(root)}:{number + 1}: entry has no anchor {anchor}")
            seen = set()
            for number, line in enumerate(doc.lines):
                match = ANCHOR_LINE.match(line)
                if match and not doc.in_code[number]:
                    if match.group(1) in seen:
                        errors.append(f"{path.relative_to(root)}:{number + 1}: duplicate anchor {match.group(1)}")
                    seen.add(match.group(1))
    for path in files:
        doc = load(path)
        rel = path.relative_to(root)
        changed = False
        for number, line in enumerate(doc.lines):
            if doc.in_code[number]:
                continue
            masked = INLINE_CODE.sub(lambda m: "\0" * len(m.group(0)), line)
            pieces = []
            last = 0
            for match in LINK.finditer(masked):
                head, target, tail = line[match.start(1):match.end(1)], line[match.start(2):match.end(2)], line[match.start(3):match.end(3)]
                new_target = target
                where = f"{rel}:{number + 1}"
                source_link = SOURCE_LINE_TARGET.match(target)
                if source_link:
                    file_part, wanted = source_link.group(1), int(source_link.group(2))
                    dest = (path.parent / file_part).resolve()
                    if not dest.is_file() or dest.suffix != ".md":
                        errors.append(f"{where}: missing file {target}")
                    else:
                        problem, actual = source_line_link(dest, head[1:-2], file_part, wanted)
                        if problem:
                            errors.append(f"{where}: {problem}")
                        elif actual != wanted:
                            new_target = f"{file_part}?display=source#L{actual}"
                            if fix:
                                links_rewritten += 1
                            else:
                                errors.append(f"{where}: link {target} must be {new_target}")
                elif target and not external(target):
                    file_part, _, fragment = target.partition("#")
                    dest = (path.parent / file_part).resolve() if file_part else path.resolve()
                    stamp = SECTION_STAMP.search(head)
                    if file_part and not dest.exists():
                        errors.append(f"{where}: missing file {target}")
                    elif dest.suffix == ".md" and dest.is_file():
                        target_doc = load(dest)
                        link_file = "" if dest == path.resolve() else file_part
                        if stamp:
                            hits = resolve_stamp(target_doc, stamp)
                            if len(hits) == 1:
                                new_target = f"{link_file}#{hits[0]}"
                                if new_target != target:
                                    if fix:
                                        links_rewritten += 1
                                    else:
                                        errors.append(f"{where}: link {target} must be {new_target}")
                            elif not hits:
                                errors.append(f"{where}: no entry {stamp.group(0)} in {file_part}")
                            else:
                                errors.append(f"{where}: {stamp.group(0)} is ambiguous in {file_part}: {', '.join(hits)}")
                        elif fragment and fragment not in target_doc.anchors():
                            errors.append(f"{where}: missing anchor #{fragment} in {file_part or rel.name}")
                        elif fragment and link_file != file_part:
                            new_target = f"#{fragment}"
                            if fix:
                                links_rewritten += 1
                            else:
                                errors.append(f"{where}: same-file link {target} must be {new_target}")
                pieces.append(line[last:match.start(2)])
                pieces.append(new_target)
                last = match.end(2)
            if pieces:
                pieces.append(line[last:])
                new_line = "".join(pieces)
                if new_line != line:
                    doc.lines[number] = new_line
                    changed = True
        if changed:
            doc.save()
    errors.extend(size_errors(root))
    errors.extend(correction_errors(root, files, load))
    return errors, anchors_added, links_rewritten, len(files)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=str(Path(__file__).resolve().parent.parent))
    parser.add_argument("--fix", action="store_true", help="add missing entry anchors, rewrite section links, move closed items, old log lines and superseded entries")
    parser.add_argument("--next", nargs="+", metavar=("PREFIX", "COUNT"), help="print the next free item IDs of PREFIX (PREFIX-S for stand-ins)")
    parser.add_argument("--reserve", nargs=3, metavar=("PREFIX", "COUNT", "WHO"), help="reserve COUNT item IDs of PREFIX for WHO")
    parser.add_argument("--release", metavar="WHO", help="drop the reservations of WHO")
    parser.add_argument("--new-prefix", action="store_true", help="allow --next / --reserve for a prefix no feature file uses yet")
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    if args.next is not None:
        valid = len(args.next) <= 2 and PREFIX.match(args.next[0]) and (len(args.next) == 1 or (args.next[1].isdigit() and int(args.next[1]) > 0))
        if not valid:
            parser.error("--next: expected PREFIX [COUNT] with PREFIX like VEH or VEH-S and COUNT a positive integer, got " + " ".join(args.next))
        if not args.new_prefix and not known_prefix(root, args.next[0]):
            parser.error(f"--next: unknown prefix {args.next[0]} (known: {', '.join(known_prefixes(root))}); add --new-prefix for a new feature")
        print("\n".join(next_ids(root, args.next[0], int(args.next[1]) if len(args.next) == 2 else 1)))
        return 0
    if args.reserve is not None:
        prefix, count, who = args.reserve
        if not PREFIX.match(prefix) or not count.isdigit() or int(count) <= 0 or not who.strip() or "|" in who:
            parser.error("--reserve: expected PREFIX COUNT WHO with PREFIX like VEH or VEH-S, COUNT a positive integer and WHO without '|', got " + " ".join(args.reserve))
        if not args.new_prefix and not known_prefix(root, prefix):
            parser.error(f"--reserve: unknown prefix {prefix} (known: {', '.join(known_prefixes(root))}); add --new-prefix for a new feature")
        print(reserve_ids(root, prefix, int(count), who.strip()))
        return 0
    if args.release is not None:
        if not args.release.strip():
            parser.error("--release: WHO must not be empty")
        released = release_ids(root, args.release.strip())
        if released == 0:
            parser.error(f"--release: no reservation held by {args.release.strip()!r}")
        print(f"{released} reservations released")
        return 0
    messages = []
    errors, added, rewritten, count = process(root, args.fix, messages)
    for message in messages:
        print(message)
    for warning in timestamp_warnings(root) + split_leftover_warnings(root):
        print(warning)
    for error in errors:
        print(error)
    if args.fix:
        print(f"{count} files checked, {added} anchors fixed, {rewritten} links rewritten")
    elif not errors:
        print(f"{count} files checked, all links and anchors resolve")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
