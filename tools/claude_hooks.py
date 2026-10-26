#!/usr/bin/env python3
"""Claude Code hooks of this repository (.claude/settings.json): the mechanical part of the CLAUDE.md and lead-round rules.

    python3 tools/claude_hooks.py pre-bash    # PreToolUse Bash: worktree set-up, push, git config, merge finish and commit rules
    python3 tools/claude_hooks.py pre-agent   # PreToolUse Agent: a worker starts only from its brief file
    python3 tools/claude_hooks.py pre-tool    # PreToolUse file tools: worktree set-up
    python3 tools/claude_hooks.py post-edit   # PostToolUse Edit/Write: doc rules and local-only data of the edited file

Claude Code writes the hook input as JSON to stdin. A denied call is answered with a JSON permission decision on stdout;
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

WORKTREES = "/.claude/worktrees/"
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
CLAUDE_SESSION = "Claude-Session"
CO_AUTHOR = "Co-Authored-By:"
MAX_FINDINGS = 25


def deny(reason):
    print(json.dumps({"hookSpecificOutput": {"hookEventName": "PreToolUse", "permissionDecision": "deny", "permissionDecisionReason": reason}}))
    return 0


def git(root, *args):
    return subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)


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


def ensure_artifacts_link(path):
    text = str(path)
    at = text.find(WORKTREES)
    if at < 0:
        return
    main = Path(text[:at])
    tree = main / ".claude" / "worktrees" / text[at + len(WORKTREES):].split("/")[0]
    link = tree / "artifacts"
    if (tree / "CLAUDE.md").is_file() and (main / "artifacts").is_dir() and not os.path.lexists(link):
        os.symlink(main / "artifacts", link)


def unquote(word):
    return word[1:-1] if len(word) > 1 and word[0] == word[-1] and word[0] in "'\"" else word


def base_dir(command, cwd):
    match = LEADING_CD.match(command)
    if not match:
        return Path(cwd)
    return (Path(cwd) / os.path.expanduser(unquote(match.group(1)))).resolve()


def bare(command):
    return QUOTED.sub("''", HEREDOC.sub("", command))


def git_calls(command, base):
    for match in GIT_CALL.finditer(bare(command)):
        target = base
        for option in GIT_DIR_OPTION.findall(match.group(1)):
            target = (target / os.path.expanduser(option)).resolve()
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
        path = base / os.path.expanduser(unquote(name))
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
        if build and (build / "libchase_assets_c.so").is_file():
            env["CHASE_ASSETS_LIB"] = str(build / "libchase_assets_c.so")
            break
    return env


def run_tool(root, env, *args):
    tool = root / "tools" / args[0]
    if not tool.is_file():
        return None
    return subprocess.run([sys.executable, str(tool), *args[1:]], cwd=root, env=env, capture_output=True, text=True)


def refresh(root, env, tool, check, update, target):
    checked = run_tool(root, env, tool, check, target)
    if checked and checked.returncode == 1:
        run_tool(root, env, tool, update, target)


def problem_lines(output, limit=MAX_FINDINGS):
    lines = [line for line in output.split("\n") if line.strip()]
    return "\n".join(lines[:limit]) + (f"\n... {len(lines) - limit} more" if len(lines) > limit else "")


def prepare_commit(root):
    env = assets_env(root)
    unstaged = set(git(root, "diff", "--name-only").stdout.split())
    untracked = set(git(root, "ls-files", "--others", "--exclude-standard").stdout.split())
    before = file_hashes(root)
    refresh(root, env, "priority_report.py", "--check", "--update", "docs/plans/priorities.md")
    run_tool(root, env, "check_doc_links.py", "--root", ".", "--fix")
    refresh(root, env, "priority_report.py", "--check", "--update", "docs/plans/priorities.md")
    refresh(root, env, "progress_report.py", "--check", "--update-readme", "README.md")
    after = file_hashes(root)
    changed = [name for name in after if name not in before or (before[name] != after[name] and name not in unstaged | untracked)]
    trend = "docs/progress-trend.svg"
    if (root / trend).is_file() and git(root, "diff", "--quiet", "--", trend).returncode:
        changed.append(trend)
    if changed:
        git(root, "add", "--", *sorted(set(changed)))
    problems = []
    checked = run_tool(root, env, "check_doc_links.py", "--root", ".")
    if checked and checked.returncode:
        found = [line for line in checked.stdout.split("\n") if line and ": warning: " not in line]
        problems.append("tools/check_doc_links.py fails:\n" + problem_lines("\n".join(found)))
    local = run_tool(root, env, "check_local_data.py", "--root", ".")
    if local and local.returncode:
        found = [line for line in local.stdout.split("\n") if line and not line.startswith("warning")]
        problems.append("tools/check_local_data.py fails (CLAUDE.md \"No local-only data in the repo\"):\n" + problem_lines("\n".join(found)))
    staged = forbidden_staged(root)
    if staged:
        problems.append("game-derived, build or binary files are staged: " + ", ".join(staged))
    return problems


def forbidden_staged(root):
    merge_agent = load_tool(root, "merge_agent")
    pattern = getattr(merge_agent, "FORBIDDEN", None)
    names = git(root, "diff", "--cached", "--name-only").stdout.split("\n")
    bad = [name for name in names if name and pattern and pattern.search(name)]
    for line in git(root, "diff", "--cached", "--numstat").stdout.split("\n"):
        parts = line.split("\t", 2)
        if len(parts) == 3 and parts[0] == "-" and parts[1] == "-" and parts[2] not in bad:
            bad.append(parts[2])
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
    base = base_dir(command, cwd)
    for place in (cwd, base):
        ensure_artifacts_link(place)
    in_agent = bool(data.get("agent_id")) or WORKTREES in str(base) or WORKTREES in cwd
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
            problems = prepare_commit(root)
            if problems:
                return deny("Fix these before committing (the generated README section, priorities.md and the doc fixes are already "
                            "updated and staged):\n\n" + "\n\n".join(problems))
    return 0


def pre_agent(data):
    prompt = data.get("tool_input", {}).get("prompt", "")
    match = BRIEF.search(prompt)
    if not match:
        return 0
    brief = Path(data.get("cwd") or os.getcwd()) / os.path.expanduser(match.group(1))
    if not brief.is_file():
        return deny(f"the brief {match.group(1)} does not exist. Workers start only from the briefs the planner writes in phase 2, "
                    "after the user approved the round (lead-round skill \"Approval and briefs\")")
    return 0


def pre_tool(data):
    ensure_artifacts_link(data.get("cwd") or os.getcwd())
    tool_input = data.get("tool_input", {})
    for key in ("file_path", "path"):
        if tool_input.get(key):
            ensure_artifacts_link(tool_input[key])
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


HOOKS = {"pre-bash": pre_bash, "pre-agent": pre_agent, "pre-tool": pre_tool, "post-edit": post_edit}


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 1 or argv[0] not in HOOKS:
        print(f"usage: claude_hooks.py {{{','.join(HOOKS)}}} < hook input JSON; got: {' '.join(argv) or 'nothing'}", file=sys.stderr)
        return 1
    try:
        data = json.load(sys.stdin)
    except json.JSONDecodeError as error:
        print(f"claude_hooks.py {argv[0]}: the hook input is not JSON: {error}", file=sys.stderr)
        return 1
    return HOOKS[argv[0]](data)


if __name__ == "__main__":
    sys.exit(main())
