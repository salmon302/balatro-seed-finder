import subprocess, json
p = subprocess.run(['dist\\immolate_synergy_config.exe','--list-results'], capture_output=True, text=True)
raw = p.stdout
print('RAW LENGTH', len(raw))
# Sanitize: escape raw newlines/carriage returns inside quoted strings
out = []
in_str = False
for ch in raw:
    if ch == '"':
        out.append(ch)
        in_str = not in_str
    elif in_str and ch == '\n':
        out.append('\\n')
    elif in_str and ch == '\r':
        out.append('\\r')
    elif in_str and ch == '\t':
        out.append('\\t')
    else:
        out.append(ch)
san = ''.join(out)
print('SANITIZED startswith', san[:200])
try:
    arr = json.loads(san)
    print('Parsed', len(arr), 'names')
    for i, name in enumerate(arr):
        print(i+1, 'len', len(name), 'repr', repr(name))
except Exception as e:
    print('JSON parse failed after sanitize:', e)
    print('Sanitized repr:', repr(san))
    
