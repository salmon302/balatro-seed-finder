import os
import sys
import subprocess
import threading
import queue
import time
import json
import datetime
from tkinter import Tk, StringVar, IntVar, BooleanVar, ttk, filedialog, messagebox, scrolledtext


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
TOOLS_DIR = os.path.join(REPO_ROOT, 'tools')
FILTERS_DIR = os.path.join(REPO_ROOT, 'filters')
DIST_DIR = os.path.join(REPO_ROOT, 'dist')


def list_filters():
    names = []
    try:
        for fname in os.listdir(FILTERS_DIR):
            if fname.endswith('_filter.hpp'):
                base = fname[:-len('_filter.hpp')]
                names.append(base)
    except FileNotFoundError:
        pass
    return sorted(names)


class ProcessRunner:
    def __init__(self):
        self.proc = None
        self.q = queue.Queue()
        self.thread = None

    def start(self, args, cwd=None):
        if self.proc is not None:
            raise RuntimeError('Process already running')
        self.proc = subprocess.Popen(
            args,
            cwd=cwd or REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True,
        )
        self.thread = threading.Thread(target=self._pump, daemon=True)
        self.thread.start()

    def _pump(self):
        assert self.proc and self.proc.stdout
        for line in self.proc.stdout:
            self.q.put(line)
        rc = self.proc.wait()
        self.q.put(f"\n[process exited with code {rc}]\n")

    def stop(self):
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.proc = None

    def read_available(self):
        lines = []
        try:
            while True:
                lines.append(self.q.get_nowait())
        except queue.Empty:
            pass
        return ''.join(lines)


class App:
    def __init__(self, root: Tk):
        self.root = root
        root.title('Balatro Seed Finder')
        root.geometry('900x600')

        self.filter_var = StringVar()
        self.seed_var = StringVar()
        self.threads_var = IntVar(value=0)
        self.debug_var = BooleanVar(value=False)
        self.resume_var = BooleanVar(value=False)
        # Number of seeds to collect during random pick (reservoir size)
        self.reservoir_size_var = IntVar(value=6)
        # Fresh profile / fresh run control
        self.fresh_profile_var = BooleanVar(value=False)
        self.fresh_run_var = BooleanVar(value=False)

        # Placeholders; actual widgets are created in _build_ui with correct parent
        self.build_output = None
        self.run_output = None

        self.process_runner = ProcessRunner()
        self.last_run_args = None
        self._missing_option_prompted = False
        self.poll_outputs()
        self._build_ui()
        # Ensure tag summary reflects initial unlocked_tags
        try:
            self._update_tags_summary()
        except Exception:
            pass
        self._load_filters()

        # GUI state (persisted across runs): mapping filter_name -> selected result name
        self._gui_state_path = os.path.join(DIST_DIR, 'gui_state.json')
        self._gui_state = {}
        self._load_gui_state()

        # selectedOptions storage (None => not explicitly set)
        self.selected_options_list = None
        # track last-loaded env file so runs will use it by default (unless "Use latest env" is checked)
        self.loaded_env_path = None

    def _build_ui(self):
        frm = ttk.Frame(self.root, padding=10)
        frm.pack(fill='both', expand=True)

        row = 0
        ttk.Label(frm, text='Filter:').grid(column=0, row=row, sticky='w')
        self.filter_cb = ttk.Combobox(frm, textvariable=self.filter_var, state='readonly', width=30)
        self.filter_cb.grid(column=1, row=row, sticky='w')
        ttk.Button(frm, text='Refresh', command=self._load_filters).grid(column=2, row=row, padx=5)

        row += 1
        ttk.Label(frm, text='Seed (optional):').grid(column=0, row=row, sticky='w', pady=(8,0))
        ttk.Entry(frm, textvariable=self.seed_var, width=20).grid(column=1, row=row, sticky='w', pady=(8,0))
        # Label to show matched rule name when running in debug mode
        self.match_name_var = StringVar(value='')
        ttk.Label(frm, textvariable=self.match_name_var, foreground='blue').grid(column=2, row=row, sticky='w', padx=(8,0))
        ttk.Label(frm, text='Threads (0=auto):').grid(column=3, row=row, sticky='w', pady=(8,0))
        ttk.Spinbox(frm, from_=0, to=256, textvariable=self.threads_var, width=8).grid(column=4, row=row, sticky='w', pady=(8,0))
        ttk.Checkbutton(frm, text='Debug single seed', variable=self.debug_var).grid(column=5, row=row, sticky='w', padx=(10,0))
        ttk.Checkbutton(frm, text='Avoid reprocessing (resume)', variable=self.resume_var).grid(column=6, row=row, sticky='w', padx=(10,0))
        ttk.Label(frm, text='Offset:').grid(column=7, row=row, sticky='w', padx=(8,0))
        self.offset_var = IntVar(value=0)
        ttk.Spinbox(frm, from_=0, to=10000000, textvariable=self.offset_var, width=10).grid(column=8, row=row, sticky='w')
        ttk.Label(frm, text='Margin:').grid(column=9, row=row, sticky='w', padx=(8,0))
        self.margin_var = IntVar(value=0)
        ttk.Spinbox(frm, from_=0, to=10000000, textvariable=self.margin_var, width=8).grid(column=10, row=row, sticky='w')

        row += 1
        btns = ttk.Frame(frm)
        btns.grid(column=0, row=row, columnspan=8, sticky='w', pady=(10,5))
        ttk.Button(btns, text='Build', command=self.on_build).pack(side='left')
        ttk.Button(btns, text='Load results', command=self._on_load_results).pack(side='left', padx=5)
        # Combobox to pick a result by name (populated by Load results)
        self.result_name_var = StringVar()
        self.result_name_cb = ttk.Combobox(btns, textvariable=self.result_name_var, state='readonly', width=40)
        self.result_name_cb.pack(side='left', padx=(8,0))
        self.result_name_cb.bind('<<ComboboxSelected>>', self._on_result_name_selected)
        ttk.Button(btns, text='Run', command=self.on_run).pack(side='left', padx=5)
        ttk.Button(btns, text='Stop', command=self.on_stop).pack(side='left')
        ttk.Button(btns, text='Open dist', command=self.on_open_dist).pack(side='left', padx=5)
        ttk.Button(btns, text='Pick random match', command=self.on_pick_random).pack(side='left', padx=10)
        ttk.Button(btns, text='Choose CSV...', command=self.on_choose_csv).pack(side='left', padx=(6,0))
        # Reservoir size control for Pick random match
        ttk.Label(btns, text='Pick count:').pack(side='left', padx=(8,4))
        ttk.Spinbox(btns, from_=1, to=20, textvariable=self.reservoir_size_var, width=4).pack(side='left')

        # Sampling strategy controls: allow faster approximate sampling when full scan is too slow
        ttk.Label(btns, text='Sampling:').pack(side='left', padx=(12,4))
        self.sampling_strategy_var = StringVar(value='reservoir')
        # strategies: reservoir (full scan + reservoir), per-file (sample K rows per file), random-scan (seek random offsets)
        self.sampling_strategy_cb = ttk.Combobox(btns, textvariable=self.sampling_strategy_var, values=['reservoir','per-file','random-scan'], state='readonly', width=12)
        self.sampling_strategy_cb.pack(side='left')
        ttk.Label(btns, text='Per-file K:').pack(side='left', padx=(8,4))
        self.per_file_k_var = IntVar(value=100)
        ttk.Spinbox(btns, from_=1, to=10000, textvariable=self.per_file_k_var, width=6).pack(side='left')

        # Match selection options
        ttk.Label(btns, text='Match mode:').pack(side='left', padx=(12,4))
        self.match_mode_var = StringVar(value='at-least')
        self.match_mode_cb = ttk.Combobox(btns, textvariable=self.match_mode_var, values=['at-least', 'exact'], state='readonly', width=8)
        self.match_mode_cb.pack(side='left')
        ttk.Label(btns, text='Level:').pack(side='left', padx=(8,4))
        self.match_level_var = IntVar(value=1)
        ttk.Spinbox(btns, from_=0, to=10, textvariable=self.match_level_var, width=4).pack(side='left')
        # internal mapping of names->indices (1-based)
        self._result_names = []
        self._name_to_index = {}

        # Environment configuration section
        row += 1
        env_frm = ttk.LabelFrame(frm, text='Environment', padding=(8,6))
        env_frm.grid(column=0, row=row, columnspan=8, sticky='ew', pady=(8,8))

        # Deck
        ttk.Label(env_frm, text='Deck:').grid(column=0, row=0, sticky='w')
        self.deck_var = StringVar(value='Red Deck')
        self.deck_cb = ttk.Combobox(
            env_frm,
            textvariable=self.deck_var,
            values=[
                'Red Deck', 'Black Deck', 'Painted Deck', 'Ghost Deck', 'Erratic Deck',
                'Green Deck', 'Cruelty Deck', 'Blast Off Deck', 'Five-Card Draw', 'Custom'
            ],
            state='readonly',
            width=24,
        )
        self.deck_cb.grid(column=1, row=0, sticky='w')

        # Stake
        ttk.Label(env_frm, text='Stake:').grid(column=2, row=0, sticky='w', padx=(12,0))
        self.stake_var = StringVar(value='White Stake')
        # Include Red and Green stakes which were previously missing
        self.stake_cb = ttk.Combobox(
            env_frm,
            textvariable=self.stake_var,
            values=[
                'White Stake', 'Black Stake', 'Blue Stake', 'Purple Stake',
                'Orange Stake', 'Red Stake', 'Green Stake', 'Gold Stake'
            ],
            state='readonly',
            width=14,
        )
        self.stake_cb.grid(column=3, row=0, sticky='w')

        # Tags: use a separate dialog with individual selection (like Jokers) rather than a dropdown
        ttk.Label(env_frm, text='Tags:').grid(column=4, row=0, sticky='w', padx=(12,0))
        # Maintain a list of unlocked tag names (strings matching the enum display names)
        self.unlocked_tags = []  # list of strings
        # Small label showing a summary of selected tags
        self.tags_summary_var = StringVar(value='(none)')
        self.tags_summary_lbl = ttk.Label(env_frm, textvariable=self.tags_summary_var)
        self.tags_summary_lbl.grid(column=5, row=0, sticky='w')
        ttk.Button(env_frm, text='Edit tags...', command=self.on_edit_tags).grid(column=6, row=0, sticky='w', padx=(8,0))

        # Showman
        self.showman_var = BooleanVar(value=False)
        ttk.Checkbutton(env_frm, text='Showman', variable=self.showman_var).grid(column=6, row=0, sticky='w', padx=(12,0))

        # Fresh profile / fresh run - these affect deterministic generation
        ttk.Checkbutton(env_frm, text='Fresh profile', variable=self.fresh_profile_var).grid(column=7, row=0, sticky='w', padx=(12,0))
        ttk.Checkbutton(env_frm, text='Fresh run', variable=self.fresh_run_var).grid(column=8, row=0, sticky='w', padx=(12,0))

        # Sixes factor and version
        ttk.Label(env_frm, text='Sixes factor:').grid(column=0, row=1, sticky='w', pady=(6,0))
        self.sixes_var = IntVar(value=1)
        ttk.Spinbox(env_frm, from_=0, to=10, textvariable=self.sixes_var, width=6).grid(column=1, row=1, sticky='w', pady=(6,0))
        ttk.Label(env_frm, text='Version:').grid(column=2, row=1, sticky='w', padx=(12,0), pady=(6,0))
        self.version_var = IntVar(value=10106)
        ttk.Spinbox(env_frm, from_=10000, to=20000, textvariable=self.version_var, width=8).grid(column=3, row=1, sticky='w', pady=(6,0))

        # Selected options (force all content)
        self.force_all_var = BooleanVar(value=True)
        ttk.Checkbutton(env_frm, text='Force all content (selectedOptions=61,true)', variable=self.force_all_var).grid(column=0, row=2, columnspan=4, sticky='w', pady=(6,0))

        # Env actions
        env_actions = ttk.Frame(env_frm)
        env_actions.grid(column=0, row=3, columnspan=8, sticky='w', pady=(8,0))
        ttk.Button(env_actions, text='Save env file', command=self.on_save_env).pack(side='left')
        ttk.Button(env_actions, text='Load env file', command=self.on_load_env).pack(side='left', padx=(8,0))
        ttk.Button(env_actions, text='Edit tags', command=self.on_edit_tags).pack(side='left', padx=(8,0))
        ttk.Button(env_actions, text='Edit selected options', command=self.on_edit_selected_options).pack(side='left', padx=(8,0))
        ttk.Button(env_actions, text='Copy reproduce cmd', command=self.on_copy_reproduce_cmd).pack(side='left', padx=(8,0))
        self.use_latest_env_var = BooleanVar(value=False)
        ttk.Checkbutton(env_actions, text='Use latest env when running', variable=self.use_latest_env_var).pack(side='left', padx=(8,0))

        row += 1
        ttk.Label(frm, text='Build output:').grid(column=0, row=row, columnspan=5, sticky='w')
        row += 1
        # Create scrolled text widgets as children of the frame to avoid pack/grid conflicts on root
        self.build_output = scrolledtext.ScrolledText(frm, height=8, wrap='word')
        self.build_output.grid(column=0, row=row, columnspan=5, sticky='nsew')

        row += 1
        ttk.Label(frm, text='Run output:').grid(column=0, row=row, columnspan=5, sticky='w', pady=(10,0))
        row += 1
        self.run_output = scrolledtext.ScrolledText(frm, height=18, wrap='word')
        self.run_output.grid(column=0, row=row, columnspan=5, sticky='nsew')

        frm.columnconfigure(1, weight=1)
        frm.rowconfigure(row, weight=1)

    def _load_filters(self):
        opts = list_filters()
        self.filter_cb['values'] = opts
        if opts and not self.filter_var.get():
            self.filter_var.set(opts[0])

    # Canonical human-friendly tag names (matches Items::Tag enum display names)
    TAG_NAMES = [
        'Uncommon Tag', 'Rare Tag', 'Negative Tag', 'Foil Tag', 'Holographic Tag', 'Polychrome Tag',
        'Investment Tag', 'Voucher Tag', 'Boss Tag', 'Standard Tag', 'Charm Tag', 'Meteor Tag', 'Buffoon Tag',
        'Handy Tag', 'Garbage Tag', 'Ethereal Tag', 'Coupon Tag', 'Double Tag', 'Juggle Tag', 'D6 Tag',
        'Topup Tag', 'Speed Tag', 'Orbital Tag', 'Economy Tag'
    ]

    def _update_tags_summary(self):
        if not self.unlocked_tags:
            self.tags_summary_var.set('(none)')
            return
        # show up to 3 tags then count
        if len(self.unlocked_tags) <= 3:
            self.tags_summary_var.set(', '.join(self.unlocked_tags))
        else:
            self.tags_summary_var.set(', '.join(self.unlocked_tags[:3]) + f' (+{len(self.unlocked_tags)-3} more)')

    def on_edit_tags(self):
        # Dialog to toggle individual tags (checkbox list)
        try:
            from tkinter import Toplevel, Label, Button, Checkbutton
        except Exception:
            messagebox.showerror('Error', 'Tkinter components not available')
            return

        top = Toplevel(self.root)
        top.title('Edit unlocked Tags')

        frm = ttk.Frame(top, padding=8)
        frm.pack(fill='both', expand=True)

        header = Label(frm, text='Toggle individual Tags. These represent unlocked tags available to the seed generator.')
        header.pack(anchor='w')

        # For a reasonable number of tags create checkboxes directly
        vars = []
        for name in self.TAG_NAMES:
            v = BooleanVar(value=(name in self.unlocked_tags))
            vars.append((name, v))

        # lay out vertically
        for name, v in vars:
            cb = ttk.Checkbutton(frm, text=name, variable=v)
            cb.pack(anchor='w', padx=6, pady=2)

        def do_save():
            sel = [name for name, v in vars if v.get()]
            self.unlocked_tags = sel
            self._update_tags_summary()
            bytes_base = 0
            rows_read = 0
            messagebox.showinfo('Saved', 'Unlocked tags updated in the GUI (will be included when saving env file)')
            top.destroy()

        btns = ttk.Frame(frm)
        btns.pack(fill='x', pady=(8,0))
        ttk.Button(btns, text='Save', command=do_save).pack(side='right')
        ttk.Button(btns, text='Cancel', command=top.destroy).pack(side='right', padx=(0,8))

    def append_build(self, text: str):
        self.build_output.insert('end', text)
        self.build_output.see('end')
        self.run_output.see('end')

    def append_run(self, text: str):
        """Append text to the run output widget and keep views scrolled to the end.

        This was missing which caused "'App' object has no attribute 'append_run'" when
        process output was polled.
        """
        try:
            if self.run_output:
                self.run_output.insert('end', text)
                self.run_output.see('end')
            if self.build_output:
                # keep build output scrolled as well to avoid odd view state
                self.build_output.see('end')
        except Exception:
            # swallow UI errors during background polling
            pass

    def poll_outputs(self):
        # Update UI with any process output
        if self.process_runner:
            out = self.process_runner.read_available()
            if out:
                self.append_run(out)
                # Detect match name printed by debug mode: 'Match Name: <name>\n'
                for line in out.splitlines():
                    if line.startswith('Match Name:'):
                        name = line[len('Match Name:'):].strip()
                        try:
                            self.match_name_var.set(name)
                        except Exception:
                            pass
                    # New JSON describe-match output: {"index": N, "name": "..."}
                    if line.strip().startswith('{') and '"index"' in line and '"name"' in line:
                        try:
                            j = json.loads(line.strip())
                            if isinstance(j, dict):
                                idx = int(j.get('index', 0))
                                name = j.get('name', '') or ''
                                # set match_name_var for display
                                try:
                                    self.match_name_var.set(name)
                                except Exception:
                                    pass
                                # if name maps to an index, update Level and combobox selection
                                if idx > 0 and 1 <= idx <= len(self._result_names):
                                    try:
                                        self.match_level_var.set(idx)
                                    except Exception:
                                        pass
                                if name and name in self._name_to_index:
                                    try:
                                        self.result_name_var.set(name)
                                    except Exception:
                                        pass
                        except Exception:
                            pass
                # Detect older executables that don't support newer flags (e.g. --resume)
                if (not self._missing_option_prompted) and ('unknown option' in out.lower() or 'unrecognized option' in out.lower()):
                    self._missing_option_prompted = True
                    # Offer to build the executable which likely contains the updated CLI
                    resp = messagebox.askyesno('Executable missing option', 'The executable reported an unknown option (for example --resume).\nWould you like to build the selected filter now to get the updated executable?')
                    if resp:
                        try:
                            self.on_build()
                        except Exception as e:
                            messagebox.showerror('Build Error', str(e))
        self.root.after(120, self.poll_outputs)

    def on_build(self):
        filt = self.filter_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        self.build_output.delete('1.0', 'end')
        script = os.path.join(TOOLS_DIR, 'build.bat') if os.name == 'nt' else os.path.join(TOOLS_DIR, 'build.sh')
        if not os.path.exists(script):
            messagebox.showerror('Error', f'Build script not found: {script}')
            return

        if os.name == 'nt':
            cmd = ['cmd', '/c', script, filt]
        else:
            cmd = ['bash', script, filt]

        try:
            self.append_build(f'Running: {" ".join(cmd)}\n')
            res = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True)
            if res.stdout:
                self.append_build(res.stdout)
            if res.stderr:
                self.append_build(res.stderr)
            if res.returncode == 0:
                self.append_build('Build completed successfully.\n')
            else:
                self.append_build(f'Build failed with code {res.returncode}.\n')
        except Exception as e:
            messagebox.showerror('Build Error', str(e))

    def on_run(self):
        filt = self.filter_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        exe = os.path.join(DIST_DIR, f'immolate_{filt}.exe' if os.name == 'nt' else f'immolate_{filt}')
        if not os.path.exists(exe):
            if not messagebox.askyesno('Executable not found', 'Executable not found. Build now?'):
                return
            self.on_build()
            if not os.path.exists(exe):
                messagebox.showerror('Error', f'Executable still not found: {exe}')
                return

        args = [exe]
        seed = self.seed_var.get().strip()
        threads = int(self.threads_var.get())
        debug = bool(self.debug_var.get())

        if seed:
            args += ['--seed', seed]
        # Resume option: don't send --resume in debug mode; otherwise include when requested
        if self.resume_var.get() and not debug:
            args += ['--resume']
            try:
                off = int(self.offset_var.get())
                if off > 0:
                    args += ['--resume-offset', str(off)]
            except Exception:
                pass
            try:
                mar = int(self.margin_var.get())
                if mar > 0:
                    args += ['--resume-margin', str(mar)]
            except Exception:
                pass
        if threads > 0 and not debug:
            args += ['--threads', str(threads)]
        # If requested, find latest env file for this filter and pass --env
        if self.use_latest_env_var.get():
            import glob
            pattern = os.path.join(DIST_DIR, f'env_{filt}_*.json')
            files = glob.glob(pattern)
            if files:
                latest = sorted(files)[-1]
                args += ['--env', latest]
        else:
            # Prefer last-loaded env file for this session (convenience)
            try:
                if getattr(self, 'loaded_env_path', None):
                    args += ['--env', self.loaded_env_path]
            except Exception:
                pass
        if debug:
            if not seed:
                messagebox.showerror('Error', 'Debug mode requires a seed')
                return
            # If possible, query the executable for a compact JSON description of the match
            try:
                j = self._describe_match_for_seed(seed, filt)
                if isinstance(j, dict):
                    idx = int(j.get('index', 0))
                    name = j.get('name', '') or ''
                    try:
                        self.match_name_var.set(name)
                    except Exception:
                        pass
                    if idx > 0:
                        try:
                            self.match_level_var.set(idx)
                        except Exception:
                            pass
                    if name and name in self._name_to_index:
                        try:
                            self.result_name_var.set(name)
                        except Exception:
                            pass
            except Exception:
                pass
            args += ['--debug']

        self.run_output.delete('1.0', 'end')
        try:
            self.process_runner.start(args, cwd=REPO_ROOT)
            self.append_run('Process started...\n')
        except Exception as e:
            messagebox.showerror('Run Error', str(e))

    def _progress_file_for_filter(self, filter_name):
        # Derive filter key similar to immolate: use getName(); fall back to simple filter_name
        key = filter_name.replace(' ', '_')
        return os.path.join(DIST_DIR, f'progress_{key}.txt')

    def number_to_seed(self, number: int) -> str:
        chars = "ABCDEFGHIJKLMNPQRSTUVWXYZ123456789"
        base = len(chars)
        if number == 0:
            return chars[0] * 8
        result = ''
        temp = int(number)
        while temp > 0:
            result = chars[temp % base] + result
            temp //= base
        # pad to 8
        while len(result) < 8:
            result = chars[0] + result
        return result

    def on_refresh_progress(self):
        filt = self.filter_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        prog = self._progress_file_for_filter(filt)
        if not os.path.exists(prog):
            self.append_run(f'No progress file found for filter: {prog}\n')
            return
        try:
            with open(prog, 'r', encoding='utf-8') as pf:
                val = pf.read().strip()
                if val:
                    seedstr = None
                    try:
                        # show as number and seed string if possible
                        num = int(val)
                        seed = self.number_to_seed(num)
                        seedstr = f"Numeric: {num} -> Seed: {seed}"
                    except Exception:
                        seedstr = val
                    self.append_run(f'Progress file {prog}: {seedstr}\n')
        except Exception as e:
            self.append_run(f'Error reading progress file: {e}\n')
        
    def on_save_env(self):
        # Compose environment dict and write JSON to dist/env_<filter>_<timestamp>.json
        filt = self.filter_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        env = {
            'deck': self.deck_var.get(),
            'stake': self.stake_var.get(),
            # Persist unlocked tags as a list of human-friendly names for reproducibility
            'unlockedTags': list(self.unlocked_tags) if self.unlocked_tags else [],
            'showman': bool(self.showman_var.get()),
            'freshProfile': bool(self.fresh_profile_var.get()),
            'freshRun': bool(self.fresh_run_var.get()),
            'sixesFactor': int(self.sixes_var.get()),
            'version': int(self.version_var.get()),
            'forceAllContent': bool(self.force_all_var.get())
        }
        # selectedOptions: include explicit list when user edited them.
        # If the user didn't edit options but "Force all content" is checked,
        # write an explicit all-true selectedOptions array for reproducibility.
        if self.selected_options_list is not None:
            env['selectedOptions'] = self.selected_options_list
        else:
            # If forceAllContent is True, explicitly write all-true so the env file
            # unambiguously reproduces "all content enabled" behavior.
            if self.force_all_var.get():
                env['selectedOptions'] = [True] * 61
        os.makedirs(DIST_DIR, exist_ok=True)
        ts = datetime.datetime.utcnow().strftime('%Y%m%d_%H%M%S')
        fname = os.path.join(DIST_DIR, f'env_{filt}_{ts}.json')
        # For compatibility with older backends, if exactly one unlocked tag is present
        # write a legacy 'tag' string alongside 'unlockedTags'. Newer backends prefer
        # 'unlockedTags', but some filters still read 'tag'.
        if env.get('unlockedTags') and len(env.get('unlockedTags')) == 1:
            try:
                env['tag'] = env['unlockedTags'][0]
            except Exception:
                pass

        try:
            with open(fname, 'w', encoding='utf-8') as fh:
                json.dump(env, fh, indent=2)
            self.append_run(f'Wrote env file: {fname}\n')
            messagebox.showinfo('Env Saved', f'Wrote environment to: {fname}')
        except Exception as e:
            messagebox.showerror('Error', f'Could not write env file: {e}')

    def on_load_env(self):
        # Let user pick an env JSON and populate GUI fields
        path = filedialog.askopenfilename(title='Open env JSON', filetypes=[('JSON files','*.json'),('All files','*.*')], initialdir=DIST_DIR)
        if not path:
            return
        try:
            with open(path, 'r', encoding='utf-8') as fh:
                data = json.load(fh)
        except Exception as e:
            messagebox.showerror('Error', f'Could not read JSON: {e}')
            return
        # Populate known fields
        try:
            if 'deck' in data:
                self.deck_var.set(data.get('deck', ''))
            if 'stake' in data:
                self.stake_var.set(data.get('stake', ''))
            # Load unlockedTags if present (list of names). Populate unlocked_tags and update summary
            if 'unlockedTags' in data:
                try:
                    tags = data.get('unlockedTags') or []
                    if isinstance(tags, list) and all(isinstance(t, str) for t in tags):
                        self.unlocked_tags = tags
                        self._update_tags_summary()
                except Exception:
                    pass
            if 'showman' in data:
                self.showman_var.set(bool(data.get('showman', False)))
            if 'freshProfile' in data:
                try:
                    self.fresh_profile_var.set(bool(data.get('freshProfile', False)))
                except Exception:
                    pass
            if 'freshRun' in data:
                try:
                    self.fresh_run_var.set(bool(data.get('freshRun', False)))
                except Exception:
                    pass
            if 'sixesFactor' in data:
                self.sixes_var.set(int(data.get('sixesFactor', 1)))
            if 'version' in data:
                self.version_var.set(int(data.get('version', 10106)))
            if 'forceAllContent' in data:
                self.force_all_var.set(bool(data.get('forceAllContent', True)))
            if 'selectedOptions' in data:
                sel = data.get('selectedOptions')
                # selectedOptions may be a list of booleans (any length) or a list of integer indices
                if isinstance(sel, list):
                    if all(isinstance(x, bool) for x in sel):
                        # Accept boolean arrays of any length (will be adapted when editing)
                        self.selected_options_list = sel
                    elif all(isinstance(x, int) for x in sel):
                        # Accept list of indices (may include indices beyond 0..60)
                        self.selected_options_list = sel
                    else:
                        messagebox.showwarning('Warning', 'selectedOptions found but invalid format; ignored')
                # Accept legacy single 'tag' field for compatibility
                if 'tag' in data and (not data.get('unlockedTags')):
                    try:
                        t = data.get('tag')
                        if isinstance(t, str) and t:
                            self.unlocked_tags = [t]
                            self._update_tags_summary()
                    except Exception:
                        pass

                # Remember which env we loaded so we can prefer reusing it when running
                try:
                    self.loaded_env_path = path
                except Exception:
                    pass

                self.append_run(f'Loaded env file: {path}\n')
        except Exception as e:
            messagebox.showerror('Error applying env', str(e))

    def on_edit_selected_options(self):
        # Open a checkbox grid allowing users to toggle the 61 selectedOptions
        try:
            from tkinter import Toplevel, Label, Button, Checkbutton
        except Exception:
            messagebox.showerror('Error', 'Tkinter components not available')
            return

        top = Toplevel(self.root)
        top.title('Edit selectedOptions')
        # size adapts to content

        # Try to load human-readable labels (full list). We'll ask user whether to edit
        # the default common range (0..60) or all Joker indices (0..149).
        labels_map = self._load_selected_option_labels()
        if not labels_map:
            messagebox.showerror('Error', 'Could not load labels from items_to_string.hpp or Docs/Jokers.md')
            return
        # labels_map is a tuple (labels_display, index_map) where index_map maps display idx -> enum idx
        labels, index_map = labels_map

        # Ask whether to edit full range or default common range
        resp = messagebox.askyesno('Edit range', 'Edit ALL Joker indices (0..N)?\nYes = All Jokers, No = Common (0..60)')
        full_mode = bool(resp)
        if full_mode:
            total = len(labels)
        else:
            total = 61

        # Current selection: support three cases for self.selected_options_list:
        # - None -> default (all True if force_all_var else all True)
        # - list of bools -> use that (if shorter/padder, adapt)
        # - list of ints -> convert to bool array
        if self.selected_options_list is None:
            current = [True] * total
        else:
            # If stored as boolean array per enum index, adapt to display mapping
            if all(isinstance(x, bool) for x in self.selected_options_list):
                src = list(self.selected_options_list)
                # src indexes correspond to enum indices; we must map to display order
                # build an array of length total using index_map
                arr = [False] * total
                for d_idx, enum_idx in enumerate(index_map if full_mode else range(total)):
                    if enum_idx < len(src):
                        arr[d_idx] = bool(src[enum_idx])
                current = arr
            elif all(isinstance(x, int) for x in self.selected_options_list):
                # stored as list of enum indices -> create bool array for display
                sel_indices = set(self.selected_options_list)
                arr = [False] * total
                for d_idx, enum_idx in enumerate(index_map if full_mode else range(total)):
                    if enum_idx in sel_indices:
                        arr[d_idx] = True
                current = arr
            else:
                # unknown format; default
                current = [True] * total

        # Header shows the index range for the current edit mode
        header_lbl = Label(top, text=f'Toggle individual options (indices 0..{total-1}). Presets: All On / All Off / Invert')
        header_lbl.pack(anchor='w', padx=8, pady=(8,0))

        # For many items create a scrollable canvas with inner frame
        canvas = ttk.Frame(top)
        canvas.pack(fill='both', expand=True, padx=8, pady=6)
        # create a canvas + vertical scrollbar
        from tkinter import Canvas, Scrollbar
        c = Canvas(canvas, borderwidth=0, highlightthickness=0)
        vsb = Scrollbar(canvas, orient='vertical', command=c.yview)
        c.configure(yscrollcommand=vsb.set)
        vsb.pack(side='right', fill='y')
        c.pack(side='left', fill='both', expand=True)
        inner = ttk.Frame(c)

        # add inner frame to canvas
        c.create_window((0, 0), window=inner, anchor='nw')

        vars = []
        cols = 4 if total > 80 else 8
        for i in range(total):
            v = BooleanVar(value=bool(current[i]))
            vars.append(v)
            lab = labels[i] if i < len(labels) else str(i)
            lab_display = lab if len(lab) <= 36 else lab[:33] + '...'
            cb = ttk.Checkbutton(inner, text=f"{i}: {lab_display}", variable=v)
            r = i // cols
            cidx = i % cols
            cb.grid(row=r, column=cidx, sticky='w', padx=4, pady=2)

        # update scrollregion after layout
        inner.update_idletasks()
        c.config(scrollregion=c.bbox('all'))

        def set_all(on=True):
            for v in vars:
                v.set(bool(on))

        def invert_all():
            for v in vars:
                v.set(not v.get())

        def do_save():
            arr = [bool(v.get()) for v in vars]
            if full_mode:
                # Convert boolean array to list of enum indices for storage (immolate accepts indices form)
                idxs = [index_map[i] for i, val in enumerate(arr) if val]
                self.selected_options_list = idxs
            else:
                # keep boolean array length 61
                if len(arr) != 61:
                    messagebox.showerror('Invalid', f'Internal error: expected 61 options, got {len(arr)}')
                    return
                self.selected_options_list = arr
            messagebox.showinfo('Saved', 'selectedOptions updated in the GUI (will be included when saving env file)')
            top.destroy()

        btn_frm = ttk.Frame(top)
        btn_frm.pack(fill='x', padx=8, pady=8)
        ttk.Button(btn_frm, text='All On', command=lambda: set_all(True)).pack(side='left')
        ttk.Button(btn_frm, text='All Off', command=lambda: set_all(False)).pack(side='left', padx=6)
        ttk.Button(btn_frm, text='Invert', command=invert_all).pack(side='left', padx=6)
        ttk.Button(btn_frm, text='Save', command=do_save).pack(side='right')
        ttk.Button(btn_frm, text='Cancel', command=top.destroy).pack(side='right', padx=(0,8))

    def on_copy_reproduce_cmd(self):
        filt = self.filter_var.get().strip()
        seed = self.seed_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        exe = os.path.join(DIST_DIR, f'immolate_{filt}.exe' if os.name == 'nt' else f'immolate_{filt}')
        cmd = f'"{exe}"'
        if seed:
            cmd += f' --seed {seed} --debug'
        # Attach env file if exists: pick latest env_{filter}_*.json
        import glob
        pattern = os.path.join(DIST_DIR, f'env_{filt}_*.json')
        files = glob.glob(pattern)
        envfile = files[-1] if files else None
        if envfile:
            cmd += f'  # env: {envfile}'

        try:
            self.root.clipboard_clear()
            self.root.clipboard_append(cmd)
            messagebox.showinfo('Copied', f'Reproduce command copied to clipboard:\n{cmd}')
        except Exception:
            messagebox.showinfo('Reproduce command', cmd)

    def _load_selected_option_labels(self):
        """Attempt to parse C++ header `items_to_string.hpp` to extract human-readable names for
        the first 61 options (Joker common names / voucher/tag mix). If parsing fails, return None.
        This is a best-effort convenience for the GUI only.
        """
        try:
            path = os.path.join(REPO_ROOT, 'items_to_string.hpp')
            if not os.path.exists(path):
                return None
            with open(path, 'r', encoding='utf-8') as fh:
                text = fh.read()
        except Exception:
            return None

        # Very small parser: look for JOKER_NAMES array and extract all entries if present.
        import re
        jnames = []
        m = re.search(r"JOKER_NAMES\s*=\s*\{([\s\S]*?)\};", text)
        if m:
            body = m.group(1)
            # Find all string literals
            jnames = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', body)

        # If we have a full JOKER_NAMES list, attempt to also build an index_map matching enum indices
        # by reading items.hpp and mapping enum order -> name positions. If items.hpp is not present,
        # we simply return the names as-is with an identity map.
        try:
            ih_path = os.path.join(REPO_ROOT, 'items.hpp')
            if os.path.exists(ih_path):
                with open(ih_path, 'r', encoding='utf-8') as fh:
                    ih_text = fh.read()
            else:
                ih_text = ''
        except Exception:
            ih_text = ''

        enum_tokens = []
        if ih_text:
            m2 = re.search(r'enum class Joker[\s\S]*?COUNT\s*=\s*\d+\s*,[\s\S]*?\};', ih_text)
            if not m2:
                m2 = re.search(r'enum class Joker[\s\S]*?COUNT[\s\S]*?\};', ih_text)
            if m2:
                blk = m2.group(0)
                for line in blk.splitlines():
                    line = line.strip()
                    if not line or line.startswith('//'):
                        continue
                    # skip marker names
                    if re.match(r'(COMMON_START|COMMON_END|UNCOMMON_START|UNCOMMON_END|RARE_START|RARE_END|LEGENDARY_START|LEGENDARY_END|COUNT|INVALID)\b', line):
                        continue
                    m3 = re.match(r'([A-Z0-9_]+)\s*(=\s*[^,]+)?\s*,?$', line)
                    if m3:
                        enum_tokens.append(m3.group(1))

        # If we have enum_tokens and jnames, try to parse Docs/Jokers.md to get canonical doc order
        if enum_tokens and jnames:
            import unicodedata
            def norm(s):
                s2 = unicodedata.normalize('NFKD', s)
                s2 = s2.lower()
                s2 = re.sub(r"[^a-z0-9]+", '', s2)
                return s2

            name_map = {norm(n): i for i,n in enumerate(jnames)}

            # attempt to parse Docs/Jokers.md for the 'List of Jokers' ordered list
            docs_path = os.path.join(REPO_ROOT, 'Docs', 'Jokers.md')
            doc_order = []
            try:
                if os.path.exists(docs_path):
                    with open(docs_path, 'r', encoding='utf-8') as fh:
                        doctxt = fh.read()
                    # split into lines and look for numeric index lines followed by a name line
                    lines = [ln.rstrip() for ln in doctxt.splitlines()]
                    i = 0
                    while i < len(lines):
                        line = lines[i].strip()
                        if line.isdigit():
                            # find next non-empty line as name
                            j = i+1
                            while j < len(lines) and not lines[j].strip():
                                j += 1
                            if j < len(lines):
                                name = lines[j].strip()
                                # filter out lines that look like section headings
                                if len(name) > 0 and len(name.split()) < 10:
                                    doc_order.append(name)
                                i = j+1
                                continue
                        i += 1
            except Exception:
                doc_order = []

            # If we found a doc_order with many entries, build labels from it mapping back to enum indices
            if len(doc_order) >= 100:
                labels_display = []
                index_map = []
                for dn in doc_order:
                    k = norm(dn)
                    if k in name_map:
                        enum_idx = name_map[k]
                        labels_display.append(jnames[enum_idx])
                        index_map.append(enum_idx)
                    else:
                        # try minor variants
                        alt = dn.replace('Seance','Séance')
                        if norm(alt) in name_map:
                            enum_idx = name_map[norm(alt)]
                            labels_display.append(jnames[enum_idx])
                            index_map.append(enum_idx)
                        else:
                            # fallback: try find closest by substring
                            found = None
                            for k2,v in name_map.items():
                                if norm(dn) in k2 or k2 in norm(dn):
                                    found = v
                                    break
                            if found is not None:
                                labels_display.append(jnames[found])
                                index_map.append(found)
                            else:
                                labels_display.append(dn)
                                index_map.append(len(jnames))
                return (labels_display, index_map)

            # fallback: produce a Docs-like ordering by scanning jnames for headings/natural ordering
            # If doc_order not available, simply return jnames with identity mapping
            return (jnames, list(range(len(jnames))))

        # Fallback behavior: if we only have jnames, return them with identity map
        if jnames:
            return (jnames, list(range(len(jnames))))

        # Final fallback: try VOUCHER_NAMES / TAG_NAMES like before
        labels = []
        m2 = re.search(r"VOUCHER_NAMES\s*=\s*\{([\s\S]*?)\};", text)
        if m2:
            strs2 = re.findall(r'"([^"\\]*)"', m2.group(1))
            labels.extend(strs2)

        m3 = re.search(r"TAG_NAMES\s*=\s*\{([\s\S]*?)\};", text)
        if m3:
            strs3 = re.findall(r'"([^"\\]*)"', m3.group(1))
            labels.extend(strs3)

        if not labels:
            return None
        if len(labels) < 61:
            for i in range(len(labels), 61):
                labels.append(f'Option {i}')
        return (labels, list(range(len(labels))))

    def on_pick_random(self):
        # Efficiently pick a random matching seed from large CSVs without loading all into memory.
        # This runs in a background thread and shows a cancellable progress dialog.
        import glob
        import csv
        import random
        import threading
        import itertools

        pattern = os.path.join(DIST_DIR, 'matches_*.csv')
        files = glob.glob(pattern)
        chosen_csv = getattr(self, 'chosen_csv', None)
        if chosen_csv:
            files = [chosen_csv]

        if not files:
            messagebox.showinfo('No matches', 'No matching seeds found in dist/*.csv or chosen CSV')
            return

        mode = self.match_mode_var.get() if hasattr(self, 'match_mode_var') else 'at-least'
        level = int(self.match_level_var.get()) if hasattr(self, 'match_level_var') else 1
        selected_name = self.result_name_var.get() if hasattr(self, 'result_name_var') else ''

        # Prepare progress dialog
        total_bytes = 0
        file_sizes = []
        for f in files:
            try:
                sz = os.path.getsize(f)
            except Exception:
                sz = 0
            file_sizes.append(sz)
            total_bytes += sz

        progress_top = None
        progress_bar = None
        progress_lbl = None

        q = queue.Queue()
        stop_event = threading.Event()

        def worker():
            strategy = self.sampling_strategy_var.get() if hasattr(self, 'sampling_strategy_var') else 'reservoir'
            try:
                RESERVOIR_SIZE = int(self.reservoir_size_var.get())
            except Exception:
                RESERVOIR_SIZE = 6

            # Helper: check a CSV iterator and feed reservoir-like sampling
            def process_rows_iterator(rows_iter, reservoir, count_info):
                rows_local = 0
                for parts in rows_iter:
                    if stop_event.is_set():
                        return False
                    if not parts:
                        continue
                    rows_local += 1
                    seed = parts[0].strip().strip('"') if len(parts) >= 1 else ''
                    lvl = 1
                    name_in_row = ''
                    if len(parts) >= 2:
                        try:
                            lvl = int(parts[1])
                        except Exception:
                            lvl = 1
                    if len(parts) >= 3:
                        name_in_row = parts[2].strip()
                        if name_in_row.startswith('"') and name_in_row.endswith('"'):
                            name_in_row = name_in_row[1:-1]

                    ok = False
                    if selected_name:
                        if name_in_row:
                            ok = (name_in_row == selected_name)
                        else:
                            if lvl > 0 and lvl <= len(self._result_names):
                                ok = (self._result_names[lvl-1] == selected_name)
                    else:
                        if mode == 'at-least':
                            ok = (lvl >= level)
                        else:
                            ok = (lvl == level)

                    if ok:
                        count_info[0] += 1
                        # reservoir sampling insertion
                        if len(reservoir) < RESERVOIR_SIZE:
                            reservoir.append(seed)
                        else:
                            r = random.randrange(count_info[0])
                            if r < RESERVOIR_SIZE:
                                reservoir[r] = seed
                    # occasionally report progress
                    if count_info[1] % 2000 == 0:
                        try:
                            pos = fh.tell()
                        except Exception:
                            pos = 0
                        bytes_done_loc = count_info[2] + pos
                        q.put({'progress': bytes_done_loc / max(1, total_bytes), 'count': count_info[0], 'seeds': list(reservoir)})
                    count_info[1] += 1
                return True

            # Strategy: reservoir (full scan), per-file (sample K rows per file), random-scan (seek random offsets)
            if strategy == 'per-file':
                # For each file, pick up to K random rows by reading file once and keeping reservoir of size K for that file
                per_k = max(1, int(self.per_file_k_var.get()))
                global_reservoir = []
                total_count = 0
                bytes_base = 0
                for idx, f in enumerate(files):
                    if stop_event.is_set():
                        q.put({'canceled': True, 'seeds': list(global_reservoir)})
                        return
                    try:
                        local_res = []
                        local_count_info = [0, 0, 0]
                        with open(f, 'r', encoding='utf-8', newline='') as fh:
                            reader = csv.reader(fh)
                            try:
                                first = next(reader)
                            except StopIteration:
                                bytes_base += file_sizes[idx]
                                q.put({'progress': bytes_base / max(1, total_bytes), 'count': total_count})
                                continue
                            has_header = False
                            if len(first) >= 2:
                                a0 = str(first[0]).lower()
                                a1 = str(first[1]).lower()
                                if 'seed' in a0 or 'match' in a1 or 'level' in a1:
                                    has_header = True
                            rows_iter = reader if has_header else itertools.chain([first], reader)
                            # per-file reservoir of size per_k
                            for parts in rows_iter:
                                if stop_event.is_set():
                                    q.put({'canceled': True, 'seeds': list(global_reservoir)})
                                    return
                                if not parts:
                                    continue
                                local_count_info[1] += 1
                                seed = parts[0].strip().strip('"') if len(parts) >= 1 else ''
                                lvl = 1
                                name_in_row = ''
                                if len(parts) >= 2:
                                    try:
                                        lvl = int(parts[1])
                                    except Exception:
                                        lvl = 1
                                if len(parts) >= 3:
                                    name_in_row = parts[2].strip()
                                    if name_in_row.startswith('"') and name_in_row.endswith('"'):
                                        name_in_row = name_in_row[1:-1]
                                ok = False
                                if selected_name:
                                    if name_in_row:
                                        ok = (name_in_row == selected_name)
                                    else:
                                        if lvl > 0 and lvl <= len(self._result_names):
                                            ok = (self._result_names[lvl-1] == selected_name)
                                else:
                                    if mode == 'at-least':
                                        ok = (lvl >= level)
                                    else:
                                        ok = (lvl == level)
                                if ok:
                                    local_count_info[0] += 1
                                    if len(local_res) < per_k:
                                        local_res.append(seed)
                                    else:
                                        r = random.randrange(local_count_info[0])
                                        if r < per_k:
                                            local_res[r] = seed
                                if local_count_info[1] % 2000 == 0:
                                    pos = fh.tell() if hasattr(fh, 'tell') else 0
                                    q.put({'progress': bytes_base + pos / max(1, total_bytes), 'count': total_count + local_count_info[0], 'seeds': list(global_reservoir)})
                        # merge local_res into global_reservoir using reservoir sampling
                        for s in local_res:
                            total_count += 1
                            if len(global_reservoir) < RESERVOIR_SIZE:
                                global_reservoir.append(s)
                            else:
                                r = random.randrange(total_count)
                                if r < RESERVOIR_SIZE:
                                    global_reservoir[r] = s
                    except Exception:
                        # skip file
                        bytes_base += file_sizes[idx]
                        q.put({'progress': bytes_base / max(1, total_bytes), 'count': total_count})
                        continue
                    bytes_base += file_sizes[idx]
                q.put({'progress': 1.0, 'count': total_count, 'seeds': list(global_reservoir), 'done': True})
                return

            if strategy == 'random-scan':
                # Heuristic: for each file, seek to a few random byte offsets and read the next full line to try to find matching seeds.
                # This avoids scanning whole file but is approximate and may miss rare matches.
                samples_per_file = max(4, int(self.per_file_k_var.get() // 10))
                global_res = []
                total_count = 0
                bytes_base = 0
                for idx, f in enumerate(files):
                    if stop_event.is_set():
                        q.put({'canceled': True, 'seeds': list(global_res)})
                        return
                    try:
                        sz = file_sizes[idx]
                        if sz < 1024:
                            # tiny file: just scan it
                            with open(f, 'r', encoding='utf-8', newline='') as fh:
                                reader = csv.reader(fh)
                                try:
                                    first = next(reader)
                                except StopIteration:
                                    bytes_base += sz
                                    continue
                                rows_iter = reader
                                # fallback to normal per-file reservoir of small size
                                local_res = []
                                local_count = 0
                                for parts in rows_iter:
                                    if stop_event.is_set():
                                        q.put({'canceled': True, 'seeds': list(global_res)})
                                        return
                                    seed = parts[0].strip().strip('"') if len(parts) >= 1 else ''
                                    lvl = 1
                                    if len(parts) >= 2:
                                        try:
                                            lvl = int(parts[1])
                                        except Exception:
                                            lvl = 1
                                    ok = (lvl >= level) if mode == 'at-least' else (lvl == level)
                                    if selected_name:
                                        name_in_row = parts[2].strip() if len(parts) >= 3 else ''
                                        if name_in_row:
                                            ok = (name_in_row == selected_name)
                                    if ok:
                                        local_count += 1
                                        if len(local_res) < samples_per_file:
                                            local_res.append(seed)
                                        else:
                                            r = random.randrange(local_count)
                                            if r < samples_per_file:
                                                local_res[r] = seed
                                for s in local_res:
                                    total_count += 1
                                    if len(global_res) < RESERVOIR_SIZE:
                                        global_res.append(s)
                                    else:
                                        r = random.randrange(total_count)
                                        if r < RESERVOIR_SIZE:
                                            global_res[r] = s
                        else:
                            # choose random offsets and read next full line
                            import io
                            local_candidates = []
                            with open(f, 'rb') as fh:
                                for _ in range(samples_per_file):
                                    if stop_event.is_set():
                                        q.put({'canceled': True, 'seeds': list(global_res)})
                                        return
                                    off = random.randrange(sz)
                                    try:
                                        fh.seek(off)
                                    except Exception:
                                        fh.seek(0)
                                    # skip partial line
                                    fh.readline()
                                    line = fh.readline()
                                    if not line:
                                        fh.seek(0)
                                        fh.readline()
                                        line = fh.readline()
                                    try:
                                        line_s = line.decode('utf-8', errors='ignore').strip()
                                    except Exception:
                                        line_s = ''
                                    if not line_s:
                                        continue
                                    # parse CSV row from line
                                    try:
                                        parts = next(csv.reader([line_s]))
                                    except Exception:
                                        parts = [line_s]
                                    seed = parts[0].strip().strip('"') if len(parts) >= 1 else ''
                                    lvl = 1
                                    if len(parts) >= 2:
                                        try:
                                            lvl = int(parts[1])
                                        except Exception:
                                            lvl = 1
                                    name_in_row = parts[2].strip() if len(parts) >= 3 else ''
                                    ok = False
                                    if selected_name:
                                        if name_in_row:
                                            ok = (name_in_row == selected_name)
                                        else:
                                            if lvl > 0 and lvl <= len(self._result_names):
                                                ok = (self._result_names[lvl-1] == selected_name)
                                    else:
                                        ok = (lvl >= level) if mode == 'at-least' else (lvl == level)
                                    if ok:
                                        local_candidates.append(seed)
                            # merge local candidates into global reservoir
                            for s in local_candidates:
                                total_count += 1
                                if len(global_res) < RESERVOIR_SIZE:
                                    global_res.append(s)
                                else:
                                    r = random.randrange(total_count)
                                    if r < RESERVOIR_SIZE:
                                        global_res[r] = s
                    except Exception:
                        pass
                    bytes_base += file_sizes[idx]
                q.put({'progress': 1.0, 'count': total_count, 'seeds': list(global_res), 'done': True})
                return

            # Default: full reservoir scan across all files (existing behavior)
            reservoir = []
            count = 0
            bytes_done = 0
            bytes_base = 0
            rows_read = 0
            for idx, f in enumerate(files):
                if stop_event.is_set():
                    q.put({'canceled': True, 'seeds': list(reservoir)})
                    return
                try:
                    with open(f, 'r', encoding='utf-8', newline='') as fh:
                        reader = csv.reader(fh)
                        try:
                            first = next(reader)
                        except StopIteration:
                            bytes_done += file_sizes[idx]
                            q.put({'progress': bytes_done / max(1, total_bytes), 'count': count})
                            continue

                        # Detect header heuristically
                        has_header = False
                        if len(first) >= 2:
                            a0 = str(first[0]).lower()
                            a1 = str(first[1]).lower()
                            if 'seed' in a0 or 'match' in a1 or 'level' in a1:
                                has_header = True

                        if has_header:
                            rows_iter = reader
                        else:
                            rows_iter = itertools.chain([first], reader)

                        for parts in rows_iter:
                            if stop_event.is_set():
                                q.put({'canceled': True})
                                return
                            if not parts:
                                continue
                            rows_read += 1
                            seed = parts[0].strip().strip('"') if len(parts) >= 1 else ''
                            lvl = 1
                            name_in_row = ''
                            if len(parts) >= 2:
                                try:
                                    lvl = int(parts[1])
                                except Exception:
                                    lvl = 1
                            if len(parts) >= 3:
                                name_in_row = parts[2].strip()
                                if name_in_row.startswith('"') and name_in_row.endswith('"'):
                                    name_in_row = name_in_row[1:-1]

                            ok = False
                            if selected_name:
                                if name_in_row:
                                    ok = (name_in_row == selected_name)
                                else:
                                    if lvl > 0 and lvl <= len(self._result_names):
                                        ok = (self._result_names[lvl-1] == selected_name)
                            else:
                                if mode == 'at-least':
                                    ok = (lvl >= level)
                                else:
                                    ok = (lvl == level)

                            if ok:
                                count += 1
                                # reservoir sampling insertion
                                if len(reservoir) < RESERVOIR_SIZE:
                                    reservoir.append(seed)
                                else:
                                    # replace with decreasing probability
                                    r = random.randrange(count)
                                    if r < RESERVOIR_SIZE:
                                        reservoir[r] = seed
                            # occasionally update progress (every 2000 rows)
                            if rows_read % 2000 == 0:
                                try:
                                    pos = fh.tell()
                                except Exception:
                                    pos = 0
                                bytes_done = bytes_base + pos
                                q.put({'progress': bytes_done / max(1, total_bytes), 'count': count, 'seeds': list(reservoir)})

                except Exception:
                    # skip problematic file but update progress
                    bytes_done = bytes_base + file_sizes[idx]
                    q.put({'progress': bytes_done / max(1, total_bytes), 'count': count})
                    continue

                # advance base by this file size for next iteration
                bytes_base += file_sizes[idx]

            # final update; include up to RESERVOIR_SIZE seeds
            q.put({'progress': 1.0, 'count': count, 'seeds': list(reservoir), 'done': True})

            # Create progress UI
        try:
            from tkinter import Toplevel, Label, Button
            progress_top = Toplevel(self.root)
            progress_top.title('Scanning matches...')
            progress_top.transient(self.root)
            progress_top.grab_set()

            frm = ttk.Frame(progress_top, padding=8)
            frm.pack(fill='both', expand=True)
            progress_lbl = Label(frm, text='Scanning files...')
            progress_lbl.pack(anchor='w')
            progress_bar = ttk.Progressbar(frm, orient='horizontal', length=400, mode='determinate')
            progress_bar.pack(fill='x', pady=8)
            info_lbl = Label(frm, text='Matches found: 0')
            info_lbl.pack(anchor='w')

            # Live-updating list of collected seeds
            seeds_lbl = Label(frm, text='Collected seeds:')
            seeds_lbl.pack(anchor='w', pady=(6,0))
            seeds_listbox = scrolledtext.ScrolledText(frm, height=6, wrap='none')
            seeds_listbox.pack(fill='both', expand=False)
            seeds_listbox.config(state='disabled')

            def do_cancel():
                stop_event.set()

            btn = Button(frm, text='Cancel', command=do_cancel)
            btn.pack()
        except Exception:
            progress_top = None

        th = threading.Thread(target=worker, daemon=True)
        th.start()

        # poll queue and update UI
        def poll():
            try:
                while True:
                    msg = q.get_nowait()
                    if 'progress' in msg and progress_bar is not None:
                        try:
                            progress_bar['value'] = msg['progress'] * 100.0
                        except Exception:
                            pass
                    if 'count' in msg and progress_top is not None:
                        try:
                            seeds = msg.get('seeds') or []
                            info_lbl.config(text=f'Matches found: {msg.get("count",0)}  Seeds: {len(seeds)}')
                            # update live seeds list
                            try:
                                seeds_listbox.config(state='normal')
                                seeds_listbox.delete('1.0', 'end')
                                for s in seeds:
                                    seeds_listbox.insert('end', s + '\n')
                                seeds_listbox.config(state='disabled')
                            except Exception:
                                pass
                        except Exception:
                            pass
                    if 'canceled' in msg and msg['canceled']:
                        seeds = msg.get('seeds') or []
                        if progress_top:
                            try:
                                progress_top.destroy()
                            except Exception:
                                pass
                        if not seeds:
                            messagebox.showinfo('Canceled', 'Random pick canceled; no seeds collected')
                            return

                        # Reuse the same describe-and-show routine used for completion
                        filt = self.filter_var.get().strip()

                        def start_fetch_on_cancel(seeds_list, filt_name):
                            # same logic as finalization: fetch and display
                            try:
                                # call into the same helper defined in the done branch by reusing
                                # the _describe_match_for_seed path inline here for simplicity
                                results = []
                                for s in seeds_list:
                                    try:
                                        j = self._describe_match_for_seed(s, filt_name)
                                    except Exception:
                                        j = None
                                    # store the raw parsed JSON (or None) so the UI can render a Treeview
                                    # when 'cards' is present. Formatting for display is done later.
                                    results.append((s, j))

                                def show_results_cancel():
                                    try:
                                        first_seed = seeds_list[0]
                                        try:
                                            self.seed_var.set(first_seed)
                                        except Exception:
                                            pass
                                        try:
                                            self.root.clipboard_clear()
                                            self.root.clipboard_append('\n'.join(seeds_list))
                                        except Exception:
                                            pass

                                        from tkinter import Toplevel, Label, Button
                                        top = Toplevel(self.root)
                                        top.title('Seed details (canceled)')
                                        frm = ttk.Frame(top, padding=8)
                                        frm.pack(fill='both', expand=True)
                                        lbl = Label(frm, text=f'Collected {len(results)} seed(s) before cancel - details:')
                                        lbl.pack(anchor='w')
                                        st = scrolledtext.ScrolledText(frm, height=20, wrap='word')
                                        st.pack(fill='both', expand=True)
                                        for seed_val, body in results:
                                            st.insert('end', f'Seed: {seed_val}\n')
                                            # body may be a dict (parsed JSON) or a string
                                            if isinstance(body, dict):
                                                try:
                                                    st.insert('end', self._format_describe_json(body) + '\n\n')
                                                except Exception:
                                                    st.insert('end', str(body) + '\n\n')
                                            else:
                                                st.insert('end', str(body) + '\n\n')
                                        st.see('1.0')

                                        def do_copy():
                                            try:
                                                alltext = '\n\n'.join([f'Seed: {s}\n{t}' for s, t in results])
                                                self.root.clipboard_clear()
                                                self.root.clipboard_append(alltext)
                                                messagebox.showinfo('Copied', 'Seed details copied to clipboard')
                                            except Exception:
                                                messagebox.showwarning('Copy failed', 'Could not copy to clipboard')

                                        btns = ttk.Frame(frm)
                                        btns.pack(fill='x', pady=(8,0))
                                        ttk.Button(btns, text='Copy details', command=do_copy).pack(side='right')
                                        ttk.Button(btns, text='Close', command=top.destroy).pack(side='right', padx=(8,0))
                                    except Exception:
                                        pass

                                try:
                                    self.root.after(0, show_results_cancel)
                                except Exception:
                                    pass
                            except Exception:
                                pass

                        threading.Thread(target=start_fetch_on_cancel, args=(seeds, filt), daemon=True).start()
                        return
                    if 'done' in msg and msg['done']:
                        seeds = msg.get('seeds') or []
                        if progress_top:
                            try:
                                progress_top.destroy()
                            except Exception:
                                pass
                        if not seeds:
                            messagebox.showinfo('No matches', 'No matching seeds found in dist/*.csv or chosen CSV')
                            return

                        # Fetch describe-match info for each seed in background to avoid blocking UI
                        filt = self.filter_var.get().strip()

                        def fetch_and_show(seeds_list, filt_name):
                            results = []
                            for s in seeds_list:
                                try:
                                    j = self._describe_match_for_seed(s, filt_name)
                                except Exception:
                                    j = None
                                # keep the parsed JSON so the details UI can inspect structured fields
                                results.append((s, j))

                            # Schedule UI update on main thread
                            def show_results():
                                # set first seed and copy raw seeds to clipboard
                                first_seed = seeds_list[0]
                                try:
                                    self.seed_var.set(first_seed)
                                except Exception:
                                    pass
                                try:
                                    self.root.clipboard_clear()
                                    self.root.clipboard_append('\n'.join(seeds_list))
                                except Exception:
                                    pass

                                # show a detailed dialog with results
                                try:
                                    from tkinter import Toplevel, Label, Button
                                    top = Toplevel(self.root)
                                except Exception:
                                    top = Tk()
                                top.title('Seed details')
                                # give a sensible default size so the details area is visible
                                try:
                                    top.minsize(640, 320)
                                except Exception:
                                    pass
                                frm = ttk.Frame(top, padding=8)
                                frm.pack(fill='both', expand=True)
                                lbl = Label(frm, text=f'Collected {len(results)} seed(s) - details:')
                                lbl.pack(anchor='w')
                                # Show results using a table when JSON has 'cards' (render inline)
                                # Helper that tries to detect JSON with 'cards' and render a Treeview
                                def render_result(parent, seed_val, body):
                                    # body may be a dict (from _describe_match_for_seed) or a plain string
                                    if isinstance(body, dict) and 'cards' in body and isinstance(body['cards'], list):
                                        # Build a Treeview table
                                        cols = ('name','count','turn','when','slot')
                                        tree = ttk.Treeview(parent, columns=cols, show='headings', height=8)
                                        tree.heading('name', text='Name')
                                        tree.heading('count', text='Cnt')
                                        tree.heading('turn', text='Turn')
                                        tree.heading('when', text='When')
                                        tree.heading('slot', text='Slot')
                                        tree.column('name', width=350, anchor='w')
                                        tree.column('count', width=40, anchor='center')
                                        tree.column('turn', width=60, anchor='center')
                                        tree.column('when', width=100, anchor='w')
                                        tree.column('slot', width=80, anchor='center')
                                        for c in body['cards']:
                                            if isinstance(c, dict):
                                                name = c.get('name','')
                                                cnt = c.get('count','')
                                                turn = c.get('turn','')
                                                when = c.get('when','')
                                                slot = c.get('slot','')
                                                tree.insert('', 'end', values=(name, cnt, turn, when, slot))
                                            else:
                                                tree.insert('', 'end', values=(str(c), '', '', '', ''))
                                        tree.pack(fill='both', expand=True, pady=(6,0))
                                        return tree
                                    else:
                                        # fallback: if it's a dict without 'cards' try to show
                                        # a compact summary (name/index) and then the full JSON below
                                        if isinstance(body, dict):
                                            # show a small summary of common fields if present
                                            try:
                                                summary_shown = False
                                                if any(k in body for k in ('name', 'index', 'card')):
                                                    sub = ttk.Frame(parent)
                                                    sub.pack(fill='x', pady=(6,0))
                                                    if 'name' in body:
                                                        ttk.Label(sub, text=f"Name: {body.get('name')}").pack(anchor='w')
                                                        summary_shown = True
                                                    if 'card' in body:
                                                        ttk.Label(sub, text=f"Card: {body.get('card')}").pack(anchor='w')
                                                        summary_shown = True
                                                    if 'index' in body:
                                                        ttk.Label(sub, text=f"Index: {body.get('index')}").pack(anchor='w')
                                                        summary_shown = True
                                                # always show the full JSON below for inspection
                                                st = scrolledtext.ScrolledText(parent, height=8, wrap='word')
                                                st.pack(fill='both', expand=True, pady=(6,0))
                                                try:
                                                    st.insert('end', json.dumps(body, ensure_ascii=False, indent=2))
                                                except Exception:
                                                    st.insert('end', str(body))
                                                st.see('1.0')
                                                return st
                                            except Exception:
                                                # fallback to raw string rendering
                                                st = scrolledtext.ScrolledText(parent, height=10, wrap='word')
                                                st.pack(fill='both', expand=True, pady=(6,0))
                                                st.insert('end', str(body))
                                                st.see('1.0')
                                                return st
                                        else:
                                            st = scrolledtext.ScrolledText(parent, height=10, wrap='word')
                                            st.pack(fill='both', expand=True, pady=(6,0))
                                            st.insert('end', str(body))
                                            st.see('1.0')
                                            return st

                                # Render results; catch and show any unexpected exceptions so dialog isn't empty
                                try:
                                    widgets = []
                                    for seed_val, body in results:
                                        header = ttk.Label(frm, text=f'Seed: {seed_val}')
                                        header.pack(anchor='w', pady=(6,0))
                                        # If body is a JSON string, attempt to parse to dict
                                        parsed = body
                                        if isinstance(body, str):
                                            try:
                                                parsed = json.loads(body)
                                            except Exception:
                                                parsed = body
                                        w = render_result(frm, seed_val, parsed)
                                        widgets.append(w)
                                except Exception:
                                    # show traceback in a scrolled text so the user can report it
                                    import traceback as _tb
                                    st_err = scrolledtext.ScrolledText(frm, height=12, wrap='word')
                                    st_err.pack(fill='both', expand=True, pady=(6,0))
                                    st_err.insert('end', 'Error rendering seed details:\n')
                                    st_err.insert('end', _tb.format_exc())
                                    st_err.see('1.0')

                                def do_copy():
                                    try:
                                        # Copy results as a readable text (CSV-like)
                                        lines = []
                                        for s, b in results:
                                            lines.append(f'Seed: {s}')
                                            if isinstance(b, dict) and 'cards' in b:
                                                for c in b['cards']:
                                                    if isinstance(c, dict):
                                                        lines.append(','.join([str(c.get('name','')), str(c.get('count','')), str(c.get('turn','')), str(c.get('when','')), str(c.get('slot','')), str(c.get('position',''))]))
                                                    else:
                                                        lines.append(str(c))
                                            else:
                                                lines.append(str(b))
                                            lines.append('')
                                        alltext = '\n'.join(lines)
                                        self.root.clipboard_clear()
                                        self.root.clipboard_append(alltext)
                                        messagebox.showinfo('Copied', 'Seed details copied to clipboard')
                                    except Exception:
                                        messagebox.showwarning('Copy failed', 'Could not copy to clipboard')

                                btns = ttk.Frame(frm)
                                btns.pack(fill='x', pady=(8,0))
                                ttk.Button(btns, text='Copy details', command=do_copy).pack(side='right')
                                ttk.Button(btns, text='Close', command=top.destroy).pack(side='right', padx=(8,0))

                                # show it
                                try:
                                    top.lift()
                                except Exception:
                                    pass

                            try:
                                self.root.after(0, show_results)
                            except Exception:
                                pass

                        t = threading.Thread(target=fetch_and_show, args=(seeds, filt), daemon=True)
                        t.start()
                        return
            except queue.Empty:
                pass
            # continue polling
            self.root.after(120, poll)

        poll()

    def on_choose_csv(self):
        path = filedialog.askopenfilename(title='Choose matches CSV', filetypes=[('CSV files','*.csv'),('All files','*.*')], initialdir=DIST_DIR)
        if path:
            self.chosen_csv = path
            self.append_run(f'Chosen CSV for random picks: {path}\n')

    def on_stop(self):
        try:
            self.process_runner.stop()
            self.append_run('\n[stopped]\n')
        except Exception as e:
            messagebox.showerror('Stop Error', str(e))

    def on_open_dist(self):
        os.makedirs(DIST_DIR, exist_ok=True)
        if sys.platform.startswith('win'):
            os.startfile(DIST_DIR)
        elif sys.platform.startswith('darwin'):
            subprocess.run(['open', DIST_DIR])
        else:
            subprocess.run(['xdg-open', DIST_DIR])

    def _on_load_results(self):
        filt = self.filter_var.get().strip()
        if not filt:
            messagebox.showerror('Error', 'Please select a filter')
            return
        exe = os.path.join(DIST_DIR, f'immolate_{filt}.exe' if os.name == 'nt' else f'immolate_{filt}')
        if not os.path.exists(exe):
            if not messagebox.askyesno('Executable not found', 'Executable not found. Build now?'):
                return
            self.on_build()
            if not os.path.exists(exe):
                messagebox.showerror('Error', f'Executable still not found: {exe}')
                return

        # Run the exe with --list-results and parse JSON array of strings
        try:
            res = subprocess.run([exe, '--list-results'], cwd=REPO_ROOT, capture_output=True, text=True)
            out = res.stdout.strip()
            if not out:
                self.append_run('No output from executable when requesting results.\n')
                return
            # Try to parse simple JSON array of strings
            import ast
            try:
                arr = ast.literal_eval(out)
                if isinstance(arr, list):
                    # Populate combobox and internal mappings
                    self._populate_result_names(arr)
                    self._show_results_dialog(arr)
                    return
            except Exception:
                pass
            # Fallback: try to extract quoted strings
            import re
            names = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', out)
            if names:
                self._populate_result_names(names)
                self._show_results_dialog(names)
            else:
                self.append_run('Could not parse result names from executable output.\n')
        except Exception as e:
            messagebox.showerror('Error', f'Failed to run executable: {e}')

    def _show_results_dialog(self, names):
        # Show an interactive dialog listing names with indices and allow applying a selection
        dlg = ttk.Frame(self.root)
        try:
            from tkinter import Toplevel
            top = Toplevel(self.root)
        except Exception:
            top = Tk()
        top.title('Filter result names')

        frm = ttk.Frame(top, padding=8)
        frm.pack(fill='both', expand=True)

        lbl = ttk.Label(frm, text='Select a result to apply:')
        lbl.pack(anchor='w')

        listbox = ttk.Treeview(frm, columns=('index','name'), show='headings', height=12)
        listbox.heading('index', text='#')
        listbox.heading('name', text='Name')
        listbox.column('index', width=40, anchor='center')
        listbox.column('name', width=600, anchor='w')
        listbox.pack(fill='both', expand=True)

        for i, n in enumerate(names, start=1):
            listbox.insert('', 'end', values=(i, n))

        btns = ttk.Frame(frm)
        btns.pack(fill='x', pady=(8,0))

        def _apply_selection():
            sel = listbox.selection()
            if not sel:
                return
            item = sel[0]
            vals = listbox.item(item, 'values')
            try:
                idx = int(vals[0])
                name = vals[1]
            except Exception:
                return
            # populate combobox/mapping (ensures _result_names/_name_to_index exist)
            self._populate_result_names(names)
            try:
                self.result_name_var.set(name)
            except Exception:
                pass
            try:
                self.match_level_var.set(idx)
            except Exception:
                pass
            # persist selection for this filter
            filt = self.filter_var.get().strip()
            if filt:
                self._gui_state.setdefault('selected_by_filter', {})[filt] = name
                self._save_gui_state()
            try:
                top.destroy()
            except Exception:
                pass

        def _close():
            try:
                top.destroy()
            except Exception:
                pass

        ttk.Button(btns, text='Apply selection', command=_apply_selection).pack(side='left')
        ttk.Button(btns, text='Close', command=_close).pack(side='left', padx=(8,0))

    def _populate_result_names(self, names):
        # names is a list of strings (1-based indices)
        self._result_names = list(names)
        self._name_to_index = {n: i+1 for i, n in enumerate(self._result_names)}
        try:
            self.result_name_cb['values'] = self._result_names
        except Exception:
            pass
        # Try to restore previously selected name for current filter
        filt = self.filter_var.get().strip()
        sel = None
        try:
            sel = self._gui_state.get('selected_by_filter', {}).get(filt, None)
        except Exception:
            sel = None
        if sel and sel in self._name_to_index:
            try:
                self.result_name_var.set(sel)
                self.match_level_var.set(self._name_to_index[sel])
            except Exception:
                pass

    def _on_result_name_selected(self, event=None):
        name = self.result_name_var.get()
        if not name:
            return
        idx = self._name_to_index.get(name, None)
        if idx is not None:
            try:
                self.match_level_var.set(idx)
            except Exception:
                pass
        # persist selection for this filter
        filt = self.filter_var.get().strip()
        if filt:
            try:
                self._gui_state.setdefault('selected_by_filter', {})[filt] = name
                self._save_gui_state()
            except Exception:
                pass

    def _describe_match_for_seed(self, seed, filt):
        # Run the compiled exe with --describe-match --seed <seed> and parse JSON
        exe = os.path.join(DIST_DIR, f'immolate_{filt}.exe' if os.name == 'nt' else os.path.join(DIST_DIR, f'immolate_{filt}'))
        if not os.path.exists(exe):
            return None
        try:
            res = subprocess.run([exe, '--describe-match', '--seed', seed], cwd=REPO_ROOT, capture_output=True, text=True, timeout=5)
            out = res.stdout.strip()
            if not out:
                return None
            j = json.loads(out)
            return j
        except Exception:
            return None

    def _format_describe_json(self, j):
        # Return a human-readable string summary for describe-match JSON
        if j is None:
            return '(no description available)'
        if isinstance(j, dict):
            # Prefer well-structured 'cards' array and render as a table
            if 'cards' in j and isinstance(j['cards'], list) and j['cards']:
                rows = []
                for c in j['cards']:
                    if isinstance(c, dict):
                        name = str(c.get('name') or c.get('card') or '')
                        cnt = c.get('count')
                        cnt_s = str(cnt) if cnt is not None else ''
                        turn = c.get('turn')
                        turn_s = str(turn) if turn is not None else ''
                        when = c.get('when') or c.get('timing') or ''
                        slot = c.get('slot') or ''
                        rows.append((name, cnt_s, turn_s, when, slot))
                    else:
                        rows.append((str(c), '', '', '', ''))

                # Column widths (name gets the remainder)
                name_w = 36
                cnt_w = 4
                turn_w = 6
                when_w = 10
                slot_w = 10
                header = f"{ 'Name':{name_w} } { 'Cnt':>{cnt_w} } { 'Turn':>{turn_w} } { 'When':{when_w} } { 'Slot':{slot_w} }"
                sep = '-' * (name_w + cnt_w + turn_w + when_w + slot_w + 4)
                body_lines = [header, sep]
                for r in rows:
                    name = (r[0][:name_w-1] + '…') if len(r[0]) > name_w else r[0]
                    body_lines.append(f"{name:{name_w}} {r[1]:>{cnt_w}} {r[2]:>{turn_w}} {r[3]:{when_w}} {r[4]:{slot_w}}")
                return '\n'.join(body_lines)

            # fallback for synergies / other arrays we've seen
            if 'synergies' in j and isinstance(j['synergies'], list):
                parts = []
                for item in j['synergies']:
                    if isinstance(item, dict):
                        card = item.get('card') or item.get('name') or str(item)
                        when = item.get('when') or item.get('turn') or item.get('timing')
                        parts.append(f"{card}" + (f" @ {when}" if when else ''))
                    else:
                        parts.append(str(item))
                return '\n'.join(parts)

            # else pretty-print JSON for inspection
            try:
                return json.dumps(j, ensure_ascii=False, indent=2)
            except Exception:
                return str(j)

        return str(j)

    def _load_gui_state(self):
        try:
            if os.path.exists(self._gui_state_path):
                with open(self._gui_state_path, 'r', encoding='utf-8') as fh:
                    self._gui_state = json.load(fh)
        except Exception:
            self._gui_state = {}

    def _save_gui_state(self):
        try:
            os.makedirs(os.path.dirname(self._gui_state_path), exist_ok=True)
            with open(self._gui_state_path, 'w', encoding='utf-8') as fh:
                json.dump(self._gui_state, fh, ensure_ascii=False, indent=2)
        except Exception:
            pass


if __name__ == '__main__':
    # Launch the GUI when the module is executed directly.
    try:
        root = Tk()
        app = App(root)
        root.mainloop()
    except Exception:
        import traceback
        traceback.print_exc()
        raise

