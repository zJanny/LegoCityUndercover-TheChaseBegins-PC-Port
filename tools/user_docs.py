"""Mechanical checks of the user docs (docs/*.md outside the subfolders, and README.md) against the code.

A change that adds or removes a command-line option or a chase-* program fails the check until the user docs say so,
and the user docs cannot keep naming an option, program or file that no longer exists.
"""
import re
import subprocess
from pathlib import Path

CODE_SPAN = re.compile(r"`([^`\n]+)`")
FENCE = re.compile(r"^\s*(```|~~~)")
OPTION = re.compile(r"(?<![\w-])--[a-z][a-z0-9-]*(?![\w-])")
SOURCE_OPTION = re.compile(r'"(--[a-z][a-z0-9-]*)"')
PROGRAM = re.compile(r"add_executable\((chase-[a-z0-9-]+)")
PROGRAM_NAME = re.compile(r"(?<![\w/.-])chase-[a-z][a-z0-9-]*(?![\w-])")
PATH = re.compile(r"(?<![\w/.-])((?:tools|src|tests|docs)/[\w./-]*[\w/])(?![\w./-])")
SOURCE_SUFFIXES = {".cpp", ".hpp", ".py", ".txt", ".cmake", ".sh", ".md", ".glsl", ".json"}
SKIPPED_DIRS = {"artifacts", "build", "graphify-out", ".git", "node_modules"}


def user_doc_files(root):
    docs = sorted((root / "docs").glob("*.md"))
    readme = root / "README.md"
    return docs + ([readme] if readme.is_file() else [])


def code_fragments(path):
    inside = False
    for number, line in enumerate(path.read_text(encoding="utf-8").split("\n"), 1):
        if FENCE.match(line):
            inside = not inside
        elif inside:
            yield number, line
        else:
            for match in CODE_SPAN.finditer(line):
                yield number, match.group(1)


def repository_files(root):
    if (root / ".git").exists():
        out = subprocess.run(["git", "-C", str(root), "ls-files", "--cached", "--others", "--exclude-standard"], capture_output=True, text=True, check=True).stdout.splitlines()
        return [root / p for p in out if (root / p).is_file() and not set(Path(p).parts) & SKIPPED_DIRS]
    return [p for p in root.rglob("*") if p.is_file() and not set(p.relative_to(root).parts) & SKIPPED_DIRS]


def is_user_doc(root, path):
    return path.parent == root / "docs" and path.suffix == ".md" or path == root / "README.md"


def source_options(root, files):
    found = set()
    for path in files:
        if path.suffix in (".cpp", ".hpp") and path.is_relative_to(root / "src"):
            found.update(SOURCE_OPTION.findall(path.read_text(encoding="utf-8", errors="ignore")))
    return found


def repository_options(root, files):
    found = set()
    for path in files:
        if path.suffix in SOURCE_SUFFIXES and not is_user_doc(root, path) and not path.is_relative_to(root / "docs"):
            found.update(OPTION.findall(path.read_text(encoding="utf-8", errors="ignore")))
    return found


def user_doc_errors(root):
    docs = user_doc_files(root)
    cmake = root / "CMakeLists.txt"
    errors = []
    if docs and (root / "src").is_dir() and cmake.is_file():
        files = repository_files(root)
        spoken = "\n".join(path.read_text(encoding="utf-8") for path in docs)
        for option in sorted(source_options(root, files)):
            if not re.search(rf"(?<![\w-]){re.escape(option)}(?![\w-])", spoken):
                errors.append(f"docs/commands.md:1: user-doc-option-undocumented: the programs accept {option} but no user doc names it; describe it in docs/commands.md")
        programs = set(PROGRAM.findall(cmake.read_text(encoding="utf-8")))
        for program in sorted(programs):
            if not re.search(rf"(?<![\w/.-]){re.escape(program)}(?![\w-])", spoken):
                errors.append(f"docs/commands.md:1: user-doc-program-undocumented: CMake builds {program} but no user doc names it; describe it in docs/commands.md")
        known = repository_options(root, files)
        for path in docs:
            for number, fragment in code_fragments(path):
                where = f"{path.relative_to(root)}:{number}"
                for option in OPTION.findall(fragment):
                    if option not in known:
                        errors.append(f"{where}: user-doc-option-unknown: {option} is in no source, tool or test; remove it from the doc or fix its name")
                for program in PROGRAM_NAME.findall(fragment):
                    if program not in programs:
                        errors.append(f"{where}: user-doc-program-unknown: CMake builds no {program}; remove it from the doc or fix its name")
    for path in docs:
        for number, fragment in code_fragments(path):
            for target in PATH.findall(fragment):
                if "*" not in target and not (root / target).exists():
                    errors.append(f"{path.relative_to(root)}:{number}: user-doc-path-missing: {target} does not exist")
    return errors
