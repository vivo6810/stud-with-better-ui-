#!/usr/bin/env python3
"""Refresh the generated inventory and patches in difference.md without changing source."""
import difflib
import hashlib
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / '.local/upstream-stud'
REPORT = ROOT / 'difference.md'
MARKER = '<!-- generated-comparison -->'

def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args])

def files(root):
    return set(git(root, 'ls-files', '-z', '--cached', '--others', '--exclude-standard').decode().split('\0')) - {''}

def content(root, name):
    p = root / name
    if p.is_symlink():
        return str(p.readlink()).encode()
    return p.read_bytes()

if not (UPSTREAM / '.git').is_dir():
    raise SystemExit('First clone https://github.com/CatPieLeaf/Stud.git into .local/upstream-stud')
current = files(ROOT) - {'difference.md'}
original = files(UPSTREAM)
changed, same = [], []
for name in sorted(current | original):
    before = content(UPSTREAM, name) if name in original else b''
    after = content(ROOT, name) if name in current else b''
    mode_changed = (name in current and name in original and
                    (ROOT / name).is_symlink() != (UPSTREAM / name).is_symlink())
    if name in current and name in original and before == after and not mode_changed:
        same.append(name)
        continue
    kind = 'added' if name not in original else 'removed' if name not in current else 'modified'
    changed.append((name, kind, before, after))
head = git(ROOT, 'rev-parse', 'HEAD').decode().strip()
upstream = git(UPSTREAM, 'rev-parse', 'HEAD').decode().strip()
parts = [MARKER, '\n## Generated exhaustive file comparison\n',
         f'- Upstream snapshot: `{upstream}`\n- Fork HEAD: `{head}` plus working-tree changes.\n',
         '- Excludes this report to avoid self-recursion, ignored files, Git metadata, and local clones.\n',
         '- Includes tracked files and non-ignored untracked files; text patches include all changed lines.\n',
         '- Symlink targets are compared as text. Executable-bit-only changes are not detected.\n',
         '- Markdown strips trailing whitespace; use Git for a byte-exact applicable patch.\n',
         f'- {len(changed)} changed paths; {len(same)} byte-identical paths.\n',
         '\n| Path | Change | Upstream bytes | Fork bytes |\n|---|---|---:|---:|\n']
for name, kind, before, after in changed:
    parts.append(f'| `{name}` | {kind} | {len(before)} | {len(after)} |\n')
parts.append('\n## Unchanged paths\n\n')
parts.extend(f'- `{name}`\n' for name in same)
parts.append('\n## Complete patches\n\n')
for name, kind, before, after in changed:
    parts.append(f'### `{name}` ({kind})\n\n')
    try:
        if b'\0' in before or b'\0' in after:
            raise UnicodeError()
        a, b = before.decode(), after.decode()
        patch = ''.join(difflib.unified_diff(a.splitlines(True), b.splitlines(True),
                                           fromfile='upstream/' + name, tofile='fork/' + name))
        parts.append('````diff\n' + patch + '\n````\n\n')
    except UnicodeError:
        parts.append(f'Binary content: upstream SHA-256 `{hashlib.sha256(before).hexdigest()}`; '
                     f'fork SHA-256 `{hashlib.sha256(after).hexdigest()}`.\n\n')
preamble = REPORT.read_text().split(MARKER)[0]
REPORT.write_text('\n'.join(line.rstrip() for line in (preamble + ''.join(parts)).splitlines()).rstrip() + '\n')
print(f'Updated {REPORT.name}: {len(changed)} changed paths, {len(same)} unchanged paths')
