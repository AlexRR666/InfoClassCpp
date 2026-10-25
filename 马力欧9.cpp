#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
#include <queue>
#include <vector>
using namespace std;

const int W = 60;
const int H = 20;
const int MAX_MAP_W = 600;
const int MAX_MAP_H = 600;
const int NORMAL_MAP_W = 600;
const int NORMAL_MAP_H = 20;
const int GROUND_Y = H - 4;

int curMapW = NORMAL_MAP_W;
int curMapH = NORMAL_MAP_H;

struct MovingPlatform {
    int x, y;
    int startX, startY;
    int len;
    int dir;
    int moveTimer;
    int speed;
    int range;
    bool horizontal;
    int coinOffsetX;
    bool hasCoin;
    int coinMask;
};
const int MAX_MP = 100;
MovingPlatform mps[MAX_MP];
int mpCount = 0;

int menuState = 0;
int selectedPlayerCount = 1;
int selectedTerrain = 0;
int selectedModifier = 0;

int p1X = 2, p1Y = GROUND_Y - 1, p1Vy = 0;
bool p1OnGround = true, p1Alive = true;
int p1Coins = 0, p1RespawnTimer = 0, p1Coyote = 0, p1Inv = 0;
int p1Score = 0;
bool p1CanDoubleJump = false;

int p2X = 4, p2Y = GROUND_Y - 1, p2Vy = 0;
bool p2OnGround = true, p2Alive = true;
int p2Coins = 0, p2RespawnTimer = 0, p2Coyote = 0, p2Inv = 0;
int p2Score = 0;
bool p2CanDoubleJump = false;

bool p1JumpPrev = false, p2JumpPrev = false;

int camX = 0;
int camY = 0;
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
bool isTowerMode = false;
bool isMonsterMode = false;
bool isCoinsMode = false;
bool isMovingPlatformMode = false;

bool gameStarted = false;
int gameTicks = 0;
int timeLeft = 0;

int p1MoveTimer = 0, p2MoveTimer = 0;

char map[MAX_MAP_H][MAX_MAP_W];

struct Monster {
    int x, y;
    int dir;
    bool alive;
    int moveTimer;
    int speed;
};

const int MAX_MON = 500;
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

string currentBGM = "";

typedef BOOL (WINAPI *PSND)(LPCSTR, HMODULE, DWORD);
PSND pPlaySound = NULL;

string exeDir() {
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    string s = buf;
    size_t pos = s.find_last_of("\\/");
    if (pos != string::npos) s = s.substr(0, pos + 1);
    return s;
}

bool fileExists(const string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

string findWav(const string& name) {
    string p1 = exeDir() + name;
    if (fileExists(p1)) return p1;
    if (fileExists(name)) return name;
    return "";
}

void initAudio() {
    HMODULE h = LoadLibraryA("winmm.dll");
    if (h) pPlaySound = (PSND)GetProcAddress(h, "PlaySoundA");
}

void playBGM(const string& name) {
    if (currentBGM == name) return;
    currentBGM = name;
    if (!pPlaySound) return;
    pPlaySound(NULL, NULL, 0);
    string path = findWav(name);
    if (path.empty()) return;
    pPlaySound(path.c_str(), NULL, SND_ASYNC | SND_LOOP | SND_FILENAME);
}

void stopBGM() {
    currentBGM = "";
    if (pPlaySound) pPlaySound(NULL, NULL, 0);
}

void initConsole() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
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
    if (x2 >= curMapW) x2 = curMapW - 1;
    if (y1 < 0) y1 = 0;
    if (y2 >= curMapH) y2 = curMapH - 1;
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            map[y][x] = ch;
}

bool isSolid(int x, int y) {
    if (x < 0 || x >= curMapW) return true;
    if (y < 0 || y >= curMapH) return false;
    return map[y][x] == '#';
}

bool isOneWay(int x, int y) {
    if (x < 0 || x >= curMapW) return false;
    if (y < 0 || y >= curMapH) return false;
    return map[y][x] == '=';
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

void findSafeSpot(int &outX, int &outY, int centerX) {
    for (int ty = camY + H - 2; ty >= camY + 2; ty--) {
        for (int d = 0; d <= 30; d++) {
            int cx1 = centerX - d;
            int cx2 = centerX + d;
            if (cx1 >= 0 && cx1 < curMapW && hasGround(cx1, ty - 1)) { outX = cx1; outY = ty - 1; return; }
            if (d != 0 && cx2 >= 0 && cx2 < curMapW && hasGround(cx2, ty - 1)) { outX = cx2; outY = ty - 1; return; }
        }
    }
    outX = centerX;
    outY = camY + H / 2;
}

void initTowerMap() {
    curMapW = 60;
    curMapH = 400;
    camX = 0;
    camY = 380;

    for (int y = 0; y < curMapH; y++)
        for (int x = 0; x < curMapW; x++)
            map[y][x] = ' ';

    fillRect('#', 0, 398, 11, 399);
    fillRect('#', 24, 398, 35, 399);
    fillRect('#', 48, 398, 59, 399);

    int gap = isEasyMode ? 3 : 4;
    int ly = 394;
    int lastCX = 30;
    int layerIndex = 0;
    int mpPlaced = 0;
    bool lastWasSolid = false;

    struct Layer { int cx; int y; int w; char ch; };
    vector<Layer> layers;

    if (isMovingPlatformMode) {
        while (ly > 12 && mpCount < MAX_MP) {
            int len = 8 + rand() % 4;
            int cx = lastCX + (rand() % 7 - 3);
            int half = len / 2;
            if (cx - half < 1) cx = half + 1;
            if (cx + half > curMapW - 2) cx = curMapW - 2 - half;
            int mx = cx - half;
            if (mx < 0) mx = 0;
            if (mx + len > curMapW) mx = curMapW - len;
            int range = 5 + rand() % 3;

            MovingPlatform mp;
            mp.x = mx; mp.y = ly;
            mp.startX = mx; mp.startY = ly;
            mp.len = len;
            mp.dir = 1; mp.moveTimer = 0;
            mp.speed = 5;
            mp.range = range;
            mp.horizontal = true;
            mp.coinOffsetX = 0;
            mp.hasCoin = true;
            mp.coinMask = 1 << (rand() % len);
            mps[mpCount++] = mp;

            lastCX = mx + len / 2;
            ly -= gap;
            layerIndex++;
        }

        fillRect('#', 22, 8, 38, 8);
        fillRect('#', 15, 11, 45, 11);
        map[7][30] = 'S';

        int targetMonsters = isMonsterMode ? MAX_MON : (isEasyMode ? (15 + rand() % 11) : (20 + rand() % 11));
        int placed = 0, attempts = 0;
        while (placed < targetMonsters && attempts < targetMonsters * 20 && monsterCount < MAX_MON) {
            int pi = rand() % mpCount;
            MovingPlatform &p = mps[pi];
            int mxx = p.x + rand() % p.len;
            int myy = p.y - 1;
            if (myy > 2 && mxx >= 0 && mxx < curMapW && map[myy][mxx] == ' ') {
                bool occupied = false;
                for (int j = 0; j < monsterCount; j++) {
                    if (monsters[j].alive && monsters[j].x == mxx && monsters[j].y == myy) { occupied = true; break; }
                }
                if (!occupied) {
                    addMonster(mxx, myy, (rand() % 2 == 0) ? 6 : 10, rand() % 4);
                    placed++;
                }
            }
            attempts++;
        }
        totalMonsters = monsterCount;
        return;
    }

    while (ly > 12) {
        bool useMP = (mpPlaced < 16 && layerIndex % 7 == 3);

        if (useMP) {
            int len = 7 + rand() % 4;
            int mx = 10 + rand() % (curMapW - 10 - len - 10);
            MovingPlatform mp;
            mp.x = mx; mp.y = ly;
            mp.startX = mx; mp.startY = ly;
            mp.len = len;
            mp.dir = 1; mp.moveTimer = 0;
            mp.speed = 4;
            mp.range = 10;
            mp.horizontal = true;
            mp.coinOffsetX = 0;
            mp.hasCoin = true;
            if (isCoinsMode) mp.coinMask = (1 << len) - 1;
            else mp.coinMask = 1 << (rand() % len);
            mps[mpCount++] = mp;
            mpPlaced++;
            lastCX = mx + len / 2;
            lastWasSolid = false;
        } else {
            bool solid = (layerIndex > 0) && !lastWasSolid && (rand() % 5 == 0);
            int w = solid ? (4 + rand() % 3) : (10 + rand() % 4);
            int cx = lastCX + (rand() % 3 - 1);
            int half = w / 2;
            if (cx - half < 1) cx = half + 1;
            if (cx + half > curMapW - 2) cx = curMapW - 2 - half;
            int x1 = cx - half;
            char ch = solid ? '#' : '=';
            fillRect(ch, x1, ly, x1 + w - 1, ly);
            Layer L;
            L.cx = cx; L.y = ly; L.w = w; L.ch = ch;
            layers.push_back(L);
            lastCX = cx;
            lastWasSolid = solid;
        }
        ly -= gap;
        layerIndex++;
    }

    fillRect('=', 22, 8, 38, 8);
    fillRect('=', 15, 11, 45, 11);
    map[7][30] = 'S';

    int totalLayers = (int)layers.size();
    vector<int> idx;
    for (int i = 0; i < totalLayers; i++) idx.push_back(i);
    for (int i = 0; i < totalLayers; i++) {
        int j = rand() % totalLayers;
        int tmp = idx[i]; idx[i] = idx[j]; idx[j] = tmp;
    }

    int coinLimit = isCoinsMode ? 99999 : 60;
    int coinCount = 0;
    for (int i = 0; i < totalLayers && coinCount < coinLimit; i++) {
        Layer &L = layers[idx[i]];
        int x1 = L.cx - L.w / 2;
        int coinX = x1 + rand() % L.w;
        if (map[L.y - 1][coinX] == ' ') {
            map[L.y - 1][coinX] = 'o';
            coinCount++;
            totalCoins++;
        }
    }
    if (coinCount < coinLimit) {
        for (int i = 0; i < totalLayers && coinCount < coinLimit; i++) {
            Layer &L = layers[i];
            int x1 = L.cx - L.w / 2;
            for (int t = 0; t < L.w && coinCount < coinLimit; t++) {
                int coinX = x1 + t;
                if (coinX >= 0 && coinX < curMapW && map[L.y - 1][coinX] == ' ') {
                    map[L.y - 1][coinX] = 'o';
                    coinCount++;
                    totalCoins++;
                }
            }
        }
    }

    int targetMonsters = isMonsterMode ? MAX_MON : 12;
    for (int i = 0; i < totalLayers; i++) {
        int j = rand() % totalLayers;
        int tmp = idx[i]; idx[i] = idx[j]; idx[j] = tmp;
    }
    int monCount = 0;
    for (int i = 0; i < totalLayers && monCount < targetMonsters; i++) {
        Layer &L = layers[idx[i]];
        int x1 = L.cx - L.w / 2;
        int monX = x1 + rand() % L.w;
        char c = map[L.y - 1][monX];
        if (c == ' ' || c == 'o') {
            addMonster(monX, L.y - 1, (rand() % 2 == 0) ? 6 : 10, rand() % 4);
            monCount++;
        }
    }
    if (monCount < targetMonsters) {
        for (int i = 0; i < totalLayers && monCount < targetMonsters; i++) {
            Layer &L = layers[i];
            int x1 = L.cx - L.w / 2;
            for (int t = 0; t < L.w && monCount < targetMonsters; t++) {
                int monX = x1 + t;
                if (monX >= 0 && monX < curMapW && map[L.y - 1][monX] != '#' && monCount < MAX_MON) {
                    addMonster(monX, L.y - 1, (rand() % 2 == 0) ? 6 : 10, rand() % 4);
                    monCount++;
                }
            }
        }
    }

    totalMonsters = monsterCount;
}

void initMap() {
    totalCoins = 0;
    collectedCoins = 0;
    monsterCount = 0;
    totalMonsters = 0;
    monstersKilled = 0;
    splatCount = 0;
    mpCount = 0;
    gameTicks = 0;
    camX = 0;
    camY = 0;
    srand((unsigned)time(0));

    if (isTowerMode) {
        initTowerMap();
        return;
    }

    curMapW = NORMAL_MAP_W;
    curMapH = NORMAL_MAP_H;

    for (int y = 0; y < curMapH; y++)
        for (int x = 0; x < curMapW; x++)
            map[y][x] = ' ';

    if (isMovingPlatformMode) {
        fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);
        fillRect('#', curMapW - 30, GROUND_Y, curMapW - 1, GROUND_Y + 1);

        int px2 = 22;
        int lastY = GROUND_Y - 3;
        while (px2 < curMapW - 40 && mpCount < MAX_MP) {
            int len = 4 + rand() % 3;
            lastY += (rand() % 5 - 2);
            if (lastY > GROUND_Y - 2) lastY = GROUND_Y - 2;
            if (lastY < 6) lastY = 6;
            int range = 3 + rand() % 3;

            MovingPlatform mp;
            mp.x = px2; mp.y = lastY;
            mp.startX = px2; mp.startY = lastY;
            mp.len = len;
            mp.dir = 1; mp.moveTimer = 0;
            mp.speed = 4 + rand() % 2;
            mp.range = range;
            mp.horizontal = true;
            mp.coinOffsetX = 0;
            mp.hasCoin = true;
            mp.coinMask = 1 << (rand() % len);
            mps[mpCount++] = mp;

            px2 += 5 + rand() % 3;
        }
    } else if (isIslandMode) {
        fillRect('#', 0, GROUND_Y, 12, GROUND_Y + 1);
        int currentX = 20, minGap = isEasyMode ? 3 : 4, maxGap = isEasyMode ? 5 : 7, currentY = GROUND_Y - 2;
        while (currentX < curMapW - 20) {
            int width = 6 + rand() % 4, depth = rand() % 2;
            for (int i = 0; i < width; i++) {
                map[currentY][currentX + i] = '#';
                for (int d = 1; d <= depth; d++) if (currentY + d < curMapH) map[currentY + d][currentX + i] = '#';
            }
            if (width >= 7 && rand() % 2 == 0) {
                fillRect('=', currentX + 2, currentY - 3, currentX + width - 3, currentY - 3);
            }
            currentY += (rand() % 3 - 1);
            if (currentY > GROUND_Y - 1) currentY = GROUND_Y - 1;
            if (currentY < GROUND_Y - 5) currentY = GROUND_Y - 5;
            currentX += width + minGap + rand() % (maxGap - minGap + 1);
        }
        fillRect('#', curMapW - 30, GROUND_Y, curMapW - 1, GROUND_Y + 1);
    } else {
        for (int x = 0; x < curMapW; x++) map[GROUND_Y][x] = map[GROUND_Y + 1][x] = '#';
        fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);
        int base = 20;
        while (base < curMapW - 40) {
            int type = rand() % 5;
            if (type == 0) {
                for (int i = 0; i < 3; i++) fillRect('#', base + i, GROUND_Y - 1 - i, base + 5 - i, GROUND_Y - 1 - i);
                for (int i = 1; i <= 3; i++) fillRect('#', base + 5 + i, GROUND_Y - 4 + i, base + 8 - i, GROUND_Y - 4 + i);
                base += 15;
            } else if (type == 1) {
                fillRect('#', base, GROUND_Y - 3, base + 1, GROUND_Y); base += 10;
            } else if (type == 2) {
                for (int i = 0; i < 4; i++) fillRect('#', base + i, GROUND_Y - i - 1, base + i, GROUND_Y);
                int pitWidth = isEasyMode ? 4 : 5;
                for (int i = 0; i < pitWidth; i++) { map[GROUND_Y][base + 4 + i] = ' '; map[GROUND_Y + 1][base + 4 + i] = ' '; }
                for (int i = 0; i < 4; i++) fillRect('#', base + 4 + pitWidth + i, GROUND_Y - 4 + i, base + 4 + pitWidth + i, GROUND_Y);
                base += 20;
            } else if (type == 3) {
                int pitWidth = 12;
                for (int i = 0; i < pitWidth; i++) { map[GROUND_Y][base + i] = ' '; map[GROUND_Y + 1][base + i] = ' '; }
                fillRect('=', base + 1, GROUND_Y - 3, base + 4, GROUND_Y - 3);
                fillRect('=', base + 6, GROUND_Y - 4, base + 9, GROUND_Y - 4);
                base += 18;
            } else {
                int maxPit = isEasyMode ? 5 : 6;
                int pitWidth = 3 + rand() % (maxPit - 3 + 1);
                for (int p = 0; p < pitWidth; p++) if (base + p < curMapW) { map[GROUND_Y][base + p] = ' '; map[GROUND_Y + 1][base + p] = ' '; }
                base += pitWidth + 5;
            }
        }
    }

    if (!isMovingPlatformMode) {
        int targetMPs = 15 + rand() % 10;
        for (int i = 0; i < targetMPs && mpCount < MAX_MP; i++) {
            int mx = 30 + rand() % (curMapW - 60);
            int my = GROUND_Y - (3 + rand() % 4);
            int len = 3 + rand() % 4;
            bool h = rand() % 3 != 0;
            int range = 3 + rand() % 5;
            MovingPlatform mp;
            mp.x = mx; mp.y = my;
            mp.startX = mx; mp.startY = my;
            mp.len = len;
            mp.dir = 1;
            mp.moveTimer = 0;
            mp.speed = 4 + rand() % 3;
            mp.range = range;
            mp.horizontal = h;
            mp.coinOffsetX = 0;
            mp.hasCoin = true;
            if (isCoinsMode) mp.coinMask = (1 << len) - 1;
            else mp.coinMask = 1 << (rand() % len);
            mps[mpCount++] = mp;
        }
    }

    for (int y = 0; y < GROUND_Y; y++) for (int x = 0; x < 12; x++) map[y][x] = ' ';
    fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);

    bool reachable[NORMAL_MAP_H][NORMAL_MAP_W] = {false};
    queue<pair<int, int> > q;
    q.push(make_pair(2, 2));

    while (!q.empty()) {
        int cx = q.front().first;
        int cy = q.front().second;
        q.pop();
        if (reachable[cy][cx]) continue;
        reachable[cy][cx] = true;

        if (cx > 0 && !isSolid(cx - 1, cy)) q.push(make_pair(cx - 1, cy));
        if (cx < curMapW - 1 && !isSolid(cx + 1, cy)) q.push(make_pair(cx + 1, cy));
        if (cy < curMapH - 1 && !isSolid(cx, cy + 1) && !isOneWay(cx, cy + 1)) q.push(make_pair(cx, cy + 1));

        if (cy < curMapH - 1 && (isSolid(cx, cy + 1) || isOneWay(cx, cy + 1))) {
            for (int jy = 1; jy <= 8; jy++) {
                if (cy - jy >= 0 && !isSolid(cx, cy - jy)) {
                    q.push(make_pair(cx, cy - jy));
                    int horiz = (jy < 4) ? jy : 4;
                    for (int dx = 1; dx <= horiz; dx++) {
                        if (cx - dx >= 0 && !isSolid(cx - dx, cy - jy)) q.push(make_pair(cx - dx, cy - jy));
                        if (cx + dx < curMapW && !isSolid(cx + dx, cy - jy)) q.push(make_pair(cx + dx, cy - jy));
                    }
                } else break;
            }
        }
    }

    vector<pair<int, int> > spots;
    for (int x = 12; x < curMapW - 12; x++) {
        for (int y = 2; y < GROUND_Y; y++) {
            if (reachable[y][x] && map[y][x] == ' ') {
                if (y - 1 >= 0 && y - 2 >= 0 && map[y - 1][x] == ' ' && map[y - 2][x] == ' ') {
                    bool ground = false;
                    for (int dy = 1; dy <= 5; dy++) {
                        if (y + dy < curMapH && (isSolid(x, y + dy) || isOneWay(x, y + dy))) { ground = true; break; }
                    }
                    if (ground) spots.push_back(make_pair(x, y));
                }
            }
        }
    }

    if (!spots.empty()) {
        for (int i = 0; i < (int)spots.size(); i++) {
            int j = rand() % spots.size();
            pair<int, int> tmp = spots[i]; spots[i] = spots[j]; spots[j] = tmp;
        }
    }

    int platCoinCount = 0;
    for (int i = 0; i < mpCount; i++) if (mps[i].hasCoin) platCoinCount++;

    int coinLimit;
    if (isCoinsMode) coinLimit = 99999;
    else if (isMovingPlatformMode) coinLimit = 105 - platCoinCount;
    else coinLimit = 105;
    if (coinLimit < 0) coinLimit = 0;

    for (int i = 0; i < (int)spots.size() && i < coinLimit; i++) {
        map[spots[i].second][spots[i].first] = 'o';
        totalCoins++;
    }

    if (!isCoinsMode) {
        int attempts = 0;
        while (totalCoins < coinLimit && attempts < 3000) {
            int rx = 20 + rand() % (curMapW - 40);
            int ry = 2 + rand() % (GROUND_Y - 2);
            if (map[ry][rx] == ' ' && reachable[ry][rx]) {
                map[ry][rx] = 'o';
                totalCoins++;
            }
            attempts++;
        }
    }

    if (isMovingPlatformMode) {
        int targetMonsters = isMonsterMode ? MAX_MON : (isEasyMode ? (15 + rand() % 11) : (20 + rand() % 11));
        int placed = 0, attempts = 0;
        while (placed < targetMonsters && attempts < targetMonsters * 20 && monsterCount < MAX_MON) {
            int pi = rand() % mpCount;
            MovingPlatform &p = mps[pi];
            int mxx = p.x + rand() % p.len;
            int myy = p.y - 1;
            if (myy > 2 && mxx >= 0 && mxx < curMapW && map[myy][mxx] == ' ') {
                bool occupied = false;
                for (int j = 0; j < monsterCount; j++) {
                    if (monsters[j].alive && monsters[j].x == mxx && monsters[j].y == myy) { occupied = true; break; }
                }
                if (!occupied) {
                    addMonster(mxx, myy, (rand() % 2 == 0) ? 6 : 10, rand() % 4);
                    placed++;
                }
            }
            attempts++;
        }
        totalMonsters = monsterCount;
        return;
    }

    int targetMonsters;
    if (isMonsterMode) targetMonsters = MAX_MON;
    else targetMonsters = isEasyMode ? (15 + rand() % 11) : (20 + rand() % 11);
    int attempts = 0;
    while (monsterCount < targetMonsters && attempts < targetMonsters * 30) {
        int mx = 20 + rand() % (curMapW - 40);
        int my = -1;

        if (rand() % 2 == 0) {
            my = GROUND_Y - 1;
            while (my > 2 && !hasGround(mx, my)) my--;
        } else {
            my = 2 + rand() % (GROUND_Y - 3);
            while (my < GROUND_Y - 1 && !hasGround(mx, my)) my++;
        }

        if (my > 2 && my < GROUND_Y && map[my][mx] == ' ' && hasGround(mx, my)) {
            addMonster(mx, my, (rand() % 2 == 0) ? 6 : 10, rand() % 4);
        }
        attempts++;
    }
    totalMonsters = monsterCount;
}

void updateMovingPlatforms() {
    for (int i = 0; i < mpCount; i++) {
        MovingPlatform &mp = mps[i];
        int oldX = mp.x;
        int oldY = mp.y;

        mp.moveTimer++;
        if (mp.moveTimer >= mp.speed) {
            mp.moveTimer = 0;
            if (mp.horizontal) {
                int nx = mp.x + mp.dir;
                bool hitWall = false;
                for (int j = 0; j < mp.len; j++) {
                    if (isSolid(nx + j, mp.y)) { hitWall = true; break; }
                }
                if (hitWall) mp.dir *= -1;
                else mp.x = nx;
                if (mp.x <= mp.startX - mp.range) { mp.dir = 1; mp.x += 1; }
                else if (mp.x >= mp.startX + mp.range) { mp.dir = -1; mp.x -= 1; }
            } else {
                mp.y += mp.dir;
                if (mp.dir < 0) {
                    bool ceiling = false;
                    for (int j = 0; j < mp.len; j++) {
                        if (isSolid(mp.x + j, mp.y)) { ceiling = true; break; }
                    }
                    if (ceiling) { mp.dir = 1; mp.y += 1; }
                }
                if (mp.y <= mp.startY - mp.range) { mp.dir = 1; mp.y += 1; }
                else if (mp.y >= mp.startY + mp.range) { mp.dir = -1; mp.y -= 1; }
            }
        }

        int dx = mp.x - oldX;
        int dy = mp.y - oldY;

        if (p1Y == oldY - 1 && p1X >= oldX && p1X < oldX + mp.len) {
            if (!isSolid(p1X + dx, p1Y)) p1X += dx;
            if (!isSolid(p1X, p1Y + dy)) p1Y += dy;
        }
        if (is2PMode && p2Y == oldY - 1 && p2X >= oldX && p2X < oldX + mp.len) {
            if (!isSolid(p2X + dx, p2Y)) p2X += dx;
            if (!isSolid(p2X, p2Y + dy)) p2Y += dy;
        }

        for (int j = 0; j < monsterCount; j++) {
            if (!monsters[j].alive) continue;
            if (monsters[j].y == oldY - 1 && monsters[j].x >= oldX && monsters[j].x < oldX + mp.len) {
                monsters[j].x += dx;
                monsters[j].y += dy;
            }
        }
    }
}

void updateMonsters() {
    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];
        m.moveTimer++;
        if (m.moveTimer < m.speed) continue;
        m.moveTimer = 0;

        if (isMovingPlatformMode) {
            int platIdx = -1;
            for (int j = 0; j < mpCount; j++) {
                if (m.y == mps[j].y - 1 && m.x >= mps[j].x && m.x < mps[j].x + mps[j].len) {
                    platIdx = j; break;
                }
            }
            if (platIdx < 0) continue;
            MovingPlatform &p = mps[platIdx];
            int nx = m.x + m.dir;
            if (nx < p.x || nx >= p.x + p.len) {
                m.dir = -m.dir;
            } else {
                m.x = nx;
            }
            continue;
        }

        if (isMoonMode) {
            m.dir = (p1X > m.x) ? 1 : -1;
            int nx = m.x + m.dir;
            if (nx >= 0 && nx < curMapW && !isSolid(nx, m.y)) m.x = nx;
        } else {
            int dist = 0;
            if (is2PMode) {
                int d1 = abs(p1X - m.x) + abs(p1Y - m.y);
                int d2 = abs(p2X - m.x) + abs(p2Y - m.y);
                if (d1 < d2) dist = p1X - m.x;
                else dist = p2X - m.x;
            } else dist = p1X - m.x;

            if (hasGround(m.x, m.y) && abs(dist) <= 8 && dist != 0) m.dir = (dist > 0) ? 1 : -1;
            int nx = m.x + m.dir;
            if (!hasGround(nx, m.y) || isSolid(nx, m.y)) {
                m.dir = -m.dir; nx = m.x + m.dir;
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
            for (int j = i; j < splatCount - 1; j++) splats[j] = splats[j + 1];
            splatCount--; i--;
        }
    }
}

void updatePlayer(int &px, int &py, int &vy, bool &onGround, bool &alive, int &respawn, int &coins, int &pScore, int &inv, bool &canDoubleJump, int &moveTimer, bool isP1) {
    if (!alive) {
        respawn--;
        if (respawn <= 0) {
            alive = true; vy = 0;
            if (isTowerMode) {
                findSafeSpot(px, py, curMapW / 2);
            } else {
                px = (isP1 ? 2 : 6); py = 2;
            }
            if (isP1) p1Coyote = 0; else p2Coyote = 0;
        }
        return;
    }

    if (inv > 0) inv--;

    int &coyote = isP1 ? p1Coyote : p2Coyote;
    bool left = isP1 ? (keyDown('A') || (!is2PMode && keyDown(VK_LEFT))) : (keyDown(VK_LEFT));
    bool right = isP1 ? (keyDown('D') || (!is2PMode && keyDown(VK_RIGHT))) : (keyDown(VK_RIGHT));
    bool jump = isP1 ? (keyDown('W') || keyDown(VK_SPACE) || (!is2PMode && keyDown(VK_UP))) : (keyDown(VK_UP));

    moveTimer++;
    if (moveTimer >= 2) {
        moveTimer = 0;
        int dx = 0;
        if (left && !right) dx = -1;
        if (right && !left) dx = 1;

        if (dx != 0) {
            bool hitPlayerX = false;
            if (is2PMode) {
                if (isP1 && p2Alive && px + dx == p2X && py == p2Y) hitPlayerX = true;
                if (!isP1 && p1Alive && px + dx == p1X && py == p1Y) hitPlayerX = true;
            }
            if (!isSolid(px + dx, py) && !hitPlayerX) {
                bool carrying = false;
                if (is2PMode) {
                    if (isP1 && p2Alive && p2X == px && p2Y == py - 1) carrying = true;
                    if (!isP1 && p1Alive && p1X == px && p1Y == py - 1) carrying = true;
                }
                if (!carrying || (carrying && !isSolid((isP1 ? p2X : p1X) + dx, (isP1 ? p2Y : p1Y)))) {
                    px += dx;
                    if (carrying) { if (isP1) p2X += dx; else p1X += dx; }
                    if (map[py][px] == 'o') { map[py][px] = ' '; coins++; collectedCoins++; pScore += 10; if (isSpeedMode) timeLeft += 50; }
                    if (carrying) {
                        if (isP1 && map[p2Y][p2X] == 'o') { map[p2Y][p2X] = ' '; p2Coins++; collectedCoins++; p2Score += 10; if (isSpeedMode) timeLeft += 50; }
                        if (!isP1 && map[p1Y][p1X] == 'o') { map[p1Y][p1X] = ' '; p1Coins++; collectedCoins++; p1Score += 10; if (isSpeedMode) timeLeft += 50; }
                    }
                }
            }
        }
    }

    bool carrying = false;
    bool onTeammate = false;
    if (is2PMode) {
        if (isP1) {
            if (p2Alive && p2X == px && p2Y == py - 1) carrying = true;
            if (p2Alive && px == p2X && py + 1 == p2Y) onTeammate = true;
        } else {
            if (p1Alive && p1X == px && p1Y == py - 1) carrying = true;
            if (p1Alive && px == p1X && py + 1 == p1Y) onTeammate = true;
        }
    }

    bool onMovingPlatform = false;
    for (int i = 0; i < mpCount; i++) {
        if (py == mps[i].y - 1 && px >= mps[i].x && px < mps[i].x + mps[i].len) {
            onMovingPlatform = true; break;
        }
    }

    if (onGround && !isSolid(px, py + 1) && !isOneWay(px, py + 1) && !onTeammate && !onMovingPlatform) { onGround = false; vy = 1; }

    if (onGround || onMovingPlatform) coyote = 0;
    else if (coyote < 10) coyote++;

    bool &jumpPrev = isP1 ? p1JumpPrev : p2JumpPrev;

    if (jump && !jumpPrev) {
        if (onGround || onMovingPlatform || coyote <= 5) {
            if (carrying) {
                vy = isSpeedMode ? -11 : (isMoonMode ? -12 : -5);
                if (isP1) { p2Vy = vy; p2OnGround = false; p2CanDoubleJump = true; p2Coyote = 10; }
                else { p1Vy = vy; p1OnGround = false; p1CanDoubleJump = true; p1Coyote = 10; }
            } else {
                vy = isSpeedMode ? -11 : (isMoonMode ? -10 : -5);
            }
            onGround = false;
            coyote = 10;
        } else if (canDoubleJump) {
            vy = isSpeedMode ? -11 : (isMoonMode ? -10 : -5);
            canDoubleJump = false;
            if (is2PMode) {
                if (isP1 && onTeammate) p2Vy = 8;
                else if (!isP1 && onTeammate) p1Vy = 8;
            }
        }
    }
    jumpPrev = jump;

    if (!onGround) {
        if (vy == 0) vy = 1;
        int step = (vy > 0) ? 1 : -1;

        bool doMove = (step < 0) || ((gameTicks + (isP1 ? 0 : 1)) % 2 == 0);

        if (doMove) {
            int ny = py + step;
            if (ny <= 0) { py = 0; vy = 0; canDoubleJump = false; }
            else if (step < 0) { if (isSolid(px, ny)) vy = 0; else py = ny; }
            else {
                bool hitPlayer = false;
                if (is2PMode) {
                    if (isP1 && p2Alive && px == p2X && ny == p2Y) hitPlayer = true;
                    if (!isP1 && p1Alive && px == p1X && ny == p1Y) hitPlayer = true;
                }

                bool hitMP = false;
                for (int i = 0; i < mpCount; i++) {
                    if (ny == mps[i].y && px >= mps[i].x && px < mps[i].x + mps[i].len) { hitMP = true; break; }
                }

                if (isSolid(px, ny) || isOneWay(px, ny) || hitPlayer || hitMP) {
                    onGround = true; vy = 0;
                    canDoubleJump = false;
                } else py = ny;
            }
        }

        if (!onGround) {
            if (isMoonMode) { if (gameTicks % 4 == 0) vy += 1; }
            else {
                if (carrying) vy += 2;
                else vy += 1;
            }
        }
    }

    if (isTowerMode) {
        if (py > camY + H - 1) {
            if (gameTicks > 10) {
                if (pScore >= 200) {
                    pScore -= 200;
                    int newX = px, newY = camY + H - 2;
                    findSafeSpot(newX, newY, px);
                    px = newX; py = newY; vy = 0; inv = 60;
                    return;
                }
                alive = false; respawn = 60;
                if (!is2PMode) gameOver = true;
            }
        }
    } else {
        if (py >= curMapH - 1 && !isSolid(px, py + 1) && !isOneWay(px, py + 1)) {
            if (gameTicks > 10) {
                if (!is2PMode && pScore >= 200) {
                    pScore -= 200; px = max(2, px - 5); py = 2; vy = 0; inv = 60; return;
                }
                alive = false; respawn = 60;
                if (!is2PMode) gameOver = true;
            }
        }
    }

    if (map[py][px] == 'o') { map[py][px] = ' '; coins++; collectedCoins++; pScore += 10; if (isSpeedMode) timeLeft += 50; }

    for (int i = 0; i < mpCount; i++) {
        if (!mps[i].hasCoin) continue;
        if (py != mps[i].y - 1) continue;
        if (px < mps[i].x || px >= mps[i].x + mps[i].len) continue;
        int bit = px - mps[i].x;
        if (mps[i].coinMask & (1 << bit)) {
            mps[i].coinMask &= ~(1 << bit);
            if (mps[i].coinMask == 0) mps[i].hasCoin = false;
            coins++; collectedCoins++; pScore += 10;
            if (isSpeedMode) timeLeft += 50;
        }
    }

    for (int i = 0; i < monsterCount; i++) {
        if (!monsters[i].alive) continue;
        Monster &m = monsters[i];

        if (m.x == px && m.y == py && gameTicks > 10 && inv == 0) {
            if (pScore >= 100) { pScore -= 100; inv = 60; vy = -6; onGround = false; }
            else { alive = false; respawn = 60; if (!is2PMode) gameOver = true; return; }
        }

        if (vy > 0) {
            bool isStomp = false;
            if (py == m.y - 1 || (isMoonMode && abs(py - m.y) <= 1)) {
                if (!isEasyMode) { if (px == m.x) isStomp = true; }
                else { if (abs(px - m.x) <= 1) isStomp = true; }
            }
            if (isStomp) {
                m.alive = false; monstersKilled++; pScore += (15 + rand() % 6); addSplat(m.x, m.y); vy = -7;
                if (isSpeedMode) timeLeft += (isTowerMode ? 125 : 100);
            }
        }
    }
}

void resetGame() {
    initMap();

    if (!isTowerMode) {
        for (int y = 0; y < GROUND_Y; y++) for (int x = 0; x < 12; x++) map[y][x] = ' ';
        fillRect('#', 0, GROUND_Y, 15, GROUND_Y + 1);
    }

    totalCoins = 0;
    for (int y = 0; y < curMapH; y++)
        for (int x = 0; x < curMapW; x++)
            if (map[y][x] == 'o') totalCoins++;
    for (int i = 0; i < mpCount; i++) {
        if (!mps[i].hasCoin) continue;
        for (int b = 0; b < mps[i].len; b++)
            if (mps[i].coinMask & (1 << b)) totalCoins++;
    }

    if (isSpeedMode) {
        if (isTowerMode) timeLeft = 90 * 25;
        else timeLeft = 60 * 25;
    } else timeLeft = 0;

    if (isTowerMode) {
        p1X = 30; p1Y = 397;
        p2X = 32; p2Y = 397;
    } else {
        p1X = 2; p1Y = 2;
        p2X = 6; p2Y = 2;
    }
    p1Vy = 0; p1OnGround = false; p1Alive = true; p1RespawnTimer = 0; p1Coins = 0; p1Coyote = 0; p1Score = 0; p1Inv = 0; p1CanDoubleJump = false; p1MoveTimer = 0;
    p2Vy = 0; p2OnGround = false; p2Alive = true; p2RespawnTimer = 0; p2Coins = 0; p2Coyote = 0; p2Score = 0; p2Inv = 0; p2CanDoubleJump = false; p2MoveTimer = 0;

    p1JumpPrev = false; p2JumpPrev = false;
    camX = 0;
    camY = isTowerMode ? (curMapH - H) : 0;
    score = 0; gameOver = false; win = false; winType = 0;

    if (isMovingPlatformMode) playBGM("moving.wav");
    else if (isTowerMode) playBGM("tower.wav");
    else if (isSpeedMode) playBGM("speed.wav");
    else if (isDarkMode) playBGM("dark.wav");
    else if (isMonsterMode) playBGM("monster.wav");
    else if (isCoinsMode) playBGM("coins.wav");
    else playBGM("game.wav");
}

void update() {
    if (keyDown('M') || keyDown('m')) {
        menuState = 0;
        gameStarted = false;
        gameOver = false;
        win = false;
        return;
    }

    if (gameOver || win) {
        if (keyDown(VK_SPACE)) { resetGame(); }
        if (keyDown('Q')) exit(0);
        return;
    }

    gameTicks++;
    if (isSpeedMode) {
        timeLeft--;
        if (timeLeft <= 0) {
            if (isTowerMode) { gameOver = true; return; }
            if (is2PMode) {
                win = true;
                if (p1Score > p2Score) winType = 3;
                else if (p2Score > p1Score) winType = 4;
                else winType = 5;
                return;
            }
            else { gameOver = true; return; }
        }
    }
    if (!is2PMode) score = p1Score;

    updateMovingPlatforms();
    updatePlayer(p1X, p1Y, p1Vy, p1OnGround, p1Alive, p1RespawnTimer, p1Coins, p1Score, p1Inv, p1CanDoubleJump, p1MoveTimer, true);
    if (is2PMode) {
        updatePlayer(p2X, p2Y, p2Vy, p2OnGround, p2Alive, p2RespawnTimer, p2Coins, p2Score, p2Inv, p2CanDoubleJump, p2MoveTimer, false);
    }

    updateMonsters();
    updateSplats();

    if (isTowerMode) {
        if (!is2PMode) {
            if (p1Alive && map[p1Y][p1X] == 'S') {
                map[p1Y][p1X] = ' ';
                win = true;
                if (isSpeedMode) winType = 6;
                else if (collectedCoins >= totalCoins && monstersKilled == totalMonsters) winType = 2;
                else if (collectedCoins >= totalCoins) winType = 1;
                else winType = 0;
                return;
            }
        } else {
            if (p1Alive && map[p1Y][p1X] == 'S') {
                map[p1Y][p1X] = ' '; win = true; winType = 3; return;
            }
            if (p2Alive && map[p2Y][p2X] == 'S') {
                map[p2Y][p2X] = ' '; win = true; winType = 4; return;
            }
            if (!p1Alive && !p2Alive) { gameOver = true; return; }
        }
    } else if (is2PMode) {
        if (p1Score >= 700 && p1X >= curMapW - 5) { win = true; winType = 3; return; }
        if (p2Score >= 700 && p2X >= curMapW - 5) { win = true; winType = 4; return; }
        if (!p1Alive && !p2Alive) { gameOver = true; return; }
    }

    if (isTowerMode) {
        int focusY = p1Y;
        if (is2PMode) {
            if (!p1Alive) focusY = p2Y;
            else if (!p2Alive) focusY = p1Y;
            else focusY = (p1Coins <= p2Coins) ? p1Y : p2Y;
        }
        int targetCamY = focusY - H / 2;
        if (targetCamY < camY) camY = targetCamY;
        if (camY < 0) camY = 0;
        if (camY > curMapH - H) camY = curMapH - H;
        camX = 0;
    } else if (is2PMode) {
        int targetX;
        bool darkFar = abs(p1X - p2X) > W - 2;
        if (isDarkMode) {
            int aliveCount = 0, sumX = 0;
            if (p1Alive) { sumX += p1X; aliveCount++; }
            if (p2Alive) { sumX += p2X; aliveCount++; }
            targetX = (aliveCount > 0) ? (sumX / aliveCount) : 0;
            if (darkFar) targetX = (p1Coins <= p2Coins) ? p1X : p2X;
        } else {
            targetX = (p1Coins <= p2Coins) ? p1X : p2X;
        }
        if (isDarkMode && !darkFar) camX = targetX - W / 2;
        else {
            if (targetX - camX > W * 2 / 3) camX = targetX - W * 2 / 3;
            if (targetX - camX < W / 3) camX = targetX - W / 3;
        }
        if (camX < 0) camX = 0;
        if (camX > curMapW - W) camX = curMapW - W;
        int leftEdge = camX + 1, rightEdge = camX + W - 2;
        if (p1X < leftEdge && (isSolid(leftEdge, p1Y + 1) || isOneWay(leftEdge, p1Y + 1))) p1X = leftEdge;
        if (p1X > rightEdge && (isSolid(rightEdge, p1Y + 1) || isOneWay(rightEdge, p1Y + 1))) p1X = rightEdge;
        if (p2X < leftEdge && (isSolid(leftEdge, p2Y + 1) || isOneWay(leftEdge, p2Y + 1))) p2X = leftEdge;
        if (p2X > rightEdge && (isSolid(rightEdge, p2Y + 1) || isOneWay(rightEdge, p2Y + 1))) p2X = rightEdge;
    } else {
        if (p1X - camX > W * 2 / 3) camX = p1X - W * 2 / 3;
        if (p1X - camX < W / 3) camX = p1X - W / 3;
        if (camX < 0) camX = 0;
        if (camX > curMapW - W) camX = curMapW - W;
        if (p1X >= curMapW - 5) {
            if (isSpeedMode) { win = true; winType = 6; }
            else if (isCoinsMode) {
                win = true;
                if (collectedCoins >= totalCoins) winType = 2;
                else if (collectedCoins >= totalCoins / 2) winType = 1;
                else winType = 0;
            }
            else {
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

    if (menuState == 0) {
        screen += "                    [ SELECT MODE ]\n\n";
        screen += "                    1. Normal\n";
        screen += "                    2. Island\n";
        screen += "                    3. Tower\n\n\n";
        screen += "                       Q to Quit\n";
    } else if (menuState == 1) {
        screen += "                    [ SELECT MODIFIER ]\n\n";
        screen += (selectedTerrain == 0 ? "                     (Normal)\n\n" :
                   selectedTerrain == 1 ? "                     (Island)\n\n" :
                                          "                     (Tower)\n\n");
        screen += "                    1. None\n";
        screen += "                    2. Speed Rush\n";
        screen += "                    3. Moon Gravity\n";
        screen += "                    4. Dark Mode\n";
        screen += "                    5. Monster Mode\n";
        screen += "                    6. Coins Mode\n";
        screen += "                    7. Moving Platform\n\n";
        screen += "                       M to Back\n";
    } else if (menuState == 2) {
        string terrName = (selectedTerrain == 0 ? "Normal" : selectedTerrain == 1 ? "Island" : "Tower");
        string modName = (selectedModifier == 0 ? "None" : selectedModifier == 1 ? "Speed" : selectedModifier == 2 ? "Moon" : selectedModifier == 3 ? "Dark" : selectedModifier == 4 ? "Monster" : selectedModifier == 5 ? "Coins" : "Moving");
        screen += "                    [ SELECT PLAYERS ]\n\n";
        screen += "                  " + terrName + " / " + modName + "\n\n";
        screen += "                    1. Solo\n";
        screen += "                    2. Versus\n\n\n";
        screen += "                       M to Back\n";
    } else if (menuState == 3) {
        string terrName = (selectedTerrain == 0 ? "Normal" : selectedTerrain == 1 ? "Island" : "Tower");
        string modName = (selectedModifier == 0 ? "None" : selectedModifier == 1 ? "Speed" : selectedModifier == 2 ? "Moon" : selectedModifier == 3 ? "Dark" : selectedModifier == 4 ? "Monster" : selectedModifier == 5 ? "Coins" : "Moving");
        string playerName = (selectedPlayerCount == 1 ? "Solo" : "Versus");
        screen += "                    [ SELECT DIFFICULTY ]\n\n";
        screen += "                  " + terrName + " / " + modName + " / " + playerName + "\n\n";
        screen += "                    1. Easy\n";
        screen += "                    2. Hard\n\n\n";
        screen += "                       M to Back\n";
    }

    cout << screen;
}

void drawGameOverScreen() {
    string screen = "\033[2J\033[H";
    screen += "\033[38;2;255;255;255m";
    string title = gameOver ? "\033[38;2;255;50;50m      G A M E    O V E R      \033[0m"
                            : (winType == 0 ? "\033[38;2;0;255;0m         Y O U  W I N         \033[0m" :
                              (winType == 1 ? "\033[38;2;255;215;0m       C O M P L E T E !      \033[0m" : "\033[38;2;255;215;0m       P E R F E C T !        \033[0m"));
    if (is2PMode && win) {
        if (winType == 3) title = "\033[38;2;255;0;0m       M A R I O  W I N       \033[0m";
        else if (winType == 4) title = "\033[38;2;0;255;0m       L U I G I  W I N       \033[0m";
        else if (winType == 5) title = "\033[38;2;255;215;0m        D R A W  !  !         \033[0m";
    } else if (winType == 6) {
        title = "\033[38;2;255;215;0m       S P E E D  W I N       \033[0m";
    }
    string modeName = is2PMode ? "2-Player Battle" : (isIslandMode ? "Island" : "Normal");
    if (is2PMode) {
        if (isIslandMode) modeName = "2P Island";
        else if (isSpeedMode) modeName = "2P Speed";
        else if (isDarkMode) modeName = "2P Dark";
        else if (isMoonMode) modeName = "2P Moon";
        else if (isTowerMode) modeName = "2P Tower";
        else if (isMonsterMode) modeName = "2P Monster";
        else if (isCoinsMode) modeName = "2P Coins";
        else if (isMovingPlatformMode) modeName = "2P Moving";
    } else {
        if (isSpeedMode) modeName = "Speed Rush";
        if (isDarkMode) modeName = "Dark Mode";
        if (isMoonMode) modeName = "Moon Gravity";
        if (isTowerMode) modeName = "Tower";
        if (isMonsterMode) modeName = "Monster Mode";
        if (isCoinsMode) modeName = "Coins Mode";
        if (isMovingPlatformMode) modeName = "Moving Platform";
    }
    screen += "\n\n\n";
    screen += "              ==============================\n";
    screen += "              " + title + "\n";
    screen += "              ==============================\n\n";
    char stats[128];
    if (is2PMode) sprintf(stats, "              Mario: %d pts  |  Luigi: %d pts\n              Total Coins: %d/%d\n", p1Score, p2Score, collectedCoins, totalCoins);
    else sprintf(stats, "                 Mode: [%s]\n                 Score: %d | Coins: %d/%d\n", modeName.c_str(), score, collectedCoins, totalCoins);
    screen += stats;

    screen += "\n              \033[38;2;0;255;0m[SPACE]\033[0m Restart (Current Mode)\n";
    screen += "              \033[38;2;255;215;0m[M]\033[0m Main Menu   \033[38;2;255;215;0m[Q]\033[0m Quit Game\n\n";

    cout << screen;
}

void drawGame() {
    string screen = "\033[H";

    bool p1Visible, p2Visible;

    for (int y = 0; y < H; y++) {
        int wy = y + camY;
        for (int x = 0; x < W; x++) {
            int wx = x + camX;
            if (wx < 0 || wx >= curMapW || wy < 0 || wy >= curMapH) { screen += " "; continue; }

            if (isDarkMode) {
                p1Visible = abs(wx - p1X) <= 6 && abs(wy - p1Y) <= 4;
                p2Visible = is2PMode && p2Alive && abs(wx - p2X) <= 6 && abs(wy - p2Y) <= 4;
                if (!p1Visible && !p2Visible) { screen += " "; continue; }
            }

            bool printed = false;
            if (p1Alive && p1X == wx && p1Y == wy) {
                if (p1Inv == 0 || (p1Inv > 0 && gameTicks % 4 < 2)) screen += "\033[38;2;255;0;0mM\033[0m";
                else screen += " ";
                printed = true;
            }
            else if (is2PMode && p2Alive && p2X == wx && p2Y == wy) {
                if (p2Inv == 0 || (p2Inv > 0 && gameTicks % 4 < 2)) screen += "\033[38;2;0;255;0mL\033[0m";
                else screen += " ";
                printed = true;
            }
            else if (!p1Alive && p1X == wx && p1Y - 1 == wy) { screen += "X"; printed = true; }
            else if (is2PMode && !p2Alive && p2X == wx && p2Y - 1 == wy) { screen += "X"; printed = true; }

            if (printed) continue;

            bool splatPrinted = false;
            for (int i = 0; i < splatCount; i++) {
                if (splats[i].x == wx && splats[i].y == wy) { screen += "\033[38;2;255;255;0m+\033[0m"; splatPrinted = true; break; }
            }
            if (splatPrinted) continue;

            bool mPrinted = false;
            for (int i = 0; i < monsterCount; i++) {
                if (monsters[i].alive && monsters[i].x == wx && monsters[i].y == wy) { screen += "\033[38;2;0;150;255mG\033[0m"; mPrinted = true; break; }
            }
            if (mPrinted) continue;

            bool mpPrinted = false;
            for (int i = 0; i < mpCount; i++) {
                if (wy == mps[i].y && wx >= mps[i].x && wx < mps[i].x + mps[i].len) {
                    screen += "\033[38;2;255;0;255m*\033[0m";
                    mpPrinted = true; break;
                }
            }
            if (mpPrinted) continue;

            bool platCoinPrinted = false;
            for (int i = 0; i < mpCount; i++) {
                if (!mps[i].hasCoin) continue;
                if (mps[i].y - 1 != wy) continue;
                if (wx < mps[i].x || wx >= mps[i].x + mps[i].len) continue;
                int bit = wx - mps[i].x;
                if (mps[i].coinMask & (1 << bit)) {
                    screen += "\033[38;2;255;215;0mo\033[0m";
                    platCoinPrinted = true; break;
                }
            }
            if (platCoinPrinted) continue;

            char c = map[wy][wx];
            if (c == '#') screen += "\033[38;2;139;69;19m#\033[0m";
            else if (c == '=') screen += "\033[38;2;255;165;0m=\033[0m";
            else if (c == 'o') screen += "\033[38;2;255;215;0mo\033[0m";
            else if (c == 'S') screen += "\033[38;2;255;215;0m\xE2\x98\x86\033[0m";
            else screen += c;
        }
        screen += "\033[K\n";
    }

    char hud[200];
    if (isTowerMode) {
        if (isSpeedMode) {
            if (is2PMode) sprintf(hud, "\033[38;2;255;50;50mTIME: %.1fs\033[0m | 2P Tower | Mario: %d | Luigi: %d | Coins: %d/%d | Kills: %d/%d",
                                  timeLeft / 25.0, p1Score, p2Score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
            else sprintf(hud, "\033[38;2;255;50;50mTIME: %.1fs\033[0m | Tower | Score: %d | Coins: %d/%d | Kills: %d/%d  Climb to the Star!",
                         timeLeft / 25.0, score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
        } else if (is2PMode) {
            sprintf(hud, "2P Tower | Mario: %d | Luigi: %d | Coins: %d/%d | Kills: %d/%d",
                    p1Score, p2Score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
        } else {
            sprintf(hud, "Tower | Score: %d | Coins: %d/%d | Kills: %d/%d  Climb to the Star!",
                    score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
        }
    } else if (is2PMode) {
        if (isSpeedMode) sprintf(hud, "\033[38;2;255;50;50mTIME: %.1fs\033[0m | Mario: %d | Luigi: %d | Coins: %d/%d", timeLeft / 25.0, p1Score, p2Score, collectedCoins, totalCoins);
        else if (isMonsterMode) sprintf(hud, "[Monster] Mario: %d | Luigi: %d | Coins: %d/%d | Kills: %d/%d", p1Score, p2Score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
        else if (isCoinsMode) sprintf(hud, "[Coins] Mario: %d | Luigi: %d | Coins: %d/%d", p1Score, p2Score, collectedCoins, totalCoins);
        else if (isMovingPlatformMode) sprintf(hud, "[Moving] Mario: %d | Luigi: %d | Coins: %d/%d | Kills: %d/%d", p1Score, p2Score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
        else sprintf(hud, "Mario: %d pts | Luigi: %d pts | Coins: %d/%d", p1Score, p2Score, collectedCoins, totalCoins);
    }
    else if (isSpeedMode) sprintf(hud, "\033[38;2;255;50;50mTIME: %.1fs\033[0m | Score: %d | Coins: %d/%d", timeLeft / 25.0, score, collectedCoins, totalCoins);
    else if (isMonsterMode) sprintf(hud, "[Monster] Score: %d | Coins: %d/%d | Kills: %d/%d",
                score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
    else if (isCoinsMode) sprintf(hud, "[Coins] Score: %d | Coins: %d/%d",
                score, collectedCoins, totalCoins);
    else if (isMovingPlatformMode) sprintf(hud, "[Moving] Score: %d | Coins: %d/%d | Kills: %d/%d",
                score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
    else sprintf(hud, "[%s] Score: %d/800 | Coins: %d/%d | Kills: %d/%d",
                isDarkMode ? "Dark" : (isMoonMode ? "Moon" : (isIslandMode ? "Island" : "Normal")),
                score, collectedCoins, totalCoins, monstersKilled, totalMonsters);
    screen += "\033[38;2;255;255;255m";
    screen += hud; screen += "\033[K\n";
    cout << screen;
}

void startGameFromMenu() {
    is2PMode = (selectedPlayerCount == 2);
    isIslandMode = (selectedTerrain == 1);
    isTowerMode = (selectedTerrain == 2);
    isSpeedMode = (selectedModifier == 1);
    isMoonMode = (selectedModifier == 2);
    isDarkMode = (selectedModifier == 3);
    isMonsterMode = (selectedModifier == 4);
    isCoinsMode = (selectedModifier == 5);
    isMovingPlatformMode = (selectedModifier == 6);
    gameStarted = true;
}

int main() {
    srand(time(0));
    initConsole();
    initAudio();
    initMap();

    while (true) {
        gameStarted = false;

        while (!gameStarted) {
            playBGM("menu.wav");
            drawMenu();

            if (menuState == 0) {
                if (keyDown('1')) { selectedTerrain = 0; menuState = 1; Sleep(120); }
                else if (keyDown('2')) { selectedTerrain = 1; menuState = 1; Sleep(120); }
                else if (keyDown('3')) { selectedTerrain = 2; menuState = 1; Sleep(120); }
                else if (keyDown('Q') || keyDown('q')) { stopBGM(); return 0; }
            }
            else if (menuState == 1) {
                if (keyDown('1')) { selectedModifier = 0; menuState = 2; Sleep(120); }
                else if (keyDown('2')) { selectedModifier = 1; menuState = 2; Sleep(120); }
                else if (keyDown('3')) { selectedModifier = 2; menuState = 2; Sleep(120); }
                else if (keyDown('4')) { selectedModifier = 3; menuState = 2; Sleep(120); }
                else if (keyDown('5')) { selectedModifier = 4; menuState = 2; Sleep(120); }
                else if (keyDown('6')) { selectedModifier = 5; menuState = 2; Sleep(120); }
                else if (keyDown('7')) { selectedModifier = 6; menuState = 2; Sleep(120); }
                else if (keyDown('M') || keyDown('m')) { menuState = 0; Sleep(120); }
            }
            else if (menuState == 2) {
                if (keyDown('1')) { selectedPlayerCount = 1; menuState = 3; Sleep(120); }
                else if (keyDown('2')) { selectedPlayerCount = 2; menuState = 3; Sleep(120); }
                else if (keyDown('M') || keyDown('m')) { menuState = 1; Sleep(120); }
            }
            else if (menuState == 3) {
                if (keyDown('1')) { isEasyMode = true; startGameFromMenu(); }
                else if (keyDown('2')) { isEasyMode = false; startGameFromMenu(); }
                else if (keyDown('M') || keyDown('m')) { menuState = 2; Sleep(120); }
            }

            Sleep(50);
        }

        resetGame();
        bool lastGameOverState = false;

        while (gameStarted) {
            update();
            if (!gameStarted) break;

            if (!gameOver && !win) {
                drawGame();
                lastGameOverState = false;
            } else if (!lastGameOverState) {
                playBGM("gameover.wav");
                drawGameOverScreen();
                lastGameOverState = true;
            }
            Sleep(40);
        }
    }
    return 0;
}
