import json, os, subprocess
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),'..'))
DIST = os.path.join(ROOT,'dist')
exe = os.path.join(DIST,'immolate_synergy_config.exe')
seed='AAA9UYXQ'

def make(path, jokers):
    env={'deck':'Red Deck','stake':'White Stake','showman':False,'freshProfile':False,'freshRun':False,'sixesFactor':1,'version':10106,'forceAllContent':True}
    if jokers is not None:
        env['unlockedJokers']=jokers
    with open(path,'w',encoding='utf-8') as f:
        json.dump(env,f,indent=2)

f1=os.path.join(DIST,'env_tmp_none.json')
f2=os.path.join(DIST,'env_tmp_with.json')
make(f1, [])
make(f2, ['Triboulet','Baron','Shoot the Moon'])

for p in [f1,f2]:
    print('Calling exe with env=',p)
    res = subprocess.run([exe,'--describe-match','--seed',seed,'--env',p],cwd=ROOT,capture_output=True,text=True,timeout=10)
    print('RC=',res.returncode)
    print(res.stdout.strip())
    if res.stderr.strip(): print('ERR:',res.stderr)

# cleanup
try: os.remove(f1)
except: pass
try: os.remove(f2)
except: pass
