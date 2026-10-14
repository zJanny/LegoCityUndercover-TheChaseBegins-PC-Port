#!/usr/bin/env python3
"""Check (and with --fix repair) relative links and anchors in the repository's Markdown files.

Every dated entry in docs/re/*.md (a "## YYYY-MM-DDTHH:MM:SS..." heading) gets an explicit,
stable anchor line "<a id="ts-YYYYMMDD-HHMMSS"></a>" in front of it. Every link whose text
contains "§YYYY-MM-DDTHH:MM" must point at "file.md#ts-..." of the entry with that timestamp.
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


def markdown_files(root):
    if (root / ".git").exists():
        out = subprocess.run(
            ["git", "-C", str(root), "ls-files", "*.md"], capture_output=True, text=True, check=True
        ).stdout.splitlines()
        return sorted(root / p for p in out if not set(Path(p).parts) & EXCLUDED_DIRS and (root / p).is_file())
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


def process(root, fix):
    docs = {}

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
                        if stamp:
                            hits = resolve_stamp(target_doc, stamp)
                            if len(hits) == 1:
                                new_target = f"{file_part}#{hits[0]}"
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
    return errors, anchors_added, links_rewritten, len(files)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=str(Path(__file__).resolve().parent.parent))
    parser.add_argument("--fix", action="store_true", help="add missing entry anchors and rewrite section links")
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    errors, added, rewritten, count = process(root, args.fix)
    for error in errors:
        print(error)
    if args.fix:
        print(f"{count} files checked, {added} anchors added, {rewritten} links rewritten")
    elif not errors:
        print(f"{count} files checked, all links and anchors resolve")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
