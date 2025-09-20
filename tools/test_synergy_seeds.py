"""
Test seeds that match a named synergy by calling the compiled executable to verify reported match.
Usage:
  python tools/test_synergy_seeds.py --csv dist/matches_test.csv --name "Synergy Two" --filter <FilterName>
If --filter is omitted the script will only list seeds matching the name and skip calling the executable.
"""
import os
import sys
import argparse
import json
import subprocess

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DIST_DIR = os.path.join(REPO_ROOT, 'dist')

if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)


def read_csv(csv_path):
    rows = []
    with open(csv_path, 'r', encoding='utf-8') as fh:
        header = fh.readline()
        for line in fh:
            parts = []
            cur = ''
            in_quote = False
            s = line.strip()
            for ch in s:
                if ch == '"':
                    in_quote = not in_quote
                    cur += ch
                elif ch == ',' and not in_quote:
                    parts.append(cur.strip())
                    cur = ''
                else:
                    cur += ch
            if cur:
                parts.append(cur.strip())
            if len(parts) >= 2:
                seed = parts[0].strip().strip('"')
                lvl = 0
                name_in_row = ''
                try:
                    lvl = int(parts[1])
                except Exception:
                    lvl = 1
                if len(parts) >= 3:
                    name_in_row = parts[2].strip()
                    if name_in_row.startswith('"') and name_in_row.endswith('"'):
                        name_in_row = name_in_row[1:-1]
                rows.append((seed, lvl, name_in_row))
    return rows


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--csv', default=os.path.join(DIST_DIR, 'matches_test.csv'))
    p.add_argument('--name', required=True)
    p.add_argument('--filter', required=False, help='Filter name to locate compiled immolate executable')
    args = p.parse_args()

    rows = read_csv(args.csv)
    selected = [s for (s, lvl, name) in rows if name == args.name]
    print(f'Found {len(selected)} seeds in {args.csv} matching name "{args.name}"')
    for s in selected:
        print(' -', s)

    if args.filter:
        exe = os.path.join(DIST_DIR, f'immolate_{args.filter}.exe' if os.name == 'nt' else f'immolate_{args.filter}')
        if not os.path.exists(exe):
            print(f'Executable not found: {exe} -> skipping describe-match step')
            return
        print(f'Calling {exe} --describe-match --seed <seed> for each seed')
        for s in selected:
            try:
                res = subprocess.run([exe, '--describe-match', '--seed', s], cwd=REPO_ROOT, capture_output=True, text=True, timeout=5)
                if res.stdout:
                    try:
                        j = json.loads(res.stdout.strip())
                        print(s, '->', j)
                    except Exception:
                        print(s, '-> output not JSON:', res.stdout.strip())
                else:
                    print(s, '-> no output')
            except Exception as e:
                print(s, '-> error calling exe:', e)

if __name__ == '__main__':
    main()
