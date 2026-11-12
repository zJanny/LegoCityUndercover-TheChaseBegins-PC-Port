#!/usr/bin/env python3
"""Claude Code hooks of this repository (.claude/settings.json): the mechanical part of the CLAUDE.md and lead-round rules.

    .claude/run_hook.cmd pre-bash    # PreToolUse Bash and PowerShell: worktree set-up, artifacts link and deletions, push, git config, merge finish and commit rules
    .claude/run_hook.cmd pre-agent   # PreToolUse Agent: a worker starts only from its brief file
    .claude/run_hook.cmd pre-tool    # PreToolUse file tools: worktree set-up
    .claude/run_hook.cmd post-edit   # PostToolUse Edit/Write: doc rules and local-only data of the edited file
    .claude/run_hook.cmd subagent-stop   # SubagentStop: a round worker stops only after its report file exists

The launcher .claude/run_hook.cmd starts this file with the first working Python 3.11+ of python3, python and py -3, under sh and cmd.

Claude Code writes the hook input as JSON to stdin. A denied call is answered with a JSON permission decision on stdout, a blocked
stop with a JSON block decision;
findings after an edit go to stderr with exit code 2, which Claude Code shows to the agent. Only checkouts of this
repository are checked; other repositories pass untouched.
"""
import importlib.util
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

import hook_paths

LEADING_CD = re.compile(r"\s*cd\s+(\"[^\"]+\"|'[^']+'|[^\s;&|]+)\s*(?:&&|;)")
HEREDOC = re.compile(r"<<-?\s*(['\"]?)(\w+)\1[^\n]*\n.*?\n\s*\2\s*(?=\n|$)", re.S)
QUOTED = re.compile(r"'[^']*'|\"(?:\\.|[^\"\\])*\"")
GIT_CALL = re.compile(r"(?<![\w./-])git((?:\s+(?:-C\s+\S+|-c\s+\S+|--no-pager|--git-dir=\S+|--work-tree=\S+))*)\s+(commit|push|config)\b([^;&|\n]*)")
GIT_DIR_OPTION = re.compile(r"-C\s+(\S+)")
FINISH = re.compile(r"merge_agent\.py\s+(?:finish|close)\b")
REDIRECT = re.compile(r"\d?>&\d|&>")
TAIL_ONLY = re.compile(r"[^;&|\n]*(?:\|\s*tail\b[^;&|\n]*)?\s*")
MESSAGE_FILE = re.compile(r"(?:^|\s)(?:-F|--file)(?:\s+|=)(\S+)")
MERGE_MESSAGE = re.compile(r"merge_agent\.py\s+(?:finish|close)\b[^;&|\n]*?\s(?:-m|--message)(?:\s+|=)(\S+)")
MESSAGE_GIVEN = re.compile(r"(?:^|\s)(?:-[a-zA-Z]*[mF]|--message|--file)(?=[\s=]|$)")
CONFIG_VALUE_OPTIONS = {"--file", "-f", "--blob", "--type", "--default", "--comment"}
CONFIG_WRITE = {"--unset", "--unset-all", "--add", "--replace-all", "--edit", "-e", "--rename-section", "--remove-section"}
BRIEF = re.compile(r"\bRead your brief (\S+?)\.?\s+and follow")
REPORT_PATH = re.compile(r"artifacts/rounds/[\w.-]+/[\w.-]+\.report\.md")
REDIRECT_WORD = re.compile(r"\d*[<>]")
COMMIT_SHORT_VALUES = "mFCct"
COMMIT_LONG_VALUES = {"--message", "--file", "--author", "--date", "--template", "--cleanup", "--trailer", "--reuse-message", "--reedit-message",
                      "--fixup", "--squash", "--pathspec-from-file"}
SEPARATORS = {";", "&&", "||", "|", "|&", "&", "(", ")"}
WRAPPERS = {"sudo", "command", "env", "time", "nohup", "exec", "nice", "xargs"}
SHELLS = {"bash", "sh", "zsh", "dash"}
DELETERS = {"rm", "unlink", "rmdir"}
FIND_ACTIONS = {"-exec", "-execdir", "-ok", "-okdir"}
AGENT_DELETABLE = re.compile(r"scratch(/.*)?|oracle/[^/]+-out[^/]*(/.*)?")
ASSIGNMENT = re.compile(r"\w+=")
CODE_SUFFIXES = {".cpp", ".cc", ".cxx", ".hpp", ".h"}
WRITE_REDIRECTS = {">", ">>", ">|", "&>", "&>>"}
FILE_WRITERS = {"tee", "cp", "mv", "install"}
GIT_LOCATION_NAMES = "GIT_DIR|GIT_WORK_TREE|GIT_COMMON_DIR|GIT_INDEX_FILE"
GIT_LOCATION_VARIABLE = re.compile(
    rf"(?<!\w)(?:{GIT_LOCATION_NAMES})\}}?\s*\+?=(?!=)"
    rf"|\bexport\s+(?:-\w+\s+)*(?:{GIT_LOCATION_NAMES})\b"
    rf"|\b(?:Set-Item|New-Item|Set-Content|Set-Variable)\b[^;\n|]*\b(?:{GIT_LOCATION_NAMES})\b"
    rf"|SetEnvironmentVariable\s*\(\s*[\"'](?:{GIT_LOCATION_NAMES})\b",
    re.I)
CLAUDE_SESSION = "Claude-Session"
CO_AUTHOR = "Co-Authored-By:"
MAX_FINDINGS = 25


def deny(reason):
    print(json.dumps({"hookSpecificOutput": {"hookEventName": "PreToolUse", "permissionDecision": "deny", "permissionDecisionReason": reason}}))
    return 0


TEXT = {"encoding": "utf-8", "errors": "replace"}
ASSETS_LIBRARIES = ("libchase_assets_c.so", "chase_assets_c.dll", "libchase_assets_c.dll")


def git(root, *args):
    return subprocess.run(["git", "-C", str(root), *args], capture_output=True, **TEXT)


def checkout_of(path):
    path = Path(path)
    while not path.is_dir():
        if path.parent == path:
            return None
        path = path.parent
    result = git(path, "rev-parse", "--show-toplevel")
    if result.returncode != 0:
        return None
    root = Path(result.stdout.strip())
    return root if (root / "CLAUDE.md").is_file() and (root / "tools" / "claude_hooks.py").is_file() else None


def unquote(word):
    return word[1:-1] if len(word) > 1 and word[0] == word[-1] and word[0] in "'\"" else word


def base_dir(command, cwd):
    match = LEADING_CD.match(command)
    if not match:
        return Path(cwd)
    return hook_paths.resolve_in(cwd, unquote(match.group(1)))


def statements(command):
    text = HEREDOC.sub("", command).replace("\\\n", " ")
    found = []
    for line in text.split("\n"):
        try:
            lexer = shlex.shlex(line, posix=True, punctuation_chars=True)
            lexer.whitespace_split = True
            tokens = list(lexer)
        except ValueError:
            tokens = line.split()
        joiner, words, writes, skip = "", [], [], None
        for token in tokens:
            if skip is not None:
                if skip in WRITE_REDIRECTS:
                    writes.append(token)
                skip = None
            elif token in SEPARATORS:
                if words:
                    found.append((joiner, words, writes))
                joiner, words, writes = token, [], []
            elif token and token[0] in "<>&" and set(token) <= set("<>&|"):
                skip = token
                if words and words[-1].isdigit():
                    words.pop()
            else:
                words.append(token)
        if words:
            found.append((joiner, words, writes))
    return found


def command_words(words):
    words = list(words)
    while words and (ASSIGNMENT.match(words[0]) or Path(words[0]).name in WRAPPERS):
        wrapper = Path(words.pop(0)).name
        while wrapper in WRAPPERS and words and words[0].startswith("-"):
            words.pop(0)
    return words


def positional(words):
    found, only = [], False
    for word in words:
        if only or not word.startswith("-") or word == "-":
            found.append(word)
        elif word == "--":
            only = True
    return found


def lexical_path(base, text):
    path = Path(os.path.expandvars(os.path.expanduser(hook_paths.native_path(text))))
    if "$" in str(path):
        return None
    path = Path(base) / path
    return path.parent.resolve() / path.name if path.name not in ("", ".", "..") else path.resolve()


def deleted_paths(words):
    name = Path(words[0]).name
    rest = words[1:]
    if name in DELETERS:
        return [(word, False) for word in positional(rest)]
    if name == "mv":
        if any(word in ("-t", "--target-directory") or word.startswith("--target-directory=") for word in rest):
            return [(word, False) for word in positional(rest[:-1] if rest[-2:-1] == ["-t"] else rest)]
        return [(word, False) for word in positional(rest)[:-1]]
    if name == "find":
        roots = []
        for word in rest:
            if word.startswith(("-", "(", "!")):
                break
            roots.append(word)
        acts = any(word == "-delete" for word in rest) or any(
            word in FIND_ACTIONS and Path(following).name in DELETERS | {"mv"} for word, following in zip(rest, rest[1:]))
        return [(word, word.endswith("/")) for word in roots] if acts else []
    return []


def main_artifacts_part(path):
    for ancestor in (path, *path.parents):
        parent = ancestor.parent
        if (ancestor.name == "artifacts" and not hook_paths.in_worktree(ancestor) and (parent / "CLAUDE.md").is_file()
                and (parent / "tools" / "claude_hooks.py").is_file()):
            return path.relative_to(ancestor).as_posix()
    return None


def artifacts_deletion(command, cwd):
    current = Path(cwd)
    for _, words, _ in statements(hook_paths.drive_paths_with_slashes(command)):
        words = command_words(words)
        if not words:
            continue
        name = Path(words[0]).name
        if name == "cd" and len(words) > 1:
            current = hook_paths.resolve_in(current, words[1])
        elif name in SHELLS and "-c" in words and words.index("-c") + 1 < len(words):
            found = artifacts_deletion(words[words.index("-c") + 1], current)
            if found:
                return found
        for text, follow in deleted_paths(words):
            path = lexical_path(current, text)
            if path is not None and follow:
                path = path.resolve()
            part = main_artifacts_part(path) if path is not None else None
            if part is not None and not AGENT_DELETABLE.fullmatch(part):
                return text
    return None


def written_paths(words, writes):
    name = Path(words[0]).name
    targets = positional(words[1:])
    if name == "tee":
        return writes + targets
    if name in ("cp", "mv", "install") and targets:
        return writes + targets[-1:]
    return writes


def is_scratch_code_in_artifacts(path):
    return path is not None and path.suffix.lower() in CODE_SUFFIXES and "artifacts" in path.parts


def artifacts_code_writes(command, cwd):
    current = Path(cwd)
    for _, words, writes in statements(hook_paths.drive_paths_with_slashes(command)):
        words = command_words(words)
        if words and Path(words[0]).name == "cd" and len(words) > 1:
            current = hook_paths.resolve_in(current, words[1])
        for text in written_paths(words, writes) if words else writes:
            if is_scratch_code_in_artifacts(lexical_path(current, text)):
                return text
    return None


def artifacts_link_requests(command, cwd):
    current = Path(cwd)
    found, guard = [], False
    for joiner, words, _ in statements(hook_paths.drive_paths_with_slashes(command)):
        words = command_words(words)
        name = Path(words[0]).name if words else ""
        if name == "cd" and len(words) > 1:
            current = hook_paths.resolve_in(current, words[1])
        elif name == "ln":
            flags = "".join(word[1:] for word in words[1:] if word.startswith("-") and not word.startswith("--"))
            replaces = "T" in flags or "n" in flags or "--no-target-directory" in words
            targets = positional(words[1:])
            if "s" in flags and len(targets) > 1 and Path(targets[-1]).name == "artifacts" and not replaces:
                found.append((lexical_path(current, targets[-1]), joiner == "||" and guard))
        guard = name in ("[", "test") and "-e" in words
    return found


def bare(command):
    return QUOTED.sub("''", HEREDOC.sub("", command))


def git_calls(command, base):
    for match in GIT_CALL.finditer(bare(command)):
        target = base
        for option in GIT_DIR_OPTION.findall(match.group(1)):
            target = hook_paths.resolve_in(target, option)
        yield target, match.group(2), match.group(3)


def config_writes(arguments):
    try:
        words = shlex.split(arguments)
    except ValueError:
        words = arguments.split()
    if any(w in CONFIG_WRITE for w in words):
        return True
    positional = [w for i, w in enumerate(words) if not w.startswith("-") and (i == 0 or words[i - 1] not in CONFIG_VALUE_OPTIONS)]
    return len(positional) >= 2


def finish_chained(command):
    match = FINISH.search(command)
    if not match:
        return False
    rest = REDIRECT.sub("", bare(command[match.end():]))
    return not TAIL_ONLY.fullmatch(rest)


def message_text(command, base):
    text = command
    read = False
    for name in MESSAGE_FILE.findall(bare(command)) + MERGE_MESSAGE.findall(bare(command)):
        path = hook_paths.join(base, unquote(name))
        if name != "-" and path.is_file():
            text += "\n" + path.read_text(encoding="utf-8", errors="replace")
            read = True
    return text, read


def trailer_problem(command, base, arguments):
    text, read = message_text(command, base)
    if arguments is None and not read:
        return None
    if CLAUDE_SESSION in text:
        return f"the commit message has a {CLAUDE_SESSION} line; commits carry only your own Co-Authored-By line, never a session link (CLAUDE.md \"Git\")"
    if (arguments is None or MESSAGE_GIVEN.search(arguments)) and CO_AUTHOR.lower() not in text.lower():
        return "the commit message has no Co-Authored-By line; end it with your own Co-Authored-By line (CLAUDE.md \"Git\")"
    return None


def commit_form(arguments):
    words = [word for word in arguments.split() if not REDIRECT_WORD.match(word)]
    take_all = False
    paths = []
    index = 0
    while index < len(words):
        word = words[index]
        if word == "--":
            paths += words[index + 1:]
            break
        if word.startswith("--"):
            take_all |= word == "--all"
            index += word in COMMIT_LONG_VALUES
        elif word.startswith("-") and len(word) > 1:
            for position, letter in enumerate(word[1:], 1):
                take_all |= letter == "a"
                if letter in COMMIT_SHORT_VALUES:
                    index += position == len(word) - 1
                    break
        elif word != "''":
            paths.append(word)
        index += 1
    return take_all, paths


def left_out_by_paths(root, base, paths):
    covered = [hook_paths.resolve_in(base, path) for path in paths]
    left = []
    for name in git(root, "diff", "--cached", "--name-only").stdout.split("\n"):
        target = (root / name).resolve()
        if name and not any(target == path or path in target.parents for path in covered):
            left.append(name)
    return left


def file_hashes(root):
    listed = git(root, "ls-files", "--cached", "--others", "--exclude-standard", "-z", "--", "docs", "README.md").stdout
    hashes = {}
    for name in filter(None, listed.split("\0")):
        path = root / name
        if path.is_file():
            hashes[name] = path.read_bytes()
    return hashes


def assets_env(root):
    env = dict(os.environ)
    common = git(root, "rev-parse", "--path-format=absolute", "--git-common-dir").stdout.strip()
    for build in (root / "build", Path(common).parent / "build" if common else None):
        library = next((build / name for name in ASSETS_LIBRARIES if build and (build / name).is_file()), None)
        if library:
            env["CHASE_ASSETS_LIB"] = str(library)
            break
    return env


def run_tool(root, env, *args):
    tool = root / "tools" / args[0]
    if not tool.is_file():
        return None
    return subprocess.run([sys.executable, str(tool), *args[1:]], cwd=root, env={**env, "PYTHONUTF8": "1"}, capture_output=True, **TEXT)


def refresh(root, env, tool, check, update, target):
    checked = run_tool(root, env, tool, check, target)
    if checked and checked.returncode == 1:
        run_tool(root, env, tool, update, target)


def problem_lines(output, limit=MAX_FINDINGS):
    lines = [line for line in output.split("\n") if line.strip()]
    return "\n".join(lines[:limit]) + (f"\n... {len(lines) - limit} more" if len(lines) > limit else "")


README = "README.md"
GENERATED_FILES = (README, "docs/plans/priorities.md", "docs/progress-trend.svg")
PROGRESS_START = "<!-- progress:start -->"
PROGRESS_END = "<!-- progress:end -->"
AGENT_RULES = "CLAUDE.md"
DEVELOPMENT_RULES = "docs/development.md"
MAIN_BRANCHES = ("main", "master")


def on_branch(root):
    name = git(root, "symbolic-ref", "--short", "-q", "HEAD").stdout.strip()
    return bool(name) and name not in MAIN_BRANCHES


def merge_head(root):
    path = git(root, "rev-parse", "--git-path", "MERGE_HEAD").stdout.strip()
    if not path:
        return []
    full = Path(path) if Path(path).is_absolute() else Path(root) / path
    return ["MERGE_HEAD"] if full.exists() else []


def committed_names(root, take_all, *paths, base=()):
    views = [["--cached"], []] if take_all else [["--cached"]]
    found = []
    for view in views:
        names = git(root, "diff", *view, *base, "--name-only", "--", *paths).stdout.split("\n")
        found += [name for name in names if name and name not in found]
    return found


def progress_section(text):
    start, end = text.find(PROGRESS_START), text.find(PROGRESS_END)
    return text[start:end + len(PROGRESS_END)] if 0 <= start < end else None


def shown(root, spec):
    result = git(root, "show", spec)
    return result.stdout if result.returncode == 0 else ""


def progress_section_changed(root, take_all, base):
    before = progress_section(shown(root, f"{(base or ['HEAD'])[0]}:{README}"))
    versions = [shown(root, f":{README}")]
    if take_all and (root / README).is_file():
        versions.append((root / README).read_text(encoding="utf-8", errors="replace"))
    return any(progress_section(text) != before for text in versions)


def staged_generated(root, take_all=False):
    base = merge_head(root)
    found = committed_names(root, take_all, *GENERATED_FILES, base=base)
    if README in found and not progress_section_changed(root, take_all, base):
        found.remove(README)
    return [f"{name} (its progress section)" if name == README else name for name in found]


def rules_without_development_doc(root, take_all=False):
    names = committed_names(root, take_all, AGENT_RULES, DEVELOPMENT_RULES, base=merge_head(root))
    return AGENT_RULES in names and DEVELOPMENT_RULES not in names


def prepare_commit(root, take_all=False):
    env = assets_env(root)
    branch = on_branch(root)
    before = file_hashes(root)
    if not branch:
        refresh(root, env, "priority_report.py", "--check", "--update", "docs/plans/priorities.md")
    run_tool(root, env, "check_doc_links.py", "--root", ".", "--fix")
    if not branch:
        refresh(root, env, "priority_report.py", "--check", "--update", "docs/plans/priorities.md")
        refresh(root, env, "progress_report.py", "--check", "--update-readme", "README.md")
    after = file_hashes(root)
    changed = [name for name in after if before.get(name) != after[name]]
    changed += [name for name in before if name not in after]
    trend = "docs/progress-trend.svg"
    if not branch and (root / trend).is_file() and git(root, "diff", "--quiet", "--", trend).returncode:
        changed.append(trend)
    if branch:
        changed = [name for name in changed if name not in GENERATED_FILES]
    if changed:
        git(root, "add", "-A", "--", *sorted(set(changed)))
    problems = []
    generated = staged_generated(root, take_all) if branch else []
    if generated:
        problems.append("generated files are staged in a branch: " + ", ".join(generated)
                        + "\nOnly tools/merge_agent.py produces them, at the merge (CLAUDE.md \"Git\"); README.md may change outside its "
                        f"{PROGRESS_START} ... {PROGRESS_END} section. Unstage them with git restore --staged "
                        "and drop the working changes with git checkout -- <file>")
    if rules_without_development_doc(root, take_all):
        problems.append(f"{AGENT_RULES} is staged without {DEVELOPMENT_RULES}: {DEVELOPMENT_RULES} is the human version of the rules, "
                        f"so a change to {AGENT_RULES} makes the matching change there in the same commit (CLAUDE.md). Edit "
                        f"{DEVELOPMENT_RULES} to match and stage it")
    checked = run_tool(root, env, "check_doc_links.py", "--root", ".")
    if checked and checked.returncode:
        found = [line for line in checked.stdout.split("\n") if line and ": warning: " not in line]
        problems.append("tools/check_doc_links.py fails:\n" + problem_lines("\n".join(found)))
    local = run_tool(root, env, "check_local_data.py", "--root", ".")
    if local and local.returncode:
        found = [line for line in local.stdout.split("\n") if line and not line.startswith("warning")]
        problems.append("tools/check_local_data.py fails (CLAUDE.md \"No local-only data in the repo\"):\n" + problem_lines("\n".join(found)))
    staged = forbidden_staged(root, take_all)
    if staged:
        problems.append("game-derived, build or binary files are staged: " + ", ".join(staged))
    return problems


def blob_head(root, probe, *specs):
    for spec in specs:
        process = subprocess.Popen(["git", "-C", str(root), "cat-file", "blob", spec], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        data = process.stdout.read(probe)
        process.stdout.close()
        if process.wait() == 0 or data:
            return data
    return None


def working_head(root, probe, name):
    path = root / name
    if path.is_symlink():
        return os.readlink(path).encode()
    if path.is_file():
        with open(path, "rb") as file:
            return file.read(probe)
    return blob_head(root, probe, f":{name}")


def forbidden_staged(root, take_all=False):
    rules = load_tool(root, "forbidden_files") or load_tool(Path(__file__).resolve().parent.parent, "forbidden_files")
    probe = rules.BINARY_PROBE
    readers = [(["--cached"], lambda name: blob_head(root, probe, f":{name}", f"HEAD:{name}"))]
    if take_all:
        readers.append(([], lambda name: working_head(root, probe, name)))
    bad = []
    for view, read_head in readers:
        names = [name for name in git(root, "diff", *view, "--name-only").stdout.split("\n") if name]
        bad += [f"{name} ({reason})" for name, reason in rules.check_paths(names, read_head) if not any(b.startswith(name + " (") for b in bad)]
    return bad


def load_tool(root, name):
    path = root / "tools" / f"{name}.py"
    if not path.is_file():
        return None
    sys.path.insert(0, str(root / "tools"))
    try:
        spec = importlib.util.spec_from_file_location(f"repo_{name}", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        sys.path.remove(str(root / "tools"))


def pre_bash(data):
    command = data.get("tool_input", {}).get("command", "")
    cwd = data.get("cwd") or os.getcwd()
    if GIT_LOCATION_VARIABLE.search(command):
        return deny("commands never point git at another checkout through GIT_DIR, GIT_WORK_TREE, GIT_COMMON_DIR or GIT_INDEX_FILE: a worktree "
                    "shares one .git, so such a setting (also inside wsl.exe -e bash -c '...') sends every later git command, including the "
                    "user's, to the wrong work tree and config. Use git -C <folder> in a checkout of its own. For the Linux check on Windows, "
                    "commit first, then clone the branch inside WSL (git clone -q --branch <branch> <main checkout> ~/wslcheck/<agent>) and "
                    "run the checks in that clone (CLAUDE.md \"Git\")")
    base = base_dir(command, cwd)
    in_agent = bool(data.get("agent_id")) or hook_paths.in_worktree(base) or hook_paths.in_worktree(cwd)
    unguarded = [link for link, guarded in artifacts_link_requests(command, cwd) if not guarded and link is not None]
    if hook_paths.IS_WINDOWS or not unguarded:
        for place in (cwd, base):
            hook_paths.ensure_artifacts_link(place)
    if any(link.is_dir() for link in unguarded):
        return deny("`artifacts` already exists here, so `ln -s ... artifacts` would create a link inside the folder it points to, in the "
                    "user's artifacts/. The link is in place: skip this step. Use the guarded form `[ -e artifacts ] || ln -s <main checkout>/artifacts "
                    "artifacts` if you must repeat it (lead-round skill, worker brief)")
    code = artifacts_code_writes(command, cwd)
    if code:
        return deny(scratch_code_reason(code))
    if in_agent:
        victim = artifacts_deletion(command, cwd)
        if victim:
            return deny(f"agents never delete, move or unlink anything in the main checkout's artifacts/ ('{victim}'): it holds the user's "
                        "save states and rounds. Ask `main` with SendMessage instead. The only paths an agent may delete there are its own "
                        "scratch, artifacts/oracle/<agent>-out folders and artifacts/scratch/ (CLAUDE.md \"No local-only data in the repo\")")
    if finish_chained(command) and checkout_of(base):
        return deny("merge_agent.py finish runs alone: no &&, ; or || after it, only '2>&1 | tail -N'. Start the next command only after "
                    "finish printed 'merged;' (lead-round skill \"The finish gate\")")
    if MERGE_MESSAGE.search(bare(command)) and checkout_of(base):
        problem = trailer_problem(command, base, None)
        if problem:
            return deny(problem)
    for target, sub, arguments in git_calls(command, base):
        root = checkout_of(target)
        if not root:
            continue
        if sub == "push":
            if in_agent:
                return deny("agents never push; only the lead pushes, after the user allowed it this session (CLAUDE.md \"Git\")")
            if git(root, "diff", "--cached", "--quiet").returncode:
                return deny("the index still holds staged changes, so the last merge_agent.py finish did not commit; fix that before pushing "
                            "(lead-round skill \"The finish gate\")")
        elif sub == "config":
            if config_writes(arguments):
                return deny("agents never change git config; the commit identity is set repo-locally by the user (CLAUDE.md \"Git\")")
        elif sub == "commit" and "--dry-run" not in arguments:
            problem = trailer_problem(command, base, arguments)
            if problem:
                return deny(problem)
            take_all, paths = commit_form(arguments)
            problems = prepare_commit(root, take_all)
            left = left_out_by_paths(root, target, paths) if paths else []
            if left:
                problems.append("git commit with paths ignores the index, so these staged files would be left out of the commit: " + ", ".join(left)
                                + "\nStage what belongs in the commit with git add and run git commit without paths")
            if problems:
                return deny("Fix these before committing (the doc fixes are already applied and staged; on main also the "
                            "generated README section and priorities.md):\n\n" + "\n\n".join(problems))
    return 0


def pre_agent(data):
    prompt = data.get("tool_input", {}).get("prompt", "")
    match = BRIEF.search(prompt)
    if not match:
        return 0
    brief = hook_paths.join(data.get("cwd") or os.getcwd(), match.group(1))
    if not brief.is_file():
        return deny(f"the brief {match.group(1)} does not exist. Workers start only from the briefs the planner writes in phase 2, "
                    "after the user approved the round (lead-round skill \"Approval and briefs\")")
    return 0


def scratch_code_reason(path):
    return (f"'{path}' is C++ source inside an artifacts/ folder, which is shared with the user and the lead (a worktree's artifacts is a link "
            "to the main one), and its compiler errors flood their context. Write throwaway probes into the scratchpad directory named in your "
            "system prompt instead; code that belongs to the work goes under src/, tests/ or tools/")


def pre_tool(data):
    cwd = data.get("cwd") or os.getcwd()
    hook_paths.ensure_artifacts_link(cwd)
    tool_input = data.get("tool_input", {})
    if data.get("tool_name") in ("Write", "Edit") and tool_input.get("file_path"):
        if is_scratch_code_in_artifacts(lexical_path(cwd, tool_input["file_path"])):
            return deny(scratch_code_reason(tool_input["file_path"]))
    for key in ("file_path", "path"):
        if tool_input.get(key):
            hook_paths.ensure_artifacts_link(tool_input[key])
    return 0


def first_prompt(transcript):
    try:
        with open(transcript, encoding="utf-8") as lines:
            for line in lines:
                record = json.loads(line)
                if record.get("type") != "user":
                    continue
                content = record.get("message", {}).get("content")
                if isinstance(content, list):
                    content = " ".join(block.get("text", "") for block in content if isinstance(block, dict))
                return (content, record.get("cwd")) if isinstance(content, str) else (None, None)
    except (OSError, ValueError, AttributeError):
        return None, None
    return None, None


def worker_report(data):
    cwd = data.get("cwd")
    transcript = data.get("agent_transcript_path")
    if not isinstance(cwd, str) or not isinstance(transcript, str):
        return None
    prompt, started_in = first_prompt(transcript)
    if not hook_paths.in_worktree(cwd) and isinstance(started_in, str):
        cwd = started_in
    main = hook_paths.main_of_worktree(cwd)
    match = BRIEF.search(prompt or "")
    if main is None or not match:
        return None
    brief = hook_paths.join(cwd, match.group(1))
    try:
        named = REPORT_PATH.findall(brief.read_text(encoding="utf-8"))
    except OSError:
        return None
    return main / named[-1] if named else None


def subagent_stop(data):
    report = worker_report(data)
    if report is None or (report.is_file() and report.stat().st_size > 0):
        return 0
    if data.get("stop_hook_active"):
        print(f"claude_hooks.py subagent-stop: the report {report.name} is still missing, but this stop was blocked once already; "
              "letting the worker stop", file=sys.stderr)
        return 0
    print(json.dumps({"decision": "block", "reason": f"your report file {report} does not exist yet. Write it "
                      "now, as your brief's \"End of session\" step 3 says, then finish with the same text as your final message "
                      "(lead-round skill, worker brief)"}))
    return 0


def post_edit(data):
    path = data.get("tool_input", {}).get("file_path")
    if not path:
        return 0
    path = Path(path).resolve()
    root = checkout_of(path.parent)
    if not root or not path.is_file():
        return 0
    name = path.relative_to(root).as_posix()
    if git(root, "check-ignore", "-q", "--", name).returncode == 0:
        return 0
    findings = []
    if name.endswith(".md") and name.startswith("docs/"):
        checked = run_tool(root, dict(os.environ), "check_doc_links.py", "--root", ".")
        if checked:
            findings += [line for line in checked.stdout.split("\n") if line.startswith(name + ":")]
    local = load_tool(root, "check_local_data")
    if local and hasattr(local, "violations"):
        created = {state for script in (root / "tools" / "oracle").glob("*.txt") for state in local.SAVESTATE_LINE.findall(script.read_text(encoding="utf-8"))}
        text = path.read_text(encoding="utf-8", errors="replace")
        findings += [f"{name}:{number}: {line.strip()[:160]} — {why} (CLAUDE.md \"No local-only data in the repo\")"
                     for number, line, why in local.violations(name, text, created)]
    if not findings:
        return 0
    print(f"Rule findings in {name} after this edit (fix them now; a paired edit still to come may explain a link finding):\n"
          + problem_lines("\n".join(findings)), file=sys.stderr)
    return 2


HOOKS = {"pre-bash": pre_bash, "pre-agent": pre_agent, "pre-tool": pre_tool, "post-edit": post_edit, "subagent-stop": subagent_stop}


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 1 or argv[0] not in HOOKS:
        print(f"usage: claude_hooks.py {{{','.join(HOOKS)}}} < hook input JSON; got: {' '.join(argv) or 'nothing'}", file=sys.stderr)
        return 1
    sys.stdin.reconfigure(encoding="utf-8-sig", errors="replace")
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(encoding="utf-8", errors="replace")
    try:
        data = json.load(sys.stdin)
    except json.JSONDecodeError as error:
        print(f"claude_hooks.py {argv[0]}: the hook input is not JSON: {error}", file=sys.stderr)
        return 1
    return HOOKS[argv[0]](data)


if __name__ == "__main__":
    sys.exit(main())
