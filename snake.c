#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define WIDTH 40
#define HEIGHT 20
#define MAX_LENGTH (WIDTH * HEIGHT)
#define START_DELAY 130
#define MIN_DELAY 55
#define FRAME_STEP 5

typedef struct {
    int x;
    int y;
} Point;

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

static Point snake[MAX_LENGTH];
static Point food;
static int length;
static int score;
static Direction dir;
static Direction nextDir;
static int dirLocked;
static int gameOver;
static int won;
static int quit;

static HANDLE console(void)
{
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static void setCursorVisible(int visible)
{
    CONSOLE_CURSOR_INFO info;

    if (!GetConsoleCursorInfo(console(), &info))
        return;
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(console(), &info);
}

static void moveCursorHome(void)
{
    COORD home = { 0, 0 };

    SetConsoleCursorPosition(console(), home);
}

static void clearScreen(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD home = { 0, 0 };
    DWORD cells;
    DWORD written;

    if (!GetConsoleScreenBufferInfo(console(), &info))
        return;

    cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
    FillConsoleOutputCharacter(console(), ' ', cells, home, &written);
    FillConsoleOutputAttribute(console(), info.wAttributes, cells, home, &written);
    moveCursorHome();
}

static int isOnSnake(int x, int y)
{
    int i;

    for (i = 0; i < length; i++) {
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    }
    return 0;
}

static void placeFood(void)
{
    do {
        food.x = rand() % WIDTH;
        food.y = rand() % HEIGHT;
    } while (isOnSnake(food.x, food.y));
}

static void initGame(void)
{
    int i;

    length = 4;
    for (i = 0; i < length; i++) {
        snake[i].x = WIDTH / 2 - i;
        snake[i].y = HEIGHT / 2;
    }

    dir = DIR_RIGHT;
    nextDir = DIR_RIGHT;
    dirLocked = 0;
    score = 0;
    gameOver = 0;
    won = 0;
    placeFood();
}

static int frameDelay(void)
{
    int delay = START_DELAY - (score / 50) * 8;

    return delay < MIN_DELAY ? MIN_DELAY : delay;
}

static void draw(void)
{
    static char buffer[(WIDTH + 3) * (HEIGHT + 2) + 1];
    char board[HEIGHT][WIDTH];
    int x;
    int y;
    int i;
    int pos = 0;

    memset(board, ' ', sizeof(board));
    board[food.y][food.x] = '*';
    for (i = 1; i < length; i++)
        board[snake[i].y][snake[i].x] = 'o';
    board[snake[0].y][snake[0].x] = '@';

    for (x = 0; x < WIDTH + 2; x++)
        buffer[pos++] = '#';
    buffer[pos++] = '\n';

    for (y = 0; y < HEIGHT; y++) {
        buffer[pos++] = '#';
        for (x = 0; x < WIDTH; x++)
            buffer[pos++] = board[y][x];
        buffer[pos++] = '#';
        buffer[pos++] = '\n';
    }

    for (x = 0; x < WIDTH + 2; x++)
        buffer[pos++] = '#';
    buffer[pos++] = '\n';
    buffer[pos] = '\0';

    moveCursorHome();
    fputs(buffer, stdout);
    printf("  score %-6d length %-6d\n", score, length);
    printf("  WASD or arrow keys, P pause, Q quit\n");
    fflush(stdout);
}

static int isOpposite(Direction a, Direction b)
{
    return (a == DIR_UP && b == DIR_DOWN) ||
           (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

static void setDirection(Direction d)
{
    if (dirLocked || isOpposite(d, dir))
        return;
    nextDir = d;
    dirLocked = 1;
}

static void pauseGame(void)
{
    int key;

    printf("  paused - press any key to resume");
    fflush(stdout);

    while (_kbhit())
        _getch();

    key = _getch();
    if (key == 0 || key == 224)
        _getch();

    printf("\r                                        \r");
    fflush(stdout);
}

static void readInput(void)
{
    int key;

    while (_kbhit()) {
        key = _getch();

        if (key == 0 || key == 224) {
            switch (_getch()) {
            case 72:
                setDirection(DIR_UP);
                break;
            case 80:
                setDirection(DIR_DOWN);
                break;
            case 75:
                setDirection(DIR_LEFT);
                break;
            case 77:
                setDirection(DIR_RIGHT);
                break;
            }
            continue;
        }

        switch (key) {
        case 'w':
        case 'W':
            setDirection(DIR_UP);
            break;
        case 's':
        case 'S':
            setDirection(DIR_DOWN);
            break;
        case 'a':
        case 'A':
            setDirection(DIR_LEFT);
            break;
        case 'd':
        case 'D':
            setDirection(DIR_RIGHT);
            break;
        case 'p':
        case 'P':
            pauseGame();
            break;
        case 'q':
        case 'Q':
            quit = 1;
            return;
        }
    }
}

static void waitFrame(void)
{
    int remaining = frameDelay();

    while (remaining > 0 && !quit) {
        readInput();
        Sleep(FRAME_STEP);
        remaining -= FRAME_STEP;
    }
}

static void update(void)
{
    Point head = snake[0];
    Point tail = snake[length - 1];
    int i;

    dir = nextDir;
    dirLocked = 0;

    switch (dir) {
    case DIR_UP:
        head.y--;
        break;
    case DIR_DOWN:
        head.y++;
        break;
    case DIR_LEFT:
        head.x--;
        break;
    case DIR_RIGHT:
        head.x++;
        break;
    }

    if (head.x < 0 || head.x >= WIDTH || head.y < 0 || head.y >= HEIGHT) {
        gameOver = 1;
        return;
    }

    for (i = 0; i < length - 1; i++) {
        if (snake[i].x == head.x && snake[i].y == head.y) {
            gameOver = 1;
            return;
        }
    }

    for (i = length - 1; i > 0; i--)
        snake[i] = snake[i - 1];
    snake[0] = head;

    if (head.x == food.x && head.y == food.y) {
        score += 10;
        if (length < MAX_LENGTH) {
            snake[length] = tail;
            length++;
        }
        if (length >= MAX_LENGTH) {
            won = 1;
            gameOver = 1;
            return;
        }
        placeFood();
    }
}

static void showIntro(void)
{
    clearScreen();
    printf("\n  SNAKE\n\n");
    printf("  eat the * to grow, do not hit the walls or yourself\n");
    printf("  move with WASD or the arrow keys\n");
    printf("  P pauses, Q quits\n\n");
    printf("  press any key to start\n");

    if (_getch() == 224)
        _getch();
    clearScreen();
}

static int askReplay(void)
{
    int key;

    printf("\n  %s  final score %d\n", won ? "you filled the board!" : "game over", score);
    printf("  press R to play again, any other key to quit\n");

    while (_kbhit())
        _getch();

    key = _getch();
    if (key == 0 || key == 224)
        _getch();

    return key == 'r' || key == 'R';
}

int main(void)
{
    int playing = 1;

    srand((unsigned int)time(NULL));
    SetConsoleTitleA("Snake");
    setCursorVisible(0);
    showIntro();

    while (playing) {
        initGame();
        clearScreen();

        while (!gameOver && !quit) {
            draw();
            waitFrame();
            if (quit)
                break;
            update();
        }

        if (quit)
            break;

        draw();
        playing = askReplay();
        clearScreen();
    }

    clearScreen();
    setCursorVisible(1);
    return 0;
}
