# Small test to exercise App.on_pick_random
import os
import sys
import json
import time
from tkinter import Tk

# ensure repo root is on sys.path so `gui` package can be imported
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

from gui.seed_finder_gui import App

# ensure dist exists
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DIST_DIR = os.path.join(REPO_ROOT, 'dist')
os.makedirs(DIST_DIR, exist_ok=True)

# write a sample CSV with header and a few rows
csv_path = os.path.join(DIST_DIR, 'matches_test.csv')
with open(csv_path, 'w', encoding='utf-8') as fh:
    fh.write('seed,level,name\n')
    fh.write('AAAA0001,1,"Synergy One"\n')
    fh.write('BBBB0002,2,"Synergy Two"\n')
    fh.write('CCCC0003,1,"Other"\n')

# Create a hidden Tk root to avoid showing the GUI
root = Tk()
root.withdraw()
app = App(root)
# point chosen csv to our test csv
app.chosen_csv = csv_path
# populate _result_names to include 'Synergy One' and 'Synergy Two'
app._populate_result_names(['Synergy One', 'Synergy Two', 'Other'])
# select a specific synergy
app.result_name_var.set('Synergy Two')

# call on_pick_random and let it show a messagebox; instead we'll call the logic
# directly by copying the relevant code with minimal changes to avoid modal dialogs in tests
import random
seeds = []
with open(csv_path, 'r', encoding='utf-8') as fh:
    fh.readline()
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
            # filter by selected name
            selected_name = app.result_name_var.get()
            ok = False
            if selected_name:
                if name_in_row:
                    ok = (name_in_row == selected_name)
                else:
                    if lvl > 0 and lvl <= len(app._result_names):
                        ok = (app._result_names[lvl-1] == selected_name)
            if ok:
                seeds.append(seed)

print('Seeds matching selected name:', seeds)
if seeds:
    print('Picked:', random.choice(seeds))
else:
    print('No seeds matched')

# cleanup
root.destroy()
