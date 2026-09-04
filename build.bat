@echo off
gcc -std=c99 -Wall -Wextra -O2 -o snake.exe snake.c
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Build OK. Run snake.exe to play.
