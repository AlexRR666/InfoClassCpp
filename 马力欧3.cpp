 #include <iostream>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
using namespace std;

const int W = 60;
const int H = 20;
const int MAP_W = 160;
const int MAP_H = H;
const int GROUND_Y = 16;

int playerX = 2;
int playerY = GROUND_Y - 1;
int camX = 0;
int score = 0;
int coinsLeft = 0;
int totalCoins = 0;
bool gameOver = false;
bool win = false;
int vy = 0;
bool onGround = true;

char map[MAP_H][MAP_W];

struct Monster {
    int x, y;
    int dir;
    bool alive;
    int moveTimer;
    int speed;
};

const int MAX_MON = 8;
Monster monsters[MAX_MON];
int monsterCount = 0;

HANDLE hOut;
CHAR_INFO buffer[H][W];
COORD bufferSize = {W, H};
COORD bufferCoord = {0, 0};
SMALL_RECT writeRegion = {0, 0, W - 1, H - 1};

void initConsole() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    cci.dwSize = 1;
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cci);
}

void flushBuffer() {
    WriteConsoleOutput(hOut, (CHAR_INFO*)buffer, bufferSize, bufferCoord, &writeRegion);
}

void setCell(int x, int y, char c) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    buffer[y][x].Char.AsciiChar = c;
    buffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

void writeText(int x, int y, const char* text) {
    for (int i = 0; text[i] != '\0'; i++)
        setCell(x + i, y, text[i]);
}

void fillRect(int x1, int y1, int x2, int y2) {
    if (x1 < 0) x1 = 0;
    if (x2 >= MAP_W) x2 = MAP_W - 1;
    if (y1 < 0) y1 = 0;
    if (y2 >= MAP_H) y2 = MAP_H - 1;
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            map[y][x] = '#';
}

void putCoin(int x, int y) {
    if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
        map[y][x] = 'o';
        totalCoins++;
    }
}

void addMonster(int x, int y, int speed, int phase) {
    if (monsterCount >= MAX_MON) return;
    monsters[monsterCount].x = x;
    monsters[monsterCount].y = y;
    monsters[monsterCount].dir = (phase % 2 == 0) ? 1 : -1;
    monsters[monsterCount].alive = true;
    monsters[monsterCount].moveTimer = phase;
    monsters[monsterCount].speed = speed;
    monsterCount++;
}

void initMap() {
    totalCoins = 0;
    monsterCount = 0;

    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map[y][x] = ' ';

    for (int x = 0; x < MAP_W; x++) {
        bool pit = (x > 22 && x < 28) || (x > 46 && x < 52) ||
                   (x > 72 && x < 78) || (x > 100 && x < 106) ||
                   (x > 130 && x < 136);
        if (!pit) {
            map[GROUND_Y][x] = '#';
            map[GROUND_Y + 1][x] = '#';
        }
    }

    fillRect(0, GROUND_Y, 8, GROUND_Y + 1);

    fillRect(12, 13, 18, 13);
    fillRect(20, 11, 26, 11);
    fillRect(30, 13, 36, 13);

    fillRect(54, 13, 60, 13);
    fillRect(62, 11, 68, 11);
    fillRect(64, 8, 70, 8);

    fillRect(80, 13, 86, 13);
    fillRect(88, 10, 94, 10);
    fillRect(96, 13, 102, 13);

    fillRect(108, 12, 114, 12);
    fillRect(116, 9, 122, 9);
    fillRect(124, 13, 130, 13);

    fillRect(140, 13, 146, 13);
    fillRect(148, 10, 154, 10);

    putCoin(14, 12); putCoin(16, 12);
    putCoin(22, 10); putCoin(24, 10);
    putCoin(32, 12); putCoin(34, 12);

    putCoin(56, 12); putCoin(58, 12);
    putCoin(64, 10); putCoin(66, 7);

    putCoin(82, 12); putCoin(84, 12);
    putCoin(90, 9);  putCoin(92, 9);

    putCoin(110, 11); putCoin(112, 11);
    putCoin(118, 8);  putCoin(120, 8);
    putCoin(126, 12);

    putCoin(142, 12); putCoin(150, 9);

    coinsLeft = totalCoins;

    addMonster(15, 12, 3, 0);
    addMonster(65, 10, 4, 1);
    addMonster(83, 12, 3, 2);
    addMonster(111, 11, 5, 3);
}

bool isSolid(int x, int y) {
    if (x < 0 || x >= MAP_W) return true;
    if (y < 0 || y >= MAP_H) return false;
    return map[y][x] == '#';
}

bool keyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool hasGround(int x, int y) {
    return isSolid(x, y + 1);
}

void updateMonsters() {
    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];

        m.moveTimer++;
        if (m.moveTimer < m.speed) continue;
        m.moveTimer = 0;

        int dist = playerX - m.x;
        if (playerY == m.y && abs(dist) <= 8 && dist != 0) {
            m.dir = (dist > 0) ? 1 : -1;
        }

        int nx = m.x + m.dir;

        if (!hasGround(nx, m.y)) {
            m.dir = -m.dir;
            nx = m.x + m.dir;
            if (!hasGround(nx, m.y)) continue;
        }

        if (isSolid(nx, m.y)) {
            m.dir = -m.dir;
            continue;
        }

        m.x = nx;
    }
}

void update() {
    if (gameOver || win) {
        // 空格重开
        if (keyDown(VK_SPACE)) {
            playerX = 2; playerY = GROUND_Y - 1;
            camX = 0; score = 0; vy = 0;
            onGround = true; gameOver = false; win = false;
            initMap();
        }
        if (keyDown('Q')) exit(0);
        return;
    }

    bool leftHeld  = keyDown('A') || keyDown(VK_LEFT);
    bool rightHeld = keyDown('D') || keyDown(VK_RIGHT);
    bool jumpHeld  = keyDown('W') || keyDown(VK_SPACE) || keyDown(VK_UP);

    if (keyDown('Q')) exit(0);

    int dx = 0;
    if (leftHeld && !rightHeld) dx = -1;
    if (rightHeld && !leftHeld) dx = 1;
    if (dx != 0) {
        int nx = playerX + dx;
        if (!isSolid(nx, playerY)) playerX = nx;
    }

    static bool jumpPrev = false;
    if (jumpHeld && !jumpPrev && onGround) {
        vy = -5;
        onGround = false;
    }
    jumpPrev = jumpHeld;

    if (!onGround) {
        int step = (vy > 0) ? 1 : -1;
        int ny = playerY + step;
        if (isSolid(playerX, ny)) {
            if (step > 0) onGround = true;
            vy = 0;
        } else {
            playerY = ny;
        }
        if (!onGround) vy += 1;
    }

    if (isSolid(playerX, playerY + 1)) {
        onGround = true;
        if (vy > 0) vy = 0;
    } else {
        onGround = false;
    }

    if (playerY >= MAP_H - 1 && !isSolid(playerX, playerY + 1)) {
        gameOver = true;
    }

    if (map[playerY][playerX] == 'o') {
        map[playerY][playerX] = ' ';
        score += 10;
        coinsLeft--;
    }

    updateMonsters();

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        if (monsters[i].x == playerX && monsters[i].y == playerY) {
            gameOver = true;
        }
    }

    if (playerX >= MAP_W - 3 && coinsLeft == 0) {
        win = true;
    }

    if (playerX - camX > W * 2 / 3) camX = playerX - W * 2 / 3;
    if (camX < 0) camX = 0;
    if (camX > MAP_W - W) camX = MAP_W - W;
}

void draw() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            setCell(x, y, ' ');

    for (int x = 0; x < W; x++)
        setCell(x, H - 1, '-');

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int wx = x + camX;
            if (wx < 0 || wx >= MAP_W) continue;
            char c = map[y][wx];
            if (c != ' ') setCell(x, y, c);
        }
    }

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        int mx = monsters[i].x - camX;
        int my = monsters[i].y;
        setCell(mx, my, 'G');
    }

    int px = playerX - camX;
    setCell(px, playerY, 'M');

    // 屏幕内提示
    if (gameOver) {
        writeText(2, H - 3, "GAME OVER! Press SPACE to restart, Q to quit");
    } else if (win) {
        writeText(2, H - 3, "YOU WIN!  Press SPACE to restart, Q to quit");
    }

    char title[256];
    sprintf(title, "Score: %d | Coins left: %d/%d | A/D move, W/Space jump, Q quit",
            score, coinsLeft, totalCoins);
    SetConsoleTitle(title);

    flushBuffer();
}

int main() {
    srand(time(0));
    initConsole();
    initMap();
    while (true) {
        update();
        draw();
        Sleep(80);
    }
    return 0;
}。 
