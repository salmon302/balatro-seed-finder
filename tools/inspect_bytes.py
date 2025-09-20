import subprocess
out = subprocess.run(['dist\\immolate_synergy_config.exe','--list-results'], capture_output=True)
raw = out.stdout
print('TYPE:', type(raw))
print('LENGTH:', len(raw))
# show repr
print('REPR START:', repr(raw[:300]))
# find problem names
for name in ['Steel Joker', 'Blueprint/Brainstorm']:
    i = raw.find(name)
    print(name, 'found at', i)
    if i!=-1:
        seg = raw[max(0,i-10):i+40]
        print('SEG REPR:', repr(seg))
        print('BYTES:', list(seg.encode('utf-8')))
