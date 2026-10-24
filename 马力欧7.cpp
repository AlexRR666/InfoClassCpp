#include <iostream>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace std;

const int W = 60;
const int H = 20;
const int MAP_W = 500;
const int MAP_H = H;
const int GROUND_Y = H - 4;

// P1 (马里奥 - 红色)
int p1X = 2, p1Y = GROUND_Y - 1, p1Vy = 0;
bool p1OnGround = true, p1Alive = true;
int p1Coins = 0, p1RespawnTimer = 0, p1Coyote = 0, p1Inv = 0;
int p1Score = 0;

// P2 (路易吉 - 绿色)
int p2X = 4, p2Y = GROUND_Y - 1, p2Vy = 0;
bool p2OnGround = true, p2Alive = true;
int p2Coins = 0, p2RespawnTimer = 0, p2Coyote = 0, p2Inv = 0;
int p2Score = 0;

bool p1JumpPrev = false, p2JumpPrev = false;

int camX = 0;
int score = 0;
int totalCoins = 0;
int collectedCoins = 0;
int totalMonsters = 0;
int monstersKilled = 0;
bool gameOver = false;
bool win = false;
int winType = 0;

bool isEasyMode = false;
bool isIslandMode = false;
bool is2PMode = false;
bool isSpeedMode = false;
bool isDarkMode = false;
bool isMoonMode = false;

bool gameStarted = false;
int gameTicks = 0;
int timeLeft = 0;

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

void initConsole() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    cci.dwSize = 1;
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cci);

    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= 0x0004; 
    SetConsoleMode(hOut, dwMode);
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

void putCoin(int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return;
    if (map[y][x] != ' ') return;
    if (totalCoins >= 100) return;
    if (map[y - 1][x] == '=' || map[y - 1][x] == '#') return;

    int supports = 0;
    if (isSolid(x, y + 1) || isOneWay(x, y + 1)) supports++;
    if (isSolid(x, y + 2) || isOneWay(x, y + 2)) supports++;
    if (isSolid(x - 1, y + 1) || isOneWay(x - 1, y + 1)) supports++;
    if (isSolid(x - 1, y + 2) || isOneWay(x - 1, y + 2)) supports++;
    if (isSolid(x + 1, y + 1) || isOneWay(x + 1, y + 1)) supports++;
    if (isSolid(x + 1, y + 2) || isOneWay(x + 1, y + 2)) supports++;

    if (supports < 2) return;
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

bool keyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool hasGround(int x, int y) {
    return isSolid(x, y + 1) || isOneWay(x, y + 1);
}

void initMap() {
    totalCoins = 0;
    collectedCoins = 0;
    monsterCount = 0;
    totalMonsters = 0;
    monstersKilled = 0;
    splatCount = 0;
    gameTicks = 0;
    srand((unsigned)time(0));

    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map[y][x] = ' ';

    if (isIslandMode) {
        fillRect('#', 0, GROUND_Y, 12, GROUND_Y + 1); 
        int currentX = 20;
        int minGap = isEasyMode ? 3 : 4;
        int maxGap = isEasyMode ? 6 : 8;
        int currentY = GROUND_Y - 2; 

        while (currentX < MAP_W - 20) {
            int width = 6 + rand() % 4;
            int depth = rand() % 2;
            for (int i = 0; i < width; i++) {
                map[currentY][currentX + i] = '#';
                for (int d = 1; d <= depth; d++) {
                    if (currentY + d < MAP_H) map[currentY + d][currentX + i] = '#';
                }
            }
            if (width >= 6) {
                putCoin(currentX + width / 2, currentY - 1);
                if (width >= 8) putCoin(currentX + width / 2 + 1, currentY - 1);
            }
            int gap = minGap + rand() % (maxGap - minGap + 1);
            currentY += (rand() % 3 - 1);
            if (currentY > GROUND_Y - 1) currentY = GROUND_Y - 1;
            if (currentY < GROUND_Y - 5) currentY = GROUND_Y - 5;
            currentX += width + gap;
        }
    } else {
        for (int x = 0; x < MAP_W; x++) {
            map[GROUND_Y][x] = '#';
            map[GROUND_Y + 1][x] = '#';
        }
        fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);

        int base = 20;
        while (base < MAP_W - 40) {
            int type = rand() % 5;
            if (type == 0) { 
                for (int i = 0; i < 3; i++) {
                    fillRect('#', base + i, GROUND_Y - 1 - i, base + 5 - i, GROUND_Y - 1 - i);
                    putCoin(base + i, GROUND_Y - 2 - i);
                    putCoin(base + 5 - i, GROUND_Y - 2 - i);
                }
                for (int i = 1; i <= 3; i++) {
                    fillRect('#', base + 5 + i, GROUND_Y - 4 + i, base + 8 - i, GROUND_Y - 4 + i);
                    putCoin(base + 5 + i, GROUND_Y - 5 + i);
                    putCoin(base + 8 - i, GROUND_Y - 5 + i);
                }
                putCoin(base + 4, GROUND_Y - 5);
                base += 15;
            } else if (type == 1) { 
                fillRect('#', base, GROUND_Y - 3, base + 1, GROUND_Y);
                base += 10;
            } else if (type == 2) { 
                for (int i = 0; i < 4; i++) {
                    fillRect('#', base + i, GROUND_Y - i - 1, base + i, GROUND_Y);
                    putCoin(base + i, GROUND_Y - i - 2);
                }
                int pitWidth = isEasyMode ? 4 : 5;
                for (int i = 0; i < pitWidth; i++) { map[GROUND_Y][base + 4 + i] = ' '; map[GROUND_Y + 1][base + 4 + i] = ' '; }
                for (int i = 0; i < 4; i++) {
                    fillRect('#', base + 4 + pitWidth + i, GROUND_Y - 4 + i, base + 4 + pitWidth + i, GROUND_Y);
                    putCoin(base + 4 + pitWidth + i, GROUND_Y - 5 + i);
                }
                base += 20;
            } else if (type == 3) { 
                int pitWidth = 12;
                for (int i = 0; i < pitWidth; i++) { map[GROUND_Y][base + i] = ' '; map[GROUND_Y + 1][base + i] = ' '; }
                fillRect('=', base + 1, GROUND_Y - 3, base + 4, GROUND_Y - 3);
                fillRect('=', base + 6, GROUND_Y - 4, base + 9, GROUND_Y - 4);
                putCoin(base + 2, GROUND_Y - 4);
                putCoin(base + 7, GROUND_Y - 5);
                base += 18;
            } else { 
                int maxPit = isEasyMode ? 5 : 6;
                int pitWidth = 3 + rand() % (maxPit - 3 + 1);
                for (int p = 0; p < pitWidth; p++) {
                    if (base + p < MAP_W) { map[GROUND_Y][base + p] = ' '; map[GROUND_Y + 1][base + p] = ' '; }
                }
                base += pitWidth + 5;
            }
        }
    }

    struct Plat { int x, y, len; };
    Plat plats[200];
    int pCount = 0;

    for (int base = 30; base < MAP_W - 30; base += 35 + rand() % 20) {
        if (pCount >= 198) break;
        int len = 4 + rand() % 4;
        int y = GROUND_Y - (2 + rand() % 4);
        char type = (rand() % 2 == 0) ? '#' : '=';
        fillRect(type, base, y, base + len - 1, y);
        plats[pCount].x = base;
        plats[pCount].y = y;
        plats[pCount].len = len;
        pCount++;
    }

    for (int i = 0; i < pCount; i++) {
        putCoin(plats[i].x + 1, plats[i].y - 1);
        putCoin(plats[i].x + plats[i].len - 2, plats[i].y - 1);
    }
    for (int x = 20; x < MAP_W - 20; x += 5) {
        if (map[GROUND_Y - 1][x] == ' ') putCoin(x, GROUND_Y - 1);
    }

    int attempts = 0;
    while (totalCoins < 100 && attempts < 1000) {
        int rx = 20 + rand() % (MAP_W - 40);
        int ry = GROUND_Y - 1;
        putCoin(rx, ry);
        attempts++;
    }

    int targetMonsters = 20 + rand() % 11;
    int placed = 0;
    for (int i = 0; i < pCount && placed < targetMonsters; i++) {
        if (plats[i].len >= 4) {
            int mx = plats[i].x + plats[i].len / 2;
            int spd = (rand() % 2 == 0) ? 3 : 5;
            int my = isMoonMode ? (GROUND_Y - 7 + rand() % 4) : (plats[i].y - 1);
            addMonster(mx, my, spd, rand() % 4);
            placed++;
        }
    }
    for (int x = 40; x < MAP_W - 20 && placed < targetMonsters; x += 20) {
        if (map[GROUND_Y][x] == '#' && map[GROUND_Y - 1][x] == ' ') {
            int my = isMoonMode ? (GROUND_Y - 7 + rand() % 4) : (GROUND_Y - 1);
            addMonster(x, my, 4, rand() % 4);
            placed++;
        }
    }
    totalMonsters = monsterCount;
}

void updateMonsters() {
    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];
        m.moveTimer++;
        if (m.moveTimer < m.speed) continue;
        m.moveTimer = 0;

        if (isMoonMode) {
            m.dir = (p1X > m.x) ? 1 : -1;
            m.x += m.dir;
        } else {
            int dist = p1X - m.x;
            if (hasGround(m.x, m.y) && abs(dist) <= 8 && dist != 0) {
                m.dir = (dist > 0) ? 1 : -1;
            }
            int nx = m.x + m.dir;
            if (!hasGround(nx, m.y) || isSolid(nx, m.y)) {
                m.dir = -m.dir;
                nx = m.x + m.dir;
                if (!hasGround(nx, m.y) || isSolid(nx, m.y)) continue;
            }
            m.x = nx;
        }
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

void updatePlayer(int &px, int &py, int &vy, bool &onGround, bool &alive, int &respawn, int &coins, int &pScore, int &inv, bool isP1) {
    if (!alive) {
        respawn--;
        if (respawn <= 0) {
            alive = true;
            px = (isP1 ? 2 : 6);
            py = 2; 
            vy = 0;
            if (isP1) p1Coyote = 0; else p2Coyote = 0;
        }
        return;
    }

    if (inv > 0) inv--;

    int &coyote = isP1 ? p1Coyote : p2Coyote;
    bool left = isP1 ? (keyDown('A') || (!is2PMode && keyDown(VK_LEFT))) : (keyDown(VK_LEFT));
    bool right = isP1 ? (keyDown('D') || (!is2PMode && keyDown(VK_RIGHT))) : (keyDown(VK_RIGHT));
    bool jump = isP1 ? (keyDown('W') || keyDown(VK_SPACE) || (!is2PMode && keyDown(VK_UP))) : (keyDown(VK_UP));

    int moveSpeed = isSpeedMode ? 2 : 1;

    int dx = 0;
    if (left && !right) dx = -moveSpeed;
    if (right && !left) dx = moveSpeed;
    if (dx != 0) {
        int step = (dx > 0) ? 1 : -1;
        for (int i = 0; i < moveSpeed; i++) {
            if (!isSolid(px + step, py)) px += step;
            else break;
        }
        if (onGround && !isSolid(px, py + 1) && !isOneWay(px, py + 1)) {
            onGround = false;
            vy = 1;
        }
    }

    if (onGround) coyote = 0;
    else if (coyote < 10) coyote++;

    bool &jumpPrev = isP1 ? p1JumpPrev : p2JumpPrev;

    if (jump && !jumpPrev && (onGround || coyote <= 2)) {
        vy = isSpeedMode ? -7 : (isMoonMode ? -8 : -5);
        onGround = false;
        coyote = 10;
    }
    jumpPrev = jump;

    if (!onGround) {
        if (vy == 0) vy = 1;
        int step = (vy > 0) ? 1 : -1;
        int ny = py + step;
        
        if (ny <= 0) {
            py = 0;
            vy = 0;
        } else if (step < 0) {
            if (isSolid(px, ny)) vy = 0;
            else py = ny;
        } else {
            if (isSolid(px, ny) || isOneWay(px, ny)) {
                onGround = true;
                vy = 0;
            } else py = ny;
        }

        if (!onGround) {
            if (isMoonMode) {
                if (gameTicks % 2 == 0) vy += 1; 
            } else {
                vy += 1;
            }
        }
    }

    if (onGround && !isSolid(px, py + 1) && !isOneWay(px, py + 1)) {
        onGround = false;
        vy = 1;
    }

    // 【核心修改】掉坑扣200分，无敌复活！
    if (py >= MAP_H - 1 && !isSolid(px, py + 1) && !isOneWay(px, py + 1)) {
        if (gameTicks > 10) {
            if (!is2PMode && pScore >= 200) {
                pScore -= 200;
                px = max(2, px - 5); 
                py = 2;              
                vy = 0;
                inv = 60;            
                return;              
            }
            alive = false;
            respawn = 62;
            if (!is2PMode) gameOver = true; 
        }
    }

    if (map[py][px] == 'o') {
        map[py][px] = ' ';
        coins++;
        collectedCoins++;
        pScore += 10; 
        if (isSpeedMode) timeLeft += 36;
    }

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];
        
        if (m.x == px && m.y == py && gameTicks > 10 && inv == 0) {
            if (pScore >= 100) {
                pScore -= 100; 
                inv = 60;      
                vy = -5;       
            } else {
                alive = false; 
                respawn = 62;
                if (!is2PMode) gameOver = true;
                return;
            }
        }
        
        if (vy > 0) {
            bool isStomp = false;
            if (py == m.y - 1 || (isMoonMode && abs(py - m.y) <= 1)) { 
                if (!isEasyMode) {
                    if (px == m.x) isStomp = true;
                } else {
                    if (abs(px - m.x) <= 1) isStomp = true;
                }
            }
            if (isStomp) {
                m.alive = false;
                monstersKilled++;
                pScore += (15 + rand() % 6); 
                addSplat(m.x, m.y);         
                vy = -4;     
                if (isSpeedMode) timeLeft += 60;
            }
        }
    }
}

void resetGame() {
    initMap(); 
    for (int y = 0; y < GROUND_Y; y++) {
        for (int x = 0; x < 12; x++) map[y][x] = ' ';
    }

    if (isSpeedMode) timeLeft = 60 * 12;
    else timeLeft = 0;

    p1X = 2; p1Y = 2; p1Vy = 0; p1OnGround = false; p1Alive = true; p1RespawnTimer = 0; p1Coins = 0; p1Coyote = 0; p1Score = 0; p1Inv = 0;
    p2X = 3; p2Y = 2; p2Vy = 0; p2OnGround = false; p2Alive = true; p2RespawnTimer = 0; p2Coins = 0; p2Coyote = 0; p2Score = 0; p2Inv = 0;
    
    p1JumpPrev = false; 
    p2JumpPrev = false;
    camX = 0; score = 0; gameOver = false; win = false; winType = 0;
}

void update() {
    if (gameOver || win) {
        if (keyDown(VK_SPACE)) { resetGame(); }
        if (keyDown('1')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('2')) { isEasyMode = false; isIslandMode = false; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('3')) { isEasyMode = true; isIslandMode = true; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('4')) { isEasyMode = false; isIslandMode = true; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('5')) { isEasyMode = true; isIslandMode = false; is2PMode = true; isSpeedMode = isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('6')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = true; isDarkMode = isMoonMode = false; resetGame(); }
        else if (keyDown('7')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = false; isDarkMode = true; isMoonMode = false; resetGame(); }
        else if (keyDown('8')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = false; isDarkMode = false; isMoonMode = true; resetGame(); }
        if (keyDown('Q')) exit(0);
        return;
    }

    gameTicks++;

    if (isSpeedMode) {
        timeLeft--;
        if (timeLeft <= 0) {
            gameOver = true; 
            return;
        }
    }

    if (!is2PMode) score = p1Score; 

    updatePlayer(p1X, p1Y, p1Vy, p1OnGround, p1Alive, p1RespawnTimer, p1Coins, p1Score, p1Inv, true);
    if (is2PMode) {
        updatePlayer(p2X, p2Y, p2Vy, p2OnGround, p2Alive, p2RespawnTimer, p2Coins, p2Score, p2Inv, false);
        
        if (p1Score >= 700) { win = true; winType = 3; return; }
        if (p2Score >= 700) { win = true; winType = 4; return; }
        if (!p1Alive && !p2Alive) { gameOver = true; return; }
    }

    updateMonsters();
    updateSplats(); 

    if (is2PMode) {
        int targetX = (p1Coins <= p2Coins) ? p1X : p2X;
        if (targetX - camX > W * 2 / 3) camX = targetX - W * 2 / 3;
        if (targetX - camX < W / 3) camX = targetX - W / 3;
        if (camX < 0) camX = 0;
        if (camX > MAP_W - W) camX = MAP_W - W;

        int leftEdge = camX + 1;
        int rightEdge = camX + W - 2;
        if (p1X < leftEdge && (isSolid(leftEdge, p1Y + 1) || isOneWay(leftEdge, p1Y + 1))) p1X = leftEdge;
        if (p1X > rightEdge && (isSolid(rightEdge, p1Y + 1) || isOneWay(rightEdge, p1Y + 1))) p1X = rightEdge;
        if (p2X < leftEdge && (isSolid(leftEdge, p2Y + 1) || isOneWay(leftEdge, p2Y + 1))) p2X = leftEdge;
        if (p2X > rightEdge && (isSolid(rightEdge, p2Y + 1) || isOneWay(rightEdge, p2Y + 1))) p2X = rightEdge;
    } else {
        if (p1X - camX > W * 2 / 3) camX = p1X - W * 2 / 3;
        if (camX < 0) camX = 0;
        if (camX > MAP_W - W) camX = MAP_W - W;

        if (p1X >= MAP_W - 5) {
            if (isSpeedMode) {
                win = true; winType = 6; 
            } else {
                if (score >= 800) { win = true; winType = 0; }
                if (collectedCoins >= totalCoins) { win = true; winType = 1; }
                if (collectedCoins >= totalCoins && monstersKilled == totalMonsters) { win = true; winType = 2; }
            }
        }
    }
}

void drawMenu() {
    string screen = "\033[2J\033[H";
    screen += "\n\n";
    screen += "              ==============================\n";
    screen += "                       SUPER MARIO\n";
    screen += "              ==============================\n\n";
    screen += "                1. Normal-Easy    2. Normal-Hard\n";
    screen += "                3. Island-Easy    4. Island-Hard\n";
    screen += "                5. 2-Player Battle\n";
    screen += "                6. Speed Rush (Fast)\n";
    screen += "                7. Dark Mode (Limited Vision)\n";
    screen += "                8. Moon Gravity (Flying Monsters)\n\n";
    screen += "                       Q to Quit\n";
    cout << screen;
}

void drawGameOverScreen() {
    string screen = "\033[2J\033[H"; 
    screen += "\033[38;2;255;255;255m";
    
    string title = gameOver ? "\033[38;2;255;50;50m      G A M E    O V E R      \033[0m" 
                            : (winType == 0 ? "\033[38;2;0;255;0m         Y O U  W I N         \033[0m" : 
                              (winType == 1 ? "\033[38;2;255;215;0m        P E R F E C T !        \033[0m" : "\033[38;2;255;215;0m      C O M P L E T E !       \033[0m"));
    
    if (is2PMode && win) {
        if (winType == 3) title = "\033[38;2;255;0;0m       M A R I O  W I N       \033[0m";
        else if (winType == 4) title = "\033[38;2;0;255;0m       L U I G I  W I N       \033[0m";
    } else if (winType == 6) {
        title = "\033[38;2;255;215;0m       S P E E D  W I N       \033[0m";
    }

    string modeName = is2PMode ? "2-Player Battle" : (isIslandMode ? "Island" : "Normal");
    if (isSpeedMode) modeName = "Speed Rush";
    if (isDarkMode) modeName = "Dark Mode";
    if (isMoonMode) modeName = "Moon Gravity";

    screen += "\n\n\n";
    screen += "              ==============================\n";
    screen += "              " + title + "\n";
    screen += "              ==============================\n\n";
    
    char stats[128];
    if (is2PMode) {
        sprintf(stats, "              Mario: %d pts  |  Luigi: %d pts\n              Total Coins: %d/%d\n", p1Score, p2Score, collectedCoins, totalCoins);
    } else {
        sprintf(stats, "                 Mode: [%s]\n                 Score: %d | Coins: %d/%d\n", modeName.c_str(), score, collectedCoins, totalCoins);
    }
    screen += stats;
    
    screen += "\n              \033[38;2;0;255;0m[SPACE]\033[0m Restart (Current Mode)\n\n";
    screen += "              \033[38;2;255;215;0m[1]\033[0m N-Easy  \033[38;2;255;215;0m[2]\033[0m N-Hard  \033[38;2;255;215;0m[3]\033[0m I-Easy  \033[38;2;255;215;0m[4]\033[0m I-Hard\n";
    screen += "              \033[38;2;255;215;0m[5]\033[0m 2P Mode \033[38;2;255;215;0m[6]\033[0m Speed   \033[38;2;255;215;0m[7]\033[0m Dark    \033[38;2;255;215;0m[8]\033[0m Moon\n";
    screen += "              \033[38;2;255;215;0m[Q]\033[0m Quit Game\n\n";
    
    cout << screen;
}

void drawGame() {
    string screen = "\033[H"; 

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int wx = x + camX;
            if (wx < 0 || wx >= MAP_W) {
                screen += " ";
                continue;
            }

            if (isDarkMode) {
                if (abs((wx) - p1X) > 6 || abs(y - p1Y) > 4) {
                    screen += " ";
                    continue;
                }
            }
            
            bool printed = false;
            if (p1Alive && p1X == wx && p1Y == y) {
                if (p1Inv == 0 || (p1Inv > 0 && gameTicks % 4 < 2)) {
                    screen += "\033[38;2;255;0;0mM\033[0m"; 
                } else {
                    screen += " ";
                }
                printed = true;
            } 
            else if (is2PMode && p2Alive && p2X == wx && p2Y == y) {
                if (p2Inv == 0 || (p2Inv > 0 && gameTicks % 4 < 2)) {
                    screen += "\033[38;2;0;255;0mL\033[0m"; 
                } else {
                    screen += " ";
                }
                printed = true;
            }
            else if (!p1Alive && p1X == wx && p1Y - 1 == y) {
                screen += "X"; printed = true;
            } else if (is2PMode && !p2Alive && p2X == wx && p2Y - 1 == y) {
                screen += "X"; printed = true;
            }
            
            if (printed) continue;

            bool splatPrinted = false;
            for (int i = 0; i < splatCount; i++) {
                if (splats[i].x == wx && splats[i].y == y) {
                    screen += "\033[38;2;255;20;147m*\033[0m";
                    splatPrinted = true;
                    break;
                }
            }
            if (splatPrinted) continue;

            bool mPrinted = false;
            for (int i = 0; i < monsterCount; i++) {
                if (monsters[i].alive && monsters[i].x == wx && monsters[i].y == y) {
                    screen += "\033[38;2;0;150;255mG\033[0m"; 
                    mPrinted = true;
                    break;
                }
            }
            if (mPrinted) continue;

            char c = map[y][wx];
            if (c == '#') screen += "\033[38;2;139;69;19m#\033[0m"; 
            else if (c == '=') screen += "\033[38;2;255;165;0m=\033[0m"; 
            else if (c == 'o') screen += "\033[38;2;255;215;0mo\033[0m"; 
            else screen += c;
        }
        screen += "\033[K\n"; 
    }

    char hud[128];
    if (is2PMode) {
        sprintf(hud, "Mario: %d pts | Luigi: %d pts | Coins: %d/%d", p1Score, p2Score, collectedCoins, totalCoins);
    } else if (isSpeedMode) {
        sprintf(hud, "\033[38;2;255;50;50mTIME: %.1fs\033[0m | Score: %d | Coins: %d/%d", timeLeft / 12.0, score, collectedCoins, totalCoins);
    } else {
        sprintf(hud, "[%s] Score: %d/800 | Coins: %d/%d | Kills: %d/%d", 
                isDarkMode ? "Dark" : (isMoonMode ? "Moon" : (isIslandMode ? "Island" : "Normal")),
                score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
    }
    screen += "\033[38;2;255;255;255m"; 
    screen += hud;
    screen += "\033[K\n";

    cout << screen;
}

int main() {
    srand(time(0));
    initConsole();
    initMap(); 
    
    drawMenu();
    while (!gameStarted) {
        if (keyDown('1')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('2')) { isEasyMode = false; isIslandMode = false; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('3')) { isEasyMode = true; isIslandMode = true; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('4')) { isEasyMode = false; isIslandMode = true; is2PMode = false; isSpeedMode = isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('5')) { isEasyMode = true; isIslandMode = false; is2PMode = true; isSpeedMode = isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('6')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = true; isDarkMode = isMoonMode = false; gameStarted = true; }
        else if (keyDown('7')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = false; isDarkMode = true; isMoonMode = false; gameStarted = true; }
        else if (keyDown('8')) { isEasyMode = true; isIslandMode = false; is2PMode = false; isSpeedMode = false; isDarkMode = false; isMoonMode = true; gameStarted = true; }
        else if (keyDown('Q')) return 0;
        Sleep(30);
    }

    resetGame(); 

    bool lastGameOverState = false;
    while (true) {
        update();
        
        if (!gameOver && !win) {
            drawGame();
            lastGameOverState = false;
        } else if (!lastGameOverState) {
            drawGameOverScreen();
            lastGameOverState = true;
        }
        
        Sleep(100);
    }
    return 0;
}

