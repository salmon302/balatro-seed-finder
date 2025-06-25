# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## General Guidelines
- When building using tools, always build from root directory (example : ./tools/build.sh perkeo)

## Building and Running

### Build System
Use the build script to compile with specific filters:
```bash
cd tools
./build.sh <filter_name>
```

Available filters: `perkeo`, `perkeo_only`, `any_legendary`, `charm_tag`, `observatory`, `observatory_perkeo`, `free_neg_bl_trib`

Executables are generated in `dist/` as `immolate_<filter_name>`

### Running
```bash
# Start search from beginning
./dist/immolate_<filter_name>

# Start from specific seed
./dist/immolate_<filter_name> ABCDEFGH
```

## Architecture

### Core Components
- **immolate.cpp**: Main search engine with multi-threaded seed exploration
- **instance.hpp/Instance**: Balatro game state simulation engine with RNG
- **filters/**: Modular filter system for different search criteria
- **items.hpp**: Balatro game items (jokers, cards, vouchers, etc.)
- **functions.hpp**: Core game mechanics and RNG functions

### Filter System
Filters implement `SearchFilter` interface in `filters/filter_base.hpp`:
- `apply(seed)`: Returns match level (0=no match, 1+=different match types)
- `getResultNames()`: Names for each match type for statistics
- `getName()`: Filter display name

Each filter is conditionally compiled via `SELECTED_FILTER` preprocessor definition.

### Search Process
1. Convert 8-character Balatro seeds to numbers for sequential exploration
2. Multi-threaded workers simulate game instances for each seed
3. Apply filter logic to check match conditions
4. Log matches to timestamped CSV files
5. Display real-time statistics (progress, match rates, ETA)

### Game Simulation
Instance class simulates Balatro mechanics:
- `nextTag(ante)`: Get tags for specific ante
- `nextArcanaPack(size, ante)`: Simulate arcana pack contents
- `nextJoker(type, ante, legendary)`: Get joker drops
- `nextBoss(ante)`, `nextVoucher(ante)`: Boss/voucher selection
- RNG seeded from game seed for deterministic results

### Performance Optimizations
- Bitmap-based locking system for game state
- Forced inline functions for hot paths
- Compile-time filter selection
- Thread-local RNG instances
- Early exit conditions in filters