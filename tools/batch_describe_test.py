"""
Batch describe-match runner

Usage examples:
  python tools/batch_describe_test.py --filters synergy_config,synergy_enum --seeds AAA9UYXQ,AAAAAAAA --envs none,trib

Env aliases supported (none/empty): no env, 'trib' will create an env with unlockedJokers ['Triboulet','Baron','Shoot the Moon']

Outputs a JSON report to dist/batch_report_<timestamp>.json
"""
import os, sys, json, subprocess, tempfile, argparse, datetime

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DIST = os.path.join(REPO_ROOT, 'dist')

def make_env_alias(alias):
    if not alias or alias in ('none',''):
        return None
    if alias == 'trib':
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedJokers':['Triboulet','Baron','Shoot the Moon']}
    if alias == 'trib_only':
        # Only unlock Triboulet (legendary) to see if Triboulet alone triggers the synergy
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedJokers':['Triboulet']}
    if alias == 'trib_all':
        # Unlock Triboulet plus common face multipliers (Baron, Shoot the Moon, Photograph)
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedJokers':['Triboulet','Baron','Shoot the Moon','Photograph']}
    if alias == 'photo':
        # Only unlock Photograph to test if face-multiplier presence alone is sufficient
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedJokers':['Photograph']}
    if alias == 'trib_combo':
        # Unlock Triboulet and face multipliers plus a Charm Tag to exercise combo synergies
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedJokers':['Triboulet','Baron','Shoot the Moon','Photograph'],'unlockedTags':['Charm Tag']}
    if alias == 'charm':
        return {'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True,'unlockedTags':['Charm Tag']}
    return None


def call_exe(exe, seed, env_path=None, timeout=8):
    args = [exe, '--describe-match', '--seed', seed]
    if env_path:
        args += ['--env', env_path]
    try:
        res = subprocess.run(args, cwd=REPO_ROOT, capture_output=True, text=True, timeout=timeout)
        out = res.stdout.strip()
        return out, res.returncode, res.stderr
    except Exception as e:
        return None, -1, str(e)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--filters', required=True, help='comma-separated filter base names (e.g. synergy_config)')
    p.add_argument('--seeds', required=True, help='comma-separated seeds')
    p.add_argument('--envs', required=False, help='comma-separated env aliases (none,trib,charm)')
    p.add_argument('--trials', type=int, default=1, help='number of trials to run (default 1)')
    p.add_argument('--only-diff', action='store_true', help='only include runs where outputs differ across envs')
    p.add_argument('--out', required=False, help='output report path (defaults to dist/batch_report_<ts>.json)')
    args = p.parse_args()

    filters = [f.strip() for f in args.filters.split(',') if f.strip()]
    seeds = [s.strip() for s in args.seeds.split(',') if s.strip()]
    env_aliases = [e.strip() for e in (args.envs or 'none').split(',')]

    report = {'runs': [], 'filters': filters, 'seeds': seeds, 'envs': env_aliases, 'timestamp': datetime.datetime.utcnow().isoformat()}

    temp_env_files = []
    try:
        for env_alias in env_aliases:
            env_obj = make_env_alias(env_alias)
            if env_obj is None:
                temp_env_files.append((env_alias, None))
                continue
            fd, path = tempfile.mkstemp(prefix='env_batch_', suffix='.json', dir=DIST if os.path.isdir(DIST) else None)
            os.close(fd)
            with open(path,'w',encoding='utf-8') as fh:
                json.dump(env_obj, fh, indent=2)
            temp_env_files.append((env_alias, path))

        for filt in filters:
            exe = os.path.join(DIST, f'immolate_{filt}.exe' if os.name == 'nt' else f'immolate_{filt}')
            if not os.path.exists(exe):
                print(f'Executable for filter {filt} not found at {exe}; skipping')
                continue
            for seed in seeds:
                for trial in range(1, args.trials + 1):
                    outs = {}
                    errs = {}
                    codes = {}
                    for env_alias, env_path in temp_env_files:
                        out, rc, err = call_exe(exe, seed, env_path)
                        outs[env_alias] = out
                        errs[env_alias] = err
                        codes[env_alias] = rc
                        print(filt, seed, f'trial={trial}', env_alias, '->', out)

                    # determine if outputs differ across env aliases
                    unique_outs = set(outs.values())
                    include = True
                    if args.only_diff and len(unique_outs) <= 1:
                        include = False

                    if include:
                        entry = {'filter':filt, 'seed':seed, 'trial':trial, 'outs':outs, 'rcs':codes, 'errs':errs}
                        report['runs'].append(entry)
        # write report
        ts = datetime.datetime.utcnow().strftime('%Y%m%d_%H%M%S')
        rep_path = os.path.join(DIST, f'batch_report_{ts}.json')
        with open(rep_path,'w',encoding='utf-8') as fh:
            json.dump(report, fh, indent=2)
        print('Report written to', rep_path)
    finally:
        for a,p in temp_env_files:
            if p and os.path.exists(p):
                try: os.remove(p)
                except: pass

if __name__ == '__main__':
    main()
