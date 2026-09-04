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
| `P` | Pause |
| `Q` | Quit |
| `R` | Restart after game over |

Eat the `*` to grow. Each apple is worth 10 points, and the snake speeds up as the score
climbs. Hitting a wall or your own body ends the run.
