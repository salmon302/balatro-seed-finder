import subprocess, json, sys
p = subprocess.run(['dist\\immolate_synergy_config.exe','--list-results'], capture_output=True, text=True)
out = p.stdout
print('---repr---')
print(repr(out))
print('---raw---')
print(out)
try:
    j = json.loads(out)
    print('JSON OK, length=', len(j))
except Exception as e:
    print('JSON parse error:', e)
    sys.exit(1)
