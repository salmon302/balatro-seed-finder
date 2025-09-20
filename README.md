# Balatro Seed Finder

Balatro Seed Finder, is an CPU-bound engine to search for specific seeds based on filters in the the filters directory.

## Attributions

Most of the engine was taken from [TheSoul](https://github.com/SpectralPack/TheSoul) and [Immolate](https://github.com/SpectralPack/TheSoul) from the [SpectralPack](https://github.com/SpectralPack/) team. Thank you for the amazing work on the simulation.

## How to

### Windows quick start (GUI)

We now include a simple GUI to build and run the seed finder on Windows.

Requirements:
- Python 3.8+ (for the GUI; Tkinter comes with the standard Python installer)
- A C++ compiler available as `g++` in your PATH (e.g. MinGW-w64)

Steps:
1. Install Python 3 from python.org (ensure “Add to PATH” is selected) and install MinGW-w64 so that `g++` is available in your terminal.
2. Double-click `run_gui.bat` (or run it from a terminal). This launches the Tkinter GUI.
3. In the GUI, select a filter, optionally set a seed and thread count, click Build, then Run.
  - Debug mode runs the filter for a single seed and writes a `debug_<seed>.txt` file.
  - Normal mode logs matches to `dist/matches_YYYYMMDD_HHmmss.csv`.

If `g++` is not found, the GUI’s build step will fail. Install MinGW-w64 and relaunch.

Manual launch without double-click:
- From PowerShell: `./run_gui.bat`

### How to build a specific filter

You can build an existing filter with the following command : `sh tools/build.sh <filter_name>`. The `filter_name` is the same name of your filter name without `_filter.hpp`. 

For example, to build the Perkeo filter you can run : `sh tools/build.sh perkeo`. It will generate the executable in `dist/seed_finder_<filter_name>`.

### Run the seed finder

Once your build done, you can run the executable to start the search. The executable can receive as an argument the 8-char seed to begin with. It is quite helpful to resume an interrupted process.

The results will the logged in a `matches_YYYYMMDD_HHmmss.csv` file. It only logs the result with a score of at least 1.

### How to design a filter

Filters are in the `filters` directory. It contains a README.md file to understand how to write your own filters.

## Windows notes

- A Windows batch build script is provided at `tools/build.bat` with the same usage as the Unix script: `tools\build.bat <filter_name>`.
- Executables built on Windows are named like `dist/immolate_<filter>.exe`.
- The GUI lives at `gui/seed_finder_gui.py` and is launched via `run_gui.bat`.

## Next steps

We would like to have a second, more tool that indexes all Balatro seeds, with tags about specifics of a seed (for example, Perkeo on first Charm Tag). 

### Opti ideas
  Better optimization approaches for this codebase would be:
  - SIMD vectorization (AVX-512) to process multiple seeds per CPU core
  - Memory optimizations (pooling, reducing string ops)
  - Algorithmic improvements (early pruning, parallel filter chains)