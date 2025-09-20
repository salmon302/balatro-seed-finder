import json
import sys
from collections import defaultdict, Counter

REPORT_PATH = sys.argv[1] if len(sys.argv) > 1 else 'dist/batch_report_20250917_050017.json'

def load_report(path):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def summarize(report):
    per_seed = defaultdict(list)
    for run in report.get('runs', []):
        key = run['seed']
        per_seed[key].append(run)

    summary = {}
    for seed, runs in per_seed.items():
        # group by trial
        trials = defaultdict(dict)
        envs = set()
        for r in runs:
            trial = r['trial']
            trials[trial][r.get('outs') and list(r['outs'].keys())[0] or 'unknown'] = r
            # actually record all envs from outs
            for env in r.get('outs', {}).keys():
                envs.add(env)
        envs = sorted(list(envs))

        # For each trial, compare outputs across envs
        diffs = []
        freq = Counter()
        rep_errs = defaultdict(Counter)
        for trial, data in trials.items():
            # data: map env->run (but above mapping was wrong); fix by scanning runs for this trial
            trial_runs = [r for r in runs if r['trial'] == trial]
            out_by_env = {env: r['outs'].get(env, None) for env in envs for r in trial_runs if env in r['outs']}
            # normalize: pick a baseline env (first)
            baseline = None
            if envs:
                baseline = envs[0]
            # Compare
            values = {env: next((r['outs'][env] for r in trial_runs if env in r['outs']), None) for env in envs}
            unique_vals = set(values.values())
            if len(unique_vals) > 1:
                diffs.append({'trial': trial, 'values': values})
            # frequency counting per env->value
            for env, val in values.items():
                freq[(env, val)] += 1
            # collect representative errors
            for env in envs:
                err = next((r['errs'].get(env,'') for r in trial_runs if env in r['errs']), '')
                if err:
                    # take first non-empty line
                    first_line = err.splitlines()[0] if err.splitlines() else ''
                    rep_errs[env][first_line] += 1

        summary[seed] = {
            'envs': envs,
            'trials': len(trials),
            'diff_trials': len(diffs),
            'diff_examples': diffs[:5],
            'frequencies': {f'{env}|{val}': count for (env,val),count in freq.items()},
            'rep_errs': {env: rep_errs[env].most_common(3) for env in envs}
        }
    return summary

if __name__ == '__main__':
    report = load_report(REPORT_PATH)
    s = summarize(report)
    print(json.dumps(s, indent=2))
