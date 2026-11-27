#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <windows.h>
#include <conio.h>

using namespace std;

/* ========== 游戏参数 ========== */

const int W = 60;
const int H = 28;
const int PADDLE_W = 9;
const int BRICK_ROWS = 5;
const int BRICK_COLS = 20;
const int BRICK_TOP = 3;
const int MAX_BALLS = 30;

/* ========== 球结构 ========== */

struct Ball {
    int x, y;
    int dx, dy;
    bool alive;
    Ball() : x(0), y(0), dx(0), dy(0), alive(false) {}
};

/* ========== 全局状态 ========== */

char board[H][W];

int paddleX;

Ball balls[MAX_BALLS];
int ballCount = 0;

bool bricks[BRICK_ROWS][BRICK_COLS];
int bricksLeft = 0;
int score = 0;
int lives = 3;

int rowScore[BRICK_ROWS] = {1, 2, 3, 5, 8};

bool gameOver = false;
bool win = false;
bool paused = false;

/* ========== 双缓冲控制台 ========== */

HANDLE hOut;
CHAR_INFO* buffer = NULL;
int screenW, screenH;

void initConsole() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD size = { (SHORT)(W + 4), (SHORT)(H + 8) };
    SetConsoleScreenBufferSize(hOut, size);

    SMALL_RECT rect = { 0, 0, (SHORT)(W + 3), (SHORT)(H + 7) };
    SetConsoleWindowInfo(hOut, TRUE, &rect);

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hOut, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &ci);

    screenW = W + 4;
    screenH = H + 8;

    buffer = new CHAR_INFO[screenW * screenH];
}

void clearBuffer() {
    for (int i = 0; i < screenW * screenH; i++) {
        buffer[i].Char.AsciiChar = ' ';
        buffer[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

void setChar(int x, int y, char ch, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
    if (x < 0 || x >= screenW || y < 0 || y >= screenH) return;
    buffer[y * screenW + x].Char.AsciiChar = ch;
    buffer[y * screenW + x].Attributes = color;
}

void setText(int x, int y, const string& s, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
    for (size_t i = 0; i < s.size(); i++) {
        setChar(x + (int)i, y, s[i], color);
    }
}

void present() {
    COORD bufferSize = { (SHORT)screenW, (SHORT)screenH };
    COORD bufferCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(screenW - 1), (SHORT)(screenH - 1) };

    WriteConsoleOutput(hOut, buffer, bufferSize, bufferCoord, &writeRegion);
}

/* ========== 球管理 ========== */

void addBall(int x, int y, int dx, int dy) {
    if (ballCount >= MAX_BALLS) return;
    balls[ballCount].x = x;
    balls[ballCount].y = y;
    balls[ballCount].dx = dx;
    balls[ballCount].dy = dy;
    balls[ballCount].alive = true;
    ballCount++;
}

void removeDeadBalls() {
    int j = 0;
    for (int i = 0; i < ballCount; i++) {
        if (balls[i].alive) {
            balls[j++] = balls[i];
        }
    }
    ballCount = j;
}

/* ========== 初始化 ========== */

void initBricks() {
    bricksLeft = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
        for (int c = 0; c < BRICK_COLS; c++) {
            bricks[r][c] = true;
            bricksLeft++;
        }
}

void resetBall() {
    paddleX = W / 2 - PADDLE_W / 2;
    ballCount = 0;
    addBall(paddleX + PADDLE_W / 2, H - 3,
            (rand() % 2 == 0) ? 1 : -1, -1);
    paused = true;
}

void newGame() {
    score = 0;
    lives = 3;
    gameOver = false;
    win = false;
    initBricks();
    resetBall();
}

/* ========== 逻辑 ========== */

void update() {
    if (gameOver || win || paused) return;

    for (int i = 0; i < ballCount; i++) {
        Ball& b = balls[i];
        if (!b.alive) continue;

        int steps = max(abs(b.dx), abs(b.dy));
        if (steps < 1) steps = 1;

        for (int s = 0; s < steps; s++) {
            int stepX = (b.dx > 0) ? 1 : (b.dx < 0 ? -1 : 0);
            int stepY = (b.dy > 0) ? 1 : (b.dy < 0 ? -1 : 0);

            int oldX = b.x;
            int oldY = b.y;

            b.x += stepX;
            b.y += stepY;

            // 左右墙反弹
            if (b.x <= 0) { b.x = 0; b.dx = 1; }
            if (b.x >= W - 1) { b.x = W - 1; b.dx = -1; }

            // 顶部反弹
            if (b.y <= 0) { b.y = 0; b.dy = 1; }

            // 挡板反弹：只有球向下飞，且落在挡板上表面时才反弹
            if (b.dy > 0 && b.y == H - 2 &&
                b.x >= paddleX && b.x < paddleX + PADDLE_W) {
                b.y = H - 3;
                b.dy = -1;
                int hit = b.x - paddleX;
                if (hit < PADDLE_W / 3) b.dx = -1;
                else if (hit > PADDLE_W * 2 / 3) b.dx = 1;
            }

            // 掉底
            if (b.y >= H - 1) {
                b.alive = false;
                break;
            }

            // 打砖块
            int br = b.y - BRICK_TOP;
            int bc = (b.x >= 1) ? (b.x - 1) / 3 : -1;

            if (br >= 0 && br < BRICK_ROWS && bc >= 0 && bc < BRICK_COLS) {
                if (bricks[br][bc]) {
                    bricks[br][bc] = false;
                    bricksLeft--;
                    score += rowScore[br];

                    // 球退回撞砖前的位置，绝对不留在砖块内部
                    b.x = oldX;
                    b.y = oldY;

                    // dx 不转，只反转 dy
                    b.dy = -b.dy;

                    // 第1行40%、第2行25%、第3行10%
                    int chance = 0;
                    if (br == 0) chance = 40;
                    else if (br == 1) chance = 25;
                    else if (br == 2) chance = 10;

                    if (chance > 0 && (rand() % 100) < chance) {
                        int ndx = (rand() % 2 == 0) ? 1 : -1;
                        int ndy = (rand() % 2 == 0) ? 1 : -1;
                        addBall(oldX, oldY, ndx, ndy);
                    }

                    if (bricksLeft == 0) win = true;
                    break;
                }
            }
        }
    }

    removeDeadBalls();

    // 所有球都掉光才扣命
    if (ballCount == 0) {
        lives--;
        if (lives <= 0) gameOver = true;
        else resetBall();
    }
}

/* ========== 渲染 ========== */

void render() {
    clearBuffer();

    // 顶部边框
    setText(0, 0, "+", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    for (int x = 1; x <= W; x++) setText(x, 0, "-", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    setText(W + 1, 0, "+", FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    // 两侧边框
    for (int y = 1; y <= H; y++) {
        setText(0, y, "|", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        setText(W + 1, y, "|", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    // 底部边框
    setText(0, H + 1, "+", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    for (int x = 1; x <= W; x++) setText(x, H + 1, "-", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    setText(W + 1, H + 1, "+", FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    // 砖块
    WORD brickColors[5] = {
        FOREGROUND_RED | FOREGROUND_INTENSITY,
        FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    };

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c]) {
                int y = BRICK_TOP + r + 1;
                int x = c * 3 + 1;
                setChar(x, y, '[', brickColors[r]);
                setChar(x + 1, y, '=', brickColors[r]);
                setChar(x + 2, y, ']', brickColors[r]);
            }
        }
    }

    // 挡板
    for (int i = 0; i < PADDLE_W; i++) {
        int x = paddleX + i + 1;
        int y = H - 2 + 1;
        if (x >= 1 && x <= W) {
            setChar(x, y, '=', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
    }

    // 所有球
    for (int i = 0; i < ballCount; i++) {
        if (balls[i].alive) {
            setChar(balls[i].x + 1, balls[i].y + 1, 'O',
                    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        }
    }

    // 信息栏
    string info = "得分：" + to_string(score) + "   生命：" + to_string(lives)
                + "   剩余砖块：" + to_string(bricksLeft)
                + "   球数：" + to_string(ballCount);
    setText(0, H + 3, info);

    string hint;
    if (win)           hint = "*** 通关！按 R 重开，Q 退出 ***";
    else if (gameOver) hint = "*** 游戏结束！按 R 重开，Q 退出 ***";
    else if (paused)   hint = "按 空格 发球，←→ 移动挡板，P 暂停，Q 退出";
    else               hint = "←→ 移动挡板，P 暂停，R 重开，Q 退出";

    setText(0, H + 4, hint, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    present();
}

/* ========== 主循环 ========== */

int main() {
    system("title 弹珠子");
    srand((unsigned)time(0));

    initConsole();
    newGame();

    DWORD lastMove = GetTickCount();

    while (true) {
        render();

        if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
            if (!paused && !gameOver && !win) {
                if (paddleX > 0) paddleX -= 1;
            }
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            if (!paused && !gameOver && !win) {
                if (paddleX + PADDLE_W < W) paddleX += 1;
            }
        }

        if (_kbhit()) {
            int key = _getch();

            if (key == 0 || key == 224) {
                _getch();
                continue;
            }

            key = tolower(key);

            if (key == 'q') break;
            if (key == 'r') { newGame(); continue; }
            if (key == 'p') {
                if (!gameOver && !win) paused = !paused;
                continue;
            }
            if (key == ' ') {
                if (!gameOver && !win) paused = false;
                continue;
            }
        }

        DWORD now = GetTickCount();
        if (now - lastMove >= 16) {
            update();
            lastMove = now;
        }

        Sleep(45);
    }

    delete[] buffer;
    return 0;
}
