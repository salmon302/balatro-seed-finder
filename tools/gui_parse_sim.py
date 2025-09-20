import subprocess, ast, re
p = subprocess.run(['dist\\immolate_synergy_config.exe','--list-results'], capture_output=True, text=True)
out = p.stdout.strip()
print('RAW LEN', len(out))
# try ast.literal_eval
arr = None
try:
    arr = ast.literal_eval(out)
    print('ast literal eval succeeded, len=', len(arr))
except Exception as e:
    print('ast failed:', e)
    # fallback regex
    names = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', out)
    print('regex found', len(names), 'names')
    for i,n in enumerate(names[:10],1):
        print(i, repr(n))
    arr = names
# print all
print('Total extracted:', len(arr))
for i,n in enumerate(arr,1):
    print(i, repr(n))
