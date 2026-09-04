# Snake

A classic snake game for the Windows console, written in C with no external dependencies.

## Build

Requires a C compiler (MinGW-w64 gcc).

```
build.bat
```

or

```
gcc -std=c99 -Wall -Wextra -O2 -o snake.exe snake.c
```

## Run

```
snake.exe
```

## Controls

| Key | Action |
| --- | --- |
| `W` `A` `S` `D` or arrow keys | Move |
| `T` | Toggle autoplay |
| `P` | Pause |
| `Q` | Quit |
| `R` | Restart after game over |

Eat the `*` to grow. Each apple is worth 10 points, and the snake speeds up as the score
climbs. Hitting a wall or your own body ends the run.

## Autoplay

Press `T` at any time to hand over to the computer, or start with it already running:

```
snake.exe --auto
```

The autoplayer solves the board every time. It follows a Hamiltonian cycle — a fixed route
that passes through all 800 cells exactly once — so it can never trap itself. Blindly
following that route would be painfully slow, so it also takes shortcuts: at each step it
looks for a neighbouring cell further along the route, but refuses to jump past its own tail
or past the food. That keeps the safety guarantee while cutting out most of the detour.

If you play by hand first and then switch autoplay on, the snake may be in a position the
cycle rule cannot describe. In that case it falls back to a breadth-first search: it finds
the shortest path to the food, checks by simulation that it can still reach its own tail
after eating, and otherwise chases its tail or heads for the largest open area.

No machine learning is involved — both strategies are plain graph algorithms.

## Colour

The board uses ANSI colour, which Windows 10 and 11 terminals support. If the terminal
refuses to enable it, the game detects that and falls back to plain text.
