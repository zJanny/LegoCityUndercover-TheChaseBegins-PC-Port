#!/usr/bin/env python3
"""Check (and with --fix repair) relative links and anchors in the repository's Markdown files.

Every dated entry in docs/re/*.md (a "## YYYY-MM-DDTHH:MM:SS..." heading) gets an explicit,
stable anchor line "<a id="ts-YYYYMMDD-HHMMSS"></a>" in front of it. Every link whose text
contains "§YYYY-MM-DDTHH:MM" must point at "file.md#ts-..." of the entry with that timestamp.

Size and shape rules (CLAUDE.md "Documentation rules"): a docs/re file is at most 40 KB, a feature file at most
16 KB, the feature index at most 40 lines, CLAUDE.md at most 20 KiB, the session log of a progress file at most
20 entry lines. With --fix, older log lines move to the progress archive (a line the archive already holds, as
after a merge of two branches that both archived it, is dropped instead of copied again) and closed items ("- [x] ID") of a feature
file move to <feature>.closed.md. An item or stand-in ID defined twice in a feature and its closed file is an
error. Every feature file has a row in the feature index; closed-items files have none. A link whose text starts with "Corrects" needs a "Corrected by" line under the corrected entry's heading
that links back, and every "Corrected by" line needs the matching "Corrects" link.
"""
import argparse
import re
import subprocess
import sys
import unicodedata
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
INDEX_ROW = re.compile(r"^\|\s*\[[^\]]*\]\(([^)#\s]+\.md)\)")
LOG_HEADING = "## Session Log"
CLOSED_SUFFIX = ".closed.md"
ARCHIVE_SUFFIX = "-archive.md"
MAX_RE_BYTES = 40960
MAX_FEATURE_BYTES = 16384
MAX_INDEX_LINES = 40
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
                match = ENTRY_HEADING.match(line)
                if match:
                    anchor = entry_anchor(match)
                    if len(out) < 2 or out[-2].strip() != f'<a id="{anchor}"></a>':
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
                if line.startswith("Corrected by"):
                    if not leading:
                        errors.append(f"{path.relative_to(root)}:{number + 1}: correction-pairing: 'Corrected by' must stand directly under the heading of its entry")
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
    return [p for p in sorted(directory.glob("*.md")) if p.name != "README.md" and not p.name.endswith(CLOSED_SUFFIX)]


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
    for number, line in enumerate(index.read_text(encoding="utf-8").split("\n")):
        match = INDEX_ROW.match(line)
        if match:
            rows[match.group(1)] = number + 1
    rel = index.relative_to(root)
    for name, number in rows.items():
        if name.endswith(CLOSED_SUFFIX):
            errors.append(f"{rel}:{number}: index-closed-file: {name} is a closed-items file and has no index row")
    for path in feature_files(root):
        if path.name not in rows:
            errors.append(f"{path.relative_to(root)}:1: index-missing-row: {path.name} has no row in {rel}; add one")
    return errors


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
            size = path.stat().st_size
            if size > MAX_RE_BYTES:
                errors.append(f"{path.relative_to(root)}:1: re-file-size: {size} bytes, limit {MAX_RE_BYTES}; split it by subtopic")
    features = feature_dir(root)
    if features.is_dir():
        index = features / "README.md"
        if index.is_file():
            count = len(index.read_text(encoding="utf-8").rstrip("\n").split("\n"))
            if count > MAX_INDEX_LINES:
                errors.append(f"{index.relative_to(root)}:{MAX_INDEX_LINES + 1}: index-file-length: {count} lines, limit {MAX_INDEX_LINES}")
        for path in feature_files(root):
            size = path.stat().st_size
            if size > MAX_FEATURE_BYTES:
                errors.append(f"{path.relative_to(root)}:1: feature-file-size: {size} bytes, limit {MAX_FEATURE_BYTES}; split it by subtopic as <feature>-<subtopic>.md")
    errors.extend(duplicate_id_errors(root))
    errors.extend(index_errors(root))
    errors.extend(session_log_errors(root))
    claude = root / "CLAUDE.md"
    if claude.is_file() and claude.stat().st_size > MAX_CLAUDE_BYTES:
        errors.append(f"CLAUDE.md:1: claude-md-size: {claude.stat().st_size} bytes, limit {MAX_CLAUDE_BYTES}")
    return errors


def process(root, fix, messages=None):
    docs = {}
    if fix:
        for what, count in (("duplicate archive log lines removed", remove_duplicate_archive_lines(root)),
                            ("log lines archived", rotate_session_logs(root)), ("closed items moved", move_closed_items(root))):
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
                if target and not external(target):
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
    parser.add_argument("--fix", action="store_true", help="add missing entry anchors and rewrite section links")
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    messages = []
    errors, added, rewritten, count = process(root, args.fix, messages)
    for message in messages:
        print(message)
    for error in errors:
        print(error)
    if args.fix:
        print(f"{count} files checked, {added} anchors added, {rewritten} links rewritten")
    elif not errors:
        print(f"{count} files checked, all links and anchors resolve")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
