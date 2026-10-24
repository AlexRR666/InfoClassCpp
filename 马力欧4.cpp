#include <iostream>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
using namespace std;

const int W = 60;
const int H = 20;
const int MAP_W = 1000;
const int MAP_H = H;
const int GROUND_Y = H - 4; // 16

int playerX = 2;
int playerY = GROUND_Y - 1;
int camX = 0;
int score = 0;
int coinsLeft = 0;
int totalCoins = 0;
int collectedCoins = 0;
int totalMonsters = 0;
int monstersKilled = 0;
bool gameOver = false;
bool win = false;
int winType = 0;
int vy = 0;
bool onGround = true;
bool isEasyMode = false;
bool gameStarted = false;

char map[MAP_H][MAP_W];

struct Monster {
    int x, y;
    int dir;
    bool alive;
    int moveTimer;
    int speed;
};

const int MAX_MON = 50;
Monster monsters[MAX_MON];
int monsterCount = 0;

struct Splat {
    int x, y;
    int timer;
};
const int MAX_SPLAT = 10;
Splat splats[MAX_SPLAT];
int splatCount = 0;

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
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleOutputW(hOut, (CHAR_INFO*)buffer, bufferSize, bufferCoord, &writeRegion);
}

void setCell(int x, int y, char c) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    buffer[y][x].Char.UnicodeChar = c;
    buffer[y][x].Char.AsciiChar = c;
    buffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

void writeText(int x, int y, const char* text) {
    for (int i = 0; text[i] != '\0'; i++)
        setCell(x + i, y, text[i]);
}

void fillRect(char ch, int x1, int y1, int x2, int y2) {
    if (x1 < 0) x1 = 0;
    if (x2 >= MAP_W) x2 = MAP_W - 1;
    if (y1 < 0) y1 = 0;
    if (y2 >= MAP_H) y2 = MAP_H - 1;
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            map[y][x] = ch;
}

void putCoin(int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return;
    if (map[y][x] != ' ') return;
    if (totalCoins >= 110) return;
    
    bool blockedAbove = false;
    for (int checkY = y - 1; checkY >= 0; checkY--) {
        if (map[checkY][x] == '=' || map[checkY][x] == '#') {
            blockedAbove = true;
            break;
        }
    }
    if (blockedAbove) return;

    bool hasGroundBelow = false;
    for (int checkY = y + 1; checkY <= y + 2 && checkY < MAP_H; checkY++) {
        if (map[checkY][x] == '#' || map[checkY][x] == '=') {
            hasGroundBelow = true;
            break;
        }
    }
    if (!hasGroundBelow) return;

    map[y][x] = 'o';
    totalCoins++;
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

void addSplat(int x, int y) {
    if (splatCount >= MAX_SPLAT) return;
    splats[splatCount].x = x;
    splats[splatCount].y = y;
    splats[splatCount].timer = 10;
    splatCount++;
}

void initMap() {
    totalCoins = 0;
    collectedCoins = 0;
    monsterCount = 0;
    totalMonsters = 0;
    monstersKilled = 0;
    splatCount = 0;
    srand((unsigned)time(0));

    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map[y][x] = ' ';
            
    for (int x = 0; x < MAP_W; x++) {
        map[GROUND_Y][x] = '#';
        map[GROUND_Y + 1][x] = '#';
    }
    
    int maxPitWidth = isEasyMode ? 5 : 6;
    int minPitWidth = 3;

    for (int i = 0; i < 25; i++) {
        int pitX = 60 + i * 35 + rand() % 15;
        int pitWidth = minPitWidth + rand() % (maxPitWidth - minPitWidth + 1);
        for (int p = 0; p < pitWidth; p++) {
            if (pitX + p < MAP_W && pitX + p > 15) {
                map[GROUND_Y][pitX + p] = ' ';
                map[GROUND_Y + 1][pitX + p] = ' ';
            }
        }
    }
    fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);

    struct Plat { int x, y, len; };
    Plat plats[200];
    int pCount = 0;

    for (int base = 20; base < MAP_W - 30; base += 25 + rand() % 15) {
        if (pCount >= 198) break;
        int n = 1 + rand() % 2;
        int currentRelY = 2 + rand() % 3;
        int currentX = base;
        
        for (int i = 0; i < n; i++) {
            int len = 6 + rand() % 5;
            int y = GROUND_Y - currentRelY;
            
            char type = (rand() % 2 == 0) ? '#' : '=';
            fillRect(type, currentX, y, currentX + len - 1, y);
            
            plats[pCount].x = currentX;
            plats[pCount].y = y;
            plats[pCount].len = len;
            pCount++;
            
            currentX += len + 3 + rand() % 5;
            if (rand() % 2 == 0) {
                if (currentRelY + 3 <= 8) currentRelY += 2 + rand() % 2;
            } else {
                if (currentRelY - 2 >= 2) currentRelY -= 2 + rand() % 2;
            }
        }
    }

    for (int i = 0; i < pCount; i++) {
        putCoin(plats[i].x + 1, plats[i].y - 1);
        putCoin(plats[i].x + plats[i].len - 2, plats[i].y - 1);
    }
    for (int x = 20; x < MAP_W - 20; x += 8) {
        if (map[GROUND_Y - 1][x] == ' ') putCoin(x, GROUND_Y - 1);
    }

    int targetMonsters = 20 + rand() % 11;
    int placed = 0;

    for (int i = 0; i < pCount && placed < targetMonsters; i++) {
        if (plats[i].len >= 6) {
            int mx = plats[i].x + plats[i].len / 2;
            int spd = (rand() % 2 == 0) ? 3 : 5;
            addMonster(mx, plats[i].y - 1, spd, rand() % 4);
            placed++;
        }
    }

    for (int x = 40; x < MAP_W - 20 && placed < targetMonsters; x += 30) {
        if (map[GROUND_Y][x] == '#' && map[GROUND_Y - 1][x] == ' ') {
            addMonster(x, GROUND_Y - 1, 4, rand() % 4);
            placed++;
        }
    }

    totalMonsters = monsterCount;
    coinsLeft = totalCoins;
}

bool isSolid(int x, int y) {
    if (x < 0 || x >= MAP_W) return true;
    if (y < 0 || y >= MAP_H) return false;
    return map[y][x] == '#';
}

bool isOneWay(int x, int y) {
    if (x < 0 || x >= MAP_W) return false;
    if (y < 0 || y >= MAP_H) return false;
    return map[y][x] == '=';
}

bool keyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool hasGround(int x, int y) {
    return isSolid(x, y + 1) || isOneWay(x, y + 1);
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

void updateSplats() {
    for (int i = 0; i < splatCount; i++) {
        splats[i].timer--;
        if (splats[i].timer <= 0) {
            for (int j = i; j < splatCount - 1; j++) {
                splats[j] = splats[j + 1];
            }
            splatCount--;
            i--;
        }
    }
}

void update() {
    if (gameOver || win) {
        if (keyDown(VK_SPACE)) {
            playerX = 2; playerY = GROUND_Y - 1;
            camX = 0; score = 0; vy = 0;
            onGround = true; gameOver = false; win = false; winType = 0;
            initMap();
        }
        // 死亡或通关后切换难度
        if (keyDown('1')) {
            isEasyMode = true;
            playerX = 2; playerY = GROUND_Y - 1;
            camX = 0; score = 0; vy = 0;
            onGround = true; gameOver = false; win = false; winType = 0;
            initMap();
        } else if (keyDown('2')) {
            isEasyMode = false;
            playerX = 2; playerY = GROUND_Y - 1;
            camX = 0; score = 0; vy = 0;
            onGround = true; gameOver = false; win = false; winType = 0;
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
        if (!isSolid(nx, playerY)) {
            playerX = nx;
            if (onGround && !isSolid(playerX, playerY + 1) && !isOneWay(playerX, playerY + 1)) {
                onGround = false;
                vy = 1;
            }
        }
    }

    static bool jumpPrev = false;
    if (jumpHeld && !jumpPrev && onGround) {
        vy = -5; 
        onGround = false;
    }
    jumpPrev = jumpHeld;

    if (!onGround) {
        if (vy == 0) vy = 1;
        int step = (vy > 0) ? 1 : -1;
        int ny = playerY + step;

        if (step < 0) {
            if (isSolid(playerX, ny)) {
                vy = 0;
            } else {
                playerY = ny;
            }
        } else {
            if (isSolid(playerX, ny) || isOneWay(playerX, ny)) {
                onGround = true;
                vy = 0;
            } else {
                playerY = ny;
            }
        }
        if (!onGround) vy += 1; 
    }

    if (onGround) {
        if (!isSolid(playerX, playerY + 1) && !isOneWay(playerX, playerY + 1)) {
            onGround = false;
            vy = 1;
        }
    }

    if (playerY >= MAP_H - 1 && !isSolid(playerX, playerY + 1) && !isOneWay(playerX, playerY + 1)) {
        gameOver = true;
    }

    if (map[playerY][playerX] == 'o') {
        map[playerY][playerX] = ' ';
        score += 10;
        coinsLeft--;
        collectedCoins++;
    }

    updateMonsters();
    updateSplats();

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];
        
        if (vy > 0) {
            bool isStomp = false;
            
            if (playerY == m.y - 1) {
                if (!isEasyMode) {
                    if (playerX == m.x) isStomp = true;
                } else {
                    bool facingPlayer = (playerX > m.x && m.dir == 1) || (playerX < m.x && m.dir == -1) || (playerX == m.x);
                    if (abs(playerX - m.x) <= 1 && facingPlayer) {
                        isStomp = true;
                    }
                }
            }
            
            if (!isStomp && playerY == m.y && abs(playerX - m.x) == 0) {
                isStomp = true;
                if (isEasyMode) {
                    if (abs(playerX - m.x) <= 1) isStomp = true;
                }
            }

            if (isStomp) {
                m.alive = false;
                score += (15 + rand() % 6);
                monstersKilled++;
                vy = -4;     
                addSplat(m.x, m.y); 
            } else if (m.x == playerX && m.y == playerY) {
                gameOver = true;
            }
        } else {
            if (m.x == playerX && m.y == playerY) {
                gameOver = true;
            }
        }
    }

    if (playerX >= MAP_W - 5) {
        if (score >= 1000) {
            win = true;
            winType = 0; 
        }
        if (coinsLeft == 0) {
            win = true;
            winType = 1; 
        }
        if (coinsLeft == 0 && monstersKilled == totalMonsters) {
            win = true;
            winType = 2; 
        }
    }

    if (playerX - camX > W * 2 / 3) camX = playerX - W * 2 / 3;
    if (camX < 0) camX = 0;
    if (camX > MAP_W - W) camX = MAP_W - W;
}

void drawMenu() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            setCell(x, y, ' ');

    writeText(20, 4, "==============================");
    writeText(24, 6, "SUPER MARIO CONSOLE");
    writeText(20, 8, "==============================");
    
    writeText(15, 11, "1. EASY MODE");
    writeText(20, 12, "- Pit: 3~5 tiles");
    writeText(20, 13, "- Stomp: Wide Area (Auto-aim)");
    
    writeText(15, 15, "2. HARD MODE");
    writeText(20, 16, "- Pit: 3~6 tiles (Extreme)");
    writeText(20, 17, "- Stomp: Precise Head Only");

    writeText(20, 19, "Q to Quit");

    flushBuffer();
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

    for (int i = 0; i < splatCount; i++) {
        int sx = splats[i].x - camX;
        int sy = splats[i].y;
        setCell(sx, sy, '*');
    }

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        int mx = monsters[i].x - camX;
        int my = monsters[i].y;
        setCell(mx, my, 'G');
    }

    int px = playerX - camX;
    setCell(px, playerY, 'M');

    char hud[128];
    sprintf(hud, "[%s] Score: %d/1000 | Coins: %d/%d | Kills: %d/%d", 
            isEasyMode ? "Easy" : "Hard",
            score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
    writeText(2, H - 3, hud);

    if (gameOver) {
        writeText(2, H - 2, "GAME OVER! [SPACE] Restart | [1] Easy | [2] Hard");
    } else if (win) {
        if (winType == 2) {
            writeText(2, H - 2, "COMPLETE! [SPACE] Restart | [1] Easy | [2] Hard");
        } else if (winType == 1) {
            writeText(2, H - 2, "PERFECT!  [SPACE] Restart | [1] Easy | [2] Hard");
        } else {
            writeText(2, H - 2, "WIN!      [SPACE] Restart | [1] Easy | [2] Hard");
        }
    }

    flushBuffer();
}

int main() {
    srand(time(0));
    initConsole();
    initMap(); 
    
    while (!gameStarted) {
        drawMenu();
        if (keyDown('1')) {
            isEasyMode = true;
            gameStarted = true;
        } else if (keyDown('2')) {
            isEasyMode = false;
            gameStarted = true;
        } else if (keyDown('Q')) {
            return 0;
        }
        Sleep(50);
    }

    while (true) {
        update();
        draw();
        Sleep(80);
    }
    return 0;
}
