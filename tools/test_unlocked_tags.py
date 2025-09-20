"""
Test unlockedTags: run describe-match for each seed with and without unlockedTags env and assert expected differences.
Usage:
  python tools/test_unlocked_tags.py --filter <FilterName> --seeds AAAAAAAA,BBBBBBBB --tag "Charm Tag"

If no --filter is provided the script will only show the intended commands (no exe calls).
"""
import os
import sys
import json
import subprocess
import argparse
import tempfile
import textwrap

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DIST_DIR = os.path.join(REPO_ROOT, 'dist')


def call_describe(exe, seed, env_path=None, timeout=5):
    args = [exe, '--describe-match', '--seed', seed]
    if env_path:
        args += ['--env', env_path]
    try:
        res = subprocess.run(args, cwd=REPO_ROOT, capture_output=True, text=True, timeout=timeout)
        out = res.stdout.strip()
        if not out:
            return None, res.returncode, res.stderr
        try:
            return json.loads(out), res.returncode, res.stderr
        except Exception:
            return out, res.returncode, res.stderr
    except Exception as e:
        return None, -1, str(e)


def make_env_file(deck='Red Deck', stake='White Stake', unlockedTags=None, selectedOptions=None):
    env = {
        'deck': deck,
        'stake': stake,
        'showman': False,
        'freshProfile': False,
        'freshRun': False,
        'sixesFactor': 1,
        'version': 10106,
        'forceAllContent': True
    }
    if unlockedTags is not None:
        env['unlockedTags'] = unlockedTags
    if selectedOptions is not None:
        env['selectedOptions'] = selectedOptions
    fd, path = tempfile.mkstemp(prefix='env_test_', suffix='.json', dir=DIST_DIR if os.path.isdir(DIST_DIR) else None)
    os.close(fd)
    with open(path, 'w', encoding='utf-8') as fh:
        json.dump(env, fh, indent=2)
    return path


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--filter', required=False, help='Filter base name (e.g. synergy_enum)')
    p.add_argument('--seeds', required=False, help='Comma-separated seeds to test', default='AAAA0001')
    p.add_argument('--tag', required=False, help='Tag name to unlock (human-friendly, e.g. "Charm Tag")', default='Charm Tag')
    p.add_argument('--verbose', action='store_true')
    args = p.parse_args()

    seeds = [s.strip() for s in args.seeds.split(',') if s.strip()]
    if not seeds:
        print('No seeds provided')
        sys.exit(2)

    exe = None
    if args.filter:
        exe_path = os.path.join(DIST_DIR, f'immolate_{args.filter}.exe' if os.name == 'nt' else f'immolate_{args.filter}')
        exe = exe_path
        if not os.path.exists(exe_path):
            print(f'Executable not found: {exe_path}')
            print('Please build the filter first (tools/build.bat <filter> on Windows).')
            sys.exit(3)

    failures = 0
    results = []

    for seed in seeds:
        # create env files
        env_none = make_env_file(unlockedTags=[])
        env_with = make_env_file(unlockedTags=[args.tag])

        if exe is None:
            print('Dry run: would call:')
            print(f'  {"<exe>"} --describe-match --seed {seed} --env {env_none}')
            print(f'  {"<exe>"} --describe-match --seed {seed} --env {env_with}')
            continue

        out_none, rc_none, err_none = call_describe(exe, seed, env_none)
        out_with, rc_with, err_with = call_describe(exe, seed, env_with)

        if args.verbose:
            print(f'--- Seed {seed} ---')
            print('No unlocks ->', out_none, 'rc=', rc_none)
            print('With unlocks ->', out_with, 'rc=', rc_with)

        # Basic assertion: if unlocking the given tag should enable a synergy that was absent
        # we expect the describe outputs to differ in index or name. We'll consider 'None' vs not-None
        # or differing index/name as change. Tests can be adjusted for specific expectations.
        different = (out_none != out_with)
        if not different:
            print(f'WARNING: seed {seed} produced identical describe-match output with and without unlockedTags')
            failures += 1
        else:
            print(f'OK: seed {seed} differs with unlockedTags (good)')

        # cleanup
        try:
            os.remove(env_none)
        except Exception:
            pass
        try:
            os.remove(env_with)
        except Exception:
            pass

    if failures:
        print(f'Failures: {failures}')
        sys.exit(4)
    else:
        print('All checks passed (or dry-run).')
        sys.exit(0)


if __name__ == '__main__':
    main()
