Describe-match timing: limitations and options

Goal
----
Explain why the `describeMatch` JSON currently uses placeholder timing fields ("turn"/"when") and propose realistic options to produce exact per-card timing (turn/when/slot/position) for the GUI.

Current state
-------------
- Several filters now emit a structured JSON object from `describeMatch(seed)`, including a `cards` array. Each card item contains `name`, `slot`, `position`, and `count`.
- I added best-effort placeholders for timing in two filters (e.g. `enum_perkeo_filter.hpp`, `any_legendary_enum_filter.hpp`):
  - pack cards: `"turn": 0, "when": "pack_open"`
  - joker: `"turn": 0, "when": "shop_joker"`
- These are semantic hints only, not a computed schedule of when a card will be played or scored during the run.

Why exact timing is non-trivial
------------------------------
Computing the exact turn/when a card will be played or scored for a given seed is a non-trivial request because it requires re-running a deterministic (seeded) game simulation that reproduces the real in-game decision and resolution order. Considerations and interactions include:

- Card draw order and deck shuffling (packs add cards into the deck/consumables in specific ways).
- Hand size and the per-round hand-play decision process (which cards are played each hand depends on hand-selection logic or heuristics the real game uses or that we must emulate).
- Joker effects, retriggers, replays, and interactions that can cause cards to score multiple times or at different times.
- Boss Blinds, Tags, Vouchers, and global modifiers that alter hand counts, scoring, and card effects.
- Consumables (Tarot/Spectral effects) that can change rank/suit/edition/seal and therefore scoring eligibility.
- Game-specific timing: "on scored" vs "on blind select" vs "on played" triggers differ and affect when an effect runs.

Because these elements are spread across the simulation code and many filters currently only sample pack/shop outputs, a simple `describeMatch` cannot derive accurate per-card timing without a run-level simulator that executes the same logic the real game uses for playing hands and scoring.

Two realistic options
---------------------
Option A — Full (lightweight) deterministic run simulator (recommended for exact timing)
- What: implement a compact simulator that uses the existing `Instance` generation logic to step through a run and deterministically emulate the in-game play/resolution order for the purposes of timing.
- Implementation outline:
  1. Add a simulator helper (e.g. `tools/describe_simulator.hpp` + `.cpp` or `instance_simulator.hpp`) that can:
     - Accept an `Instance::Instance` (constructed with seed and env) and run the same pack/shop generation code.
     - Maintain deck/hand/discard and apply simplified but accurate play policy (deterministic algorithm for selecting which cards are played each hand). If you want perfect parity with the game, that policy must match the original game's played-hand selection; if unknown, we choose a documented deterministic heuristic.
     - Track events: when a card is drawn, when it's played, when it scores; record an event list per card indicating turn number and semantic `when` (e.g. `played`, `scored`, `pack_open`, `shop_joker`, `retrig`, ...).
     - Provide a lightweight API callable from `describeMatch` that returns structured timing data so filters can emit exact `turn`/`when` values and counts.
  2. Add unit tests and a small harness (`tools/describe_simulator_test.cpp`) to validate deterministic behavior for known seeds.
  3. Update `describeMatch` methods in filters to call the simulator and include the returned timing fields in JSON.
- Pros: Accurate per-card timing; GUI can show exact turn/when and detect multi-score events.
- Cons: Non-trivial work; must carefully emulate play policy and tie into many parts of the simulation; requires tests and validation.

Option B — Keep best-effort placeholders and improve UI/visibility (fast)
- What: propagate the placeholder fields across more filters and improve GUI labels/tooltips to make it clear these are semantic hints, not exact play times.
- Implementation outline:
  1. Expand the current placeholder pattern (pack_open/shop_joker) to other filters that already emit `cards`.
  2. Update GUI text/tooltip where it shows `Turn/When` to add a small note like "(approx: pack_open/shop_joker)" or make the column tooltip explain placeholders.
  3. Optionally add a small link or button in the GUI to say "Request exact timing" which would explain that it requires a run simulation.
- Pros: Quick to implement; gives consistent structured output for the GUI with minimal risk.
- Cons: Not exact timing; might mislead users if not clearly labelled.

Recommendation
--------------
- If you want precise, actionable per-card timing (best UX for users who need to know when a card will score), choose Option A. I can implement it in incremental pieces (simulator core, small tests, then wire to `describeMatch`) and validate with unit tests.
- If you prefer minimal changes that improve UX quickly, choose Option B and I will propagate placeholders and update GUI labels/tooltips.

Questions for you / next step
----------------------------
Which option do you want me to implement next?
- Reply "Option A" to proceed with the deterministic simulator implementation (I will create the simulator module, unit tests, and wire a single filter to use it as a proof-of-concept).
- Reply "Option B" to propagate placeholders across more filters and update the GUI text/tooltips (fast).
- Or reply with any variant (e.g. "Start with B, then A later") and I'll schedule tasks accordingly.

Notes
-----
- I intentionally kept this doc small and practical. If you prefer, I can expand the implementation plan for Option A into a step-by-step code changelog with estimated time/cost per step.
- The placeholder semantics I used are:
  - `turn: 0, when: "pack_open"` — indicates the card is produced by opening a pack (tarot/planet/standard/spectral)
  - `turn: 0, when: "shop_joker"` — indicates the card came from the shop's Joker slot (nextJoker)

