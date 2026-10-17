#include <iostream>
#include <windows.h>
#include <ctime>
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
int coinsLeft = 0;          // 剩余金币数
int totalCoins = 0;         // 总金币数
bool gameOver = false;
bool win = false;
int vy = 0;
bool onGround = true;

char map[MAP_H][MAP_W];
char screen[H][W + 1];

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

void initMap() {
    totalCoins = 0;
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
}

void clearScreen() { system("cls"); }

void draw() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            screen[y][x] = ' ';
    for (int x = 0; x < W; x++)
        screen[H - 1][x] = '-';

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int wx = x + camX;
            if (wx < 0 || wx >= MAP_W) continue;
            char c = map[y][wx];
            if (c != ' ') screen[y][x] = c;
        }
    }

    int px = playerX - camX;
    if (px >= 0 && px < W && playerY >= 0 && playerY < H)
        screen[playerY][px] = 'M';

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) cout << screen[y][x];
        cout << '\n';
    }
    cout << "Score: " << score
         << "  |  Coins left: " << coinsLeft << "/" << totalCoins
         << "  |  A/D移动  W/空格跳跃  Q退出\n";
    if (gameOver) cout << "===== GAME OVER! 按 R 重开 =====\n";
    if (win)      cout << "===== YOU WIN! 金币全收 + 到达右边缘 =====\n";
}

bool isSolid(int x, int y) {
    if (x < 0 || x >= MAP_W) return true;
    if (y < 0 || y >= MAP_H) return false;
    return map[y][x] == '#';
}

bool keyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

void update() {
    if (gameOver || win) {
        if (keyDown('R')) {
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
    if (leftHeld && !rightHeld) dx = -2;
    if (rightHeld && !leftHeld) dx = 2;
    if (dx != 0) {
        int step = (dx > 0) ? 1 : -1;
        for (int i = 0; i < abs(dx); i++) {
            int nx = playerX + step;
            if (!isSolid(nx, playerY)) playerX = nx;
            else break;
        }
    }

    static bool jumpPrev = false;
    if (jumpHeld && !jumpPrev && onGround) {
        vy = -6;
        onGround = false;
    }
    jumpPrev = jumpHeld;

    if (!onGround) {
        int step = (vy > 0) ? 1 : -1;
        int remain = abs(vy);
        while (remain > 0) {
            int ny = playerY + step;
            if (isSolid(playerX, ny)) {
                if (step > 0) onGround = true;
                vy = 0;
                break;
            }
            playerY = ny;
            remain--;
        }
        if (!onGround) vy += 2;
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

    // 通关：走到右边缘 且 金币全吃完
    if (playerX >= MAP_W - 3 && coinsLeft == 0) {
        win = true;
    }

    if (playerX - camX > W * 2 / 3) camX = playerX - W * 2 / 3;
    if (camX < 0) camX = 0;
    if (camX > MAP_W - W) camX = MAP_W - W;
}

int main() {
    srand(time(0));
    initMap();
    while (true) {
        clearScreen();
        update();
        draw();
        Sleep(60);
    }
    return 0;
}
