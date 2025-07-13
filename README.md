# Tug-of-War Game

A multi-process tug-of-war simulation written in C for Linux. Two teams of players compete by sending “energy” updates over FIFOs, while a GLUT/OpenGL GUI visualizes the rope, scores, and player states in real time.

---

## Demo

![Tug-of-War Demo](./demo.gif)

---


## Features

- **Configurable parameters** via `arguments.txt` (round count, energy ranges, decay/recovery rates, thresholds, etc.).
- **Inter-process communication** using UNIX FIFOs (`mkfifo`, `open`, `read`, `write`).
- **Process control** with `fork()`, Unix signals (`SIGUSR1`, `SIGUSR2`, `SIGQUIT`).
- **Real-time GUI** built on GLUT/OpenGL showing rope position, player avatars, energy bars, scores, and winning screen.
- **Dynamic sorting** of player energies each round to determine ranking and influence on rope pull.

---

## Requirements

- Linux OS (tested on Ubuntu/Debian)
- GNU C compiler (`gcc`)
- Make (`make`)
- Libraries:
  - `GLUT` / `freeglut`
  - `OpenGL` (`-lGL -lGLU -lglut`)
  - Standard C libraries (`unistd.h`, `fcntl.h`, `sys/stat.h`, etc.)

---

## Installation & Build

1. **Clone the repo** (or copy the source files) into a directory.
2. **Edit configuration** in `arguments.txt` as desired.
3. **Build** with:
   ```bash
   make
   ```
   This produces one executable:
   - `game` ( after compilation run the project with `./game`)

---

## Configuration

All configurable parameters live in `arguments.txt` (one `KEY=VALUE` per line, comments and blank lines allowed). Defaults:

```text
NUMBER_OF_ROUNDS=5
THRESHOLD=400
ROUND_TIME=10
INITIAL_ENERGY_MIN=20
INITIAL_ENERGY_MAX=80
ENERGY_DECAY_MIN=1
ENERGY_DECAY_MAX=5
FALL_TIME_MIN=2
FALL_TIME_MAX=6
ENERGY_INCREASE_MIN=1
ENERGY_INCREASE_MAX=10
```

Parameters control how many rounds are played, how energy is generated/decayed/recovered, and when a round ends early.

---

## Usage

1. Ensure `arguments.txt` is in the working directory.
2. Run the main controller:
   ```bash
   ./game
   ```
3. Watch the console logs for process statuses, then enjoy the GUI window showing:
   - Rope displacement
   - Player icons colored by energy level
   - Scores, round number, and timer
   - Final “Team X Wins!” screen

Press **Ctrl+C** in the terminal or close the GUI window to exit.

---


## Project Structure

```
.
├── Makefile
├── arguments.txt        ← Configuration file
├── header.h            ← Shared types, globals, and function prototypes
├── main.c              ← Controller: forks GUI + players, manages rounds
├── player.c            ← Player logic: energy exchange, ranking, periodic updates
├── player.h            ← Player-specific declarations
├── functions.c         ← FIFO read/write, config parser, helpers
├── tug_of_war.c        ← GLUT/OpenGL visualization
└── README.md           ← (this file)
```

---

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.

---

## Author

**Taleen Abuzulof**  
Computer Engineering Student – Linux C & IPC Enthusiast
