#!/usr/bin/env python3
"""Fail when a commit of a pull request lacks a Signed-off-by line (Developer Certificate of Origin 1.1).

    python3 tools/check_dco.py --base BASE_COMMIT --head HEAD_COMMIT [--root DIR]

Checked: every commit reachable from HEAD_COMMIT and not from BASE_COMMIT. Merge commits and commits of bots
(author name ending in [bot]) are not checked.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIGNED_OFF = re.compile(r'^Signed-off-by: \S.* <[^<>\s]+@[^<>\s]+>\s*$', re.MULTILINE)
BOT_AUTHOR = re.compile(r'\[bot\]$')
SEPARATOR = '\x1e'
FIELD = '\x1f'

ADVICE = """Every commit of a pull request needs a Signed-off-by line: your confirmation that you may contribute it under the
project license (Developer Certificate of Origin 1.1, https://developercertificate.org).

To fix it, from your branch:
  git rebase --signoff main        # signs every commit of your branch
  git push --force-with-lease      # force-push your own branch only
For the last commit alone: git commit --amend -s
For new commits: git commit -s
A GitHub noreply address (ID+USERNAME@users.noreply.github.com) is fine as the sign-off email."""


class DcoError(Exception):
    pass


def git(root, *args):
    result = subprocess.run(['git', '-C', str(root), *args], capture_output=True, encoding='utf-8', errors='replace', check=False)
    if result.returncode != 0:
        raise DcoError('git %s failed: %s' % (' '.join(args), result.stderr.strip()))
    return result.stdout


def commits_in_range(root, base, head):
    """[(sha, author name, parent count, message)] of base..head."""
    output = git(root, 'log', '--format=%H' + FIELD + '%an' + FIELD + '%P' + FIELD + '%B' + SEPARATOR, '%s..%s' % (base, head))
    commits = []
    for record in output.split(SEPARATOR):
        if not record.strip():
            continue
        sha, author, parents, message = record.strip('\n').split(FIELD, 3)
        commits.append((sha, author, len(parents.split()), message))
    return commits


def unsigned_commits(commits):
    return [sha for sha, author, parents, message in commits
            if parents <= 1 and not BOT_AUTHOR.search(author) and not SIGNED_OFF.search(message)]


def revision(root):
    def parse(text):
        if not re.fullmatch(r'[0-9A-Za-z][0-9A-Za-z._/-]*', text):
            raise argparse.ArgumentTypeError('not a commit name: %s' % text)
        try:
            git(root, 'rev-parse', '--verify', '--quiet', text + '^{commit}')
        except DcoError:
            raise argparse.ArgumentTypeError('no such commit: %s' % text)
        return text
    return parse


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=Path, default=ROOT, help='repository (default: this checkout)')
    known, _rest = parser.parse_known_args(argv)
    parser.add_argument('--base', required=True, type=revision(known.root), help='commit the pull request branches from')
    parser.add_argument('--head', required=True, type=revision(known.root), help='last commit of the pull request')
    args = parser.parse_args(argv)
    commits = commits_in_range(args.root, args.base, args.head)
    missing = unsigned_commits(commits)
    if missing:
        print('DCO check failed: %d of %d commits have no Signed-off-by line:' % (len(missing), len(commits)))
        for sha in missing:
            print('  %s' % sha)
        print()
        print(ADVICE)
        return 1
    print('DCO check passed: %d commits checked' % len(commits))
    return 0


if __name__ == '__main__':
    sys.exit(main())
