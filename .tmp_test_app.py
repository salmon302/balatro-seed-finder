import traceback
try:
    from tkinter import Tk
    from gui.seed_finder_gui import App
    root = Tk()
    root.withdraw()
    app = App(root)
    print('OK')
    root.destroy()
except Exception:
    traceback.print_exc()
    raise
