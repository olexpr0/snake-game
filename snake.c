#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#define WIDTH 40
#define HEIGHT 20
#define MAX_LENGTH (WIDTH * HEIGHT)
#define START_DELAY 130
#define MIN_DELAY 55
#define FRAME_STEP 5
#define DRAW_BUFFER 16384

#define COLOR_HEAD "\033[1;92m"
#define COLOR_BODY "\033[32m"
#define COLOR_FOOD "\033[91m"
#define COLOR_WALL "\033[90m"
#define COLOR_RESET "\033[0m"

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

typedef struct {
    Point cells[MAX_LENGTH];
    int length;
} Body;

static const int DX[4] = { 0, 0, -1, 1 };
static const int DY[4] = { -1, 1, 0, 0 };

static Point snake[MAX_LENGTH];
static Point food;
static int cycleIndex[HEIGHT][WIDTH];
static Point cycleCell[MAX_LENGTH];
static int cycleReady;
static int length;
static int score;
static Direction dir;
static Direction nextDir;
static int dirLocked;
static int gameOver;
static int won;
static int quit;
static int autoPlay;
static int colorEnabled;

static HANDLE console(void)
{
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static void enableColor(void)
{
    DWORD mode;

    colorEnabled = 0;
    if (!GetConsoleMode(console(), &mode))
        return;
    if (SetConsoleMode(console(), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        colorEnabled = 1;
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

static int inBounds(int x, int y)
{
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
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

static void buildCycle(void)
{
    int x;
    int y;
    int step = 0;

    cycleReady = 0;
    if (HEIGHT % 2 != 0 || WIDTH < 2 || HEIGHT < 2)
        return;

    for (x = 0; x < WIDTH; x++)
        cycleIndex[0][x] = step++;

    for (y = 1; y < HEIGHT; y++) {
        if (y % 2 == 1) {
            for (x = WIDTH - 1; x >= 1; x--)
                cycleIndex[y][x] = step++;
        } else {
            for (x = 1; x < WIDTH; x++)
                cycleIndex[y][x] = step++;
        }
    }

    for (y = HEIGHT - 1; y >= 1; y--)
        cycleIndex[y][0] = step++;

    if (step != MAX_LENGTH)
        return;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            cycleCell[cycleIndex[y][x]].x = x;
            cycleCell[cycleIndex[y][x]].y = y;
        }
    }

    cycleReady = 1;
}

static void initGame(void)
{
    int i;

    length = 4;
    if (cycleReady) {
        int start = cycleIndex[HEIGHT / 2][WIDTH / 2];

        for (i = 0; i < length; i++)
            snake[i] = cycleCell[(start - i + MAX_LENGTH) % MAX_LENGTH];
    } else {
        for (i = 0; i < length; i++) {
            snake[i].x = WIDTH / 2 - i;
            snake[i].y = HEIGHT / 2;
        }
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

static const char *colorFor(char cell)
{
    switch (cell) {
    case '@':
        return COLOR_HEAD;
    case 'o':
        return COLOR_BODY;
    case '*':
        return COLOR_FOOD;
    case '#':
        return COLOR_WALL;
    default:
        return COLOR_RESET;
    }
}

static void appendText(char *buffer, int *pos, const char *text)
{
    while (*text)
        buffer[(*pos)++] = *text++;
}

static void draw(void)
{
    static char buffer[DRAW_BUFFER];
    char screen[HEIGHT + 2][WIDTH + 2];
    const char *last = NULL;
    const char *want;
    int x;
    int y;
    int i;
    int pos = 0;

    memset(screen, ' ', sizeof(screen));
    for (x = 0; x < WIDTH + 2; x++) {
        screen[0][x] = '#';
        screen[HEIGHT + 1][x] = '#';
    }
    for (y = 0; y < HEIGHT + 2; y++) {
        screen[y][0] = '#';
        screen[y][WIDTH + 1] = '#';
    }

    screen[food.y + 1][food.x + 1] = '*';
    for (i = 1; i < length; i++)
        screen[snake[i].y + 1][snake[i].x + 1] = 'o';
    screen[snake[0].y + 1][snake[0].x + 1] = '@';

    for (y = 0; y < HEIGHT + 2; y++) {
        for (x = 0; x < WIDTH + 2; x++) {
            if (colorEnabled) {
                want = colorFor(screen[y][x]);
                if (want != last) {
                    appendText(buffer, &pos, want);
                    last = want;
                }
            }
            buffer[pos++] = screen[y][x];
        }
        buffer[pos++] = '\n';
    }
    if (colorEnabled)
        appendText(buffer, &pos, COLOR_RESET);
    buffer[pos] = '\0';

    moveCursorHome();
    fputs(buffer, stdout);
    printf("  score %-6d length %-6d %s\n", score, length, autoPlay ? "[auto]" : "      ");
    printf("  WASD or arrows, T autoplay, P pause, Q quit\n");
    fflush(stdout);
}

static void bodyFromGame(Body *b)
{
    int i;

    b->length = length;
    for (i = 0; i < length; i++)
        b->cells[i] = snake[i];
}

static void bodyStep(Body *b, Point head, int grow)
{
    Point tail = b->cells[b->length - 1];
    int i;

    for (i = b->length - 1; i > 0; i--)
        b->cells[i] = b->cells[i - 1];
    b->cells[0] = head;

    if (grow && b->length < MAX_LENGTH) {
        b->cells[b->length] = tail;
        b->length++;
    }
}

static int findPath(const Body *b, Point start, Point goal, Point *path)
{
    static int dist[HEIGHT][WIDTH];
    static Point queue[MAX_LENGTH];
    int head = 0;
    int tail = 0;
    int x;
    int y;
    int i;
    int step;
    Point cur;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++)
            dist[y][x] = -1;
    }

    for (i = 0; i < b->length - 1; i++)
        dist[b->cells[i].y][b->cells[i].x] = -2;

    dist[start.y][start.x] = 0;
    queue[tail++] = start;

    while (head < tail) {
        cur = queue[head++];
        if (cur.x == goal.x && cur.y == goal.y)
            break;

        for (i = 0; i < 4; i++) {
            int nx = cur.x + DX[i];
            int ny = cur.y + DY[i];

            if (!inBounds(nx, ny) || dist[ny][nx] != -1)
                continue;

            dist[ny][nx] = dist[cur.y][cur.x] + 1;
            queue[tail].x = nx;
            queue[tail].y = ny;
            tail++;
        }
    }

    if (dist[goal.y][goal.x] < 0)
        return -1;

    cur = goal;
    for (step = dist[goal.y][goal.x] - 1; step >= 0; step--) {
        path[step] = cur;
        for (i = 0; i < 4; i++) {
            int nx = cur.x + DX[i];
            int ny = cur.y + DY[i];

            if (inBounds(nx, ny) && dist[ny][nx] == step) {
                cur.x = nx;
                cur.y = ny;
                break;
            }
        }
    }

    return dist[goal.y][goal.x];
}

static int reachableCells(const Body *b, Point start)
{
    static int seen[HEIGHT][WIDTH];
    static Point queue[MAX_LENGTH];
    int head = 0;
    int tail = 0;
    int count = 1;
    int x;
    int y;
    int i;
    Point cur;

    if (!inBounds(start.x, start.y))
        return 0;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++)
            seen[y][x] = 0;
    }

    for (i = 0; i < b->length - 1; i++)
        seen[b->cells[i].y][b->cells[i].x] = 1;

    if (seen[start.y][start.x])
        return 0;

    seen[start.y][start.x] = 1;
    queue[tail++] = start;

    while (head < tail) {
        cur = queue[head++];
        for (i = 0; i < 4; i++) {
            int nx = cur.x + DX[i];
            int ny = cur.y + DY[i];

            if (!inBounds(nx, ny) || seen[ny][nx])
                continue;

            seen[ny][nx] = 1;
            queue[tail].x = nx;
            queue[tail].y = ny;
            tail++;
            count++;
        }
    }

    return count;
}

static int isOpposite(Direction a, Direction b)
{
    return (a == DIR_UP && b == DIR_DOWN) ||
           (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

static int directionBetween(Point from, Point to, Direction *out)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (from.x + DX[i] == to.x && from.y + DY[i] == to.y) {
            *out = (Direction)i;
            return 1;
        }
    }
    return 0;
}

static int pathIsSafe(const Body *start, const Point *path, int steps)
{
    static Body virt;
    static Point scratch[MAX_LENGTH];
    int i;

    virt = *start;
    for (i = 0; i < steps; i++)
        bodyStep(&virt, path[i], i == steps - 1);

    if (virt.length >= MAX_LENGTH)
        return 1;

    return findPath(&virt, virt.cells[0], virt.cells[virt.length - 1], scratch) >= 0;
}

static int searchMove(Direction *out)
{
    static Body body;
    static Point path[MAX_LENGTH];
    Direction candidate;
    Direction best = dir;
    int bestRoom = 0;
    int steps;
    int i;

    bodyFromGame(&body);

    steps = findPath(&body, body.cells[0], food, path);
    if (steps > 0 && pathIsSafe(&body, path, steps))
        return directionBetween(body.cells[0], path[0], out);

    steps = findPath(&body, body.cells[0], body.cells[body.length - 1], path);
    if (steps > 0)
        return directionBetween(body.cells[0], path[0], out);

    for (i = 0; i < 4; i++) {
        Point next;
        int room;

        candidate = (Direction)i;
        if (isOpposite(candidate, dir))
            continue;

        next.x = body.cells[0].x + DX[i];
        next.y = body.cells[0].y + DY[i];
        room = reachableCells(&body, next);
        if (room > bestRoom) {
            bestRoom = room;
            best = candidate;
        }
    }

    if (bestRoom > 0) {
        *out = best;
        return 1;
    }
    return 0;
}

static int followsCycle(void)
{
    int tailIndex;
    int previous = -1;
    int i;

    if (!cycleReady)
        return 0;

    tailIndex = cycleIndex[snake[length - 1].y][snake[length - 1].x];

    for (i = length - 1; i >= 0; i--) {
        int rel = (cycleIndex[snake[i].y][snake[i].x] - tailIndex + MAX_LENGTH) % MAX_LENGTH;

        if (rel <= previous)
            return 0;
        previous = rel;
    }

    return 1;
}

static int cycleMove(Direction *out)
{
    int headIndex = cycleIndex[snake[0].y][snake[0].x];
    int tailRel = (cycleIndex[snake[length - 1].y][snake[length - 1].x] - headIndex + MAX_LENGTH) % MAX_LENGTH;
    int foodRel = (cycleIndex[food.y][food.x] - headIndex + MAX_LENGTH) % MAX_LENGTH;
    int bestRel = 0;
    Direction best = dir;
    int i;

    for (i = 0; i < 4; i++) {
        int nx = snake[0].x + DX[i];
        int ny = snake[0].y + DY[i];
        int rel;

        if (!inBounds(nx, ny))
            continue;

        rel = (cycleIndex[ny][nx] - headIndex + MAX_LENGTH) % MAX_LENGTH;
        if (rel < 1 || rel >= tailRel || rel > foodRel)
            continue;

        if (rel > bestRel) {
            bestRel = rel;
            best = (Direction)i;
        }
    }

    if (bestRel > 0) {
        *out = best;
        return 1;
    }

    for (i = 0; i < 4; i++) {
        int nx = snake[0].x + DX[i];
        int ny = snake[0].y + DY[i];

        if (!inBounds(nx, ny))
            continue;
        if ((cycleIndex[ny][nx] - headIndex + MAX_LENGTH) % MAX_LENGTH == 1) {
            *out = (Direction)i;
            return 1;
        }
    }

    return 0;
}

static int autoMove(Direction *out)
{
    if (followsCycle())
        return cycleMove(out);
    return searchMove(out);
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
        case 't':
        case 'T':
            autoPlay = !autoPlay;
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

    if (!inBounds(head.x, head.y)) {
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

static void steerAuto(void)
{
    Direction d;

    dirLocked = 0;
    if (autoMove(&d))
        setDirection(d);
}

static void showIntro(void)
{
    clearScreen();
    printf("\n  SNAKE\n\n");
    printf("  eat the * to grow, do not hit the walls or yourself\n");
    printf("  move with WASD or the arrow keys\n");
    printf("  T lets the computer play, P pauses, Q quits\n\n");
    printf("  press any key to start\n");

    if (_getch() == 224)
        _getch();
    clearScreen();
}

static int askReplay(void)
{
    int key;

    printf("\n  %s  final score %d\n", won ? "board full, perfect game!" : "game over", score);
    printf("  press R to play again, any other key to quit\n");

    while (_kbhit())
        _getch();

    key = _getch();
    if (key == 0 || key == 224)
        _getch();

    return key == 'r' || key == 'R';
}

int main(int argc, char **argv)
{
    int playing = 1;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--auto") == 0)
            autoPlay = 1;
    }

    srand((unsigned int)time(NULL));
    SetConsoleTitleA("Snake");
    buildCycle();
    enableColor();
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
            if (autoPlay)
                steerAuto();
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
