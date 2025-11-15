# Abyssal Rift: Operation Tidebreak

Single-file OpenGL/GLUT game for DMET502 Assignment 2. The scene represents an underwater research dome that satisfies the brief: ground plane, four walls, diver player, at least three oxygen-cell goals, and five themed props (floodlight fence, airlock gate, coral cluster, console, repair drone). Lighting, animations, camera controls, HUD, and win/lose transitions are implemented per the specification excerpted below.

## Scene Contents

- **Ground & Walls:** Textured seabed slab plus color-cycling floodlit walls enclosing a 2×2×0.7 volume.
- **Player (Diver):** Built from 7+ primitives (body chassis, tanks, limbs, helmet) with yaw rotation on movement and x-axis tilt while airborne.
- **Goals:** Three oxygen cells, each made of cube + sphere + antenna, spinning continuously until collected.
- **Major Objects (≥5 primitives each):**
  - Floodlight mast (base, pole, swivel head, light housing, lens).
  - Airlock gate (frame, animated twin doors, reinforcement beam).
- **Regular Objects:** Coral cluster, diagnostics console, repair drone—each ≥3 primitives and individually animated.
- **HUD:** Goals remaining and countdown timer; overlays for GAME WIN / GAME LOSE.

## Gameplay & Controls

- **Movement:** `I/J/K/L` move diver on XZ plane; `R/F` swim up/down within bounds.
- **Camera translation:** `Q/E` dolly forward/back, `W/S` elevate, `A/D` strafe sideways.
- **Camera rotations:** Arrow keys pitch/yaw the free camera.
- **Preset views:** `1` front, `2` side, `3` top, `0` free-orbit.
- **Animations:** Keys `5–9` toggle the five prop animations (floodlight sweep, airlock doors, coral sway, console pulse, drone hover).
- **Restart:** `P` resets timer, goals, and animations when WIN/LOSE screen appears.

## Build & Run (macOS)

```bash
cd "/Users/hatem/University/w_25/DMET502/Assignments & Projects/2/a2/A2"
clang++ Lab6_Solution.cpp -std=c++17 -DGL_SILENCE_DEPRECATION \
  -framework GLUT -framework OpenGL -framework Cocoa \
  -o underwater_base
./underwater_base
```

### Why `brew unlink freeglut`?

Homebrew's `freeglut` builds against X11 and will crash with `failed to open display ''` when no XQuartz server is active. Unlink it once so the linker picks Apple’s GLUT framework instead:

```bash
brew unlink freeglut
```

Rebuild afterward using the command above; the resulting binary uses the native Cocoa windowing path and launches normally.

## Audio & Credits

Planned audio set (meets the “3 distinct sounds” bonus once wired in):

- **Background track:** "Crab Rave" – Noisestorm (Monstercat). Loop via `afplay` or embed audio player; credit in-game HUD + README.
- **Animation servo cue:** Short hydraulic/servo sample from FreeSound (e.g., `freesound.org/people/…` — update with exact asset once finalized).
- **Goal pickup effect:** Bubble pop/chime sample (FreeSound). Trigger when an oxygen cell is collected.
- **Timer fail cue (optional fourth):** Alarm/klaxon sample to emphasize the GAME LOSE transition.

> Remember to keep local `.mp3`/`.wav` assets in `audio/` and cite creator + license next to each bullet once locked.

## Assignment Checklist

- 9+ distinct object types inside the boundary volume.
- Player composed of ≥6 primitives with rotation rules for ground vs. air movement.
- Goal objects composed of ≥3 primitives each, animating from the start.
- Five themed props with individual animations (start/stop via numeric keys).
- Camera freely translatable + three preset views bound to single key presses.
- Collision-limited movement, countdown timer, and win/lose transitions on goal completion or timeout.
- Single source file (`Lab6_Solution.cpp`) as required for submission.

## Submission Reminder

Follow the course instructions: rename the file to `PXX-58-XXXX.cpp` before uploading, submit only the C++ source (or a zipped pair if you break it up), and be prepared to explain your implementation during evaluation.
