#include <bits/stdc++.h>
#include <windows.h>
using namespace std;

// ==================== 数学库 ====================
struct Vec3 {
    float x, y, z;
    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vec3 normalize() const { float l = length(); return l > 0.0001f ? Vec3(x / l, y / l, z / l) : Vec3(0, 0, 0); }
};

struct Mat4 {
    float m[4][4];
    Mat4() { memset(m, 0, sizeof(m)); }
    static Mat4 identity() {
        Mat4 r;
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1;
        return r;
    }
    static Mat4 perspective(float fov, float aspect, float nearPlane, float farPlane) {
        Mat4 r;
        float f = 1.0f / tanf(fov * 0.5f);
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
        r.m[2][3] = -1;
        r.m[3][2] = (2 * farPlane * nearPlane) / (nearPlane - farPlane);
        return r;
    }
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalize();
        Vec3 s = f.cross(up).normalize();
        Vec3 u = s.cross(f);
        Mat4 r = identity();
        r.m[0][0] = s.x; r.m[0][1] = s.y; r.m[0][2] = s.z;
        r.m[1][0] = u.x; r.m[1][1] = u.y; r.m[1][2] = u.z;
        r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z;
        r.m[3][0] = -s.dot(eye);
        r.m[3][1] = -u.dot(eye);
        r.m[3][2] = f.dot(eye);
        return r;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                r.m[i][j] = 0;
                for (int k = 0; k < 4; k++) r.m[i][j] += m[i][k] * o.m[k][j];
            }
        return r;
    }
};

float radians(float deg) { return deg * 3.14159265358979323846f / 180.0f; }

// ==================== 常量与全局 ====================
const int CHUNK_SIZE = 16;
const int WORLD_HEIGHT = 32;
const int RENDER_DISTANCE = 3;
const float PLAYER_HEIGHT = 1.8f;
const float PLAYER_RADIUS = 0.3f;
const float GRAVITY = -20.0f;
const float JUMP_SPEED = 7.5f;
const float MOVE_SPEED = 5.0f;

enum BlockType : uint8_t { AIR = 0, GRASS, DIRT, STONE, WOOD, LEAVES, COAL, IRON, DIAMOND, BEDROCK };

int g_width = 1280, g_height = 720;
vector<uint32_t> g_pixels;
vector<float> g_depth;

HDC g_memDC = nullptr;
HBITMAP g_memBitmap = nullptr;
HGDIOBJ g_oldBitmap = nullptr;
HWND g_hwnd = nullptr;

Vec3 playerPos(0, 20, 0);
float yaw = -90.0f, pitch = 0.0f;
Vec3 velocity(0, 0, 0);
bool onGround = false;
bool keys[256] = {false};
bool mouseLeftPressed = false, mouseRightPressed = false;
bool hasTarget = false;
int targetBX, targetBY, targetBZ;
int normalDX, normalDY, normalDZ;

struct Chunk {
    int cx, cz;
    uint8_t blocks[CHUNK_SIZE][WORLD_HEIGHT][CHUNK_SIZE];
    Chunk(int x, int z) : cx(x), cz(z) { memset(blocks, 0, sizeof(blocks)); }
};

unordered_map<long long, Chunk*> chunks;
long long chunkKey(int cx, int cz) { return ((long long)cx << 32) | (unsigned int)cz; }

// ==================== 噪声与地形 ====================
float noise2D(int x, int z) {
    uint32_t n = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return ((n ^ (n >> 16)) & 0x7fffffffu) / (float)0x7fffffffu;
}

float smoothNoise(float x, float z) {
    int ix = (int)floorf(x), iz = (int)floorf(z);
    float fx = x - ix, fz = z - iz;
    fx = fx * fx * (3 - 2 * fx);
    fz = fz * fz * (3 - 2 * fz);
    float a = noise2D(ix, iz);
    float b = noise2D(ix + 1, iz);
    float c = noise2D(ix, iz + 1);
    float d = noise2D(ix + 1, iz + 1);
    return a * (1 - fx) * (1 - fz) + b * fx * (1 - fz) + c * (1 - fx) * fz + d * fx * fz;
}

int getHeight(int wx, int wz) {
    float n = smoothNoise(wx * 0.05f, wz * 0.05f) * 0.6f
            + smoothNoise(wx * 0.1f, wz * 0.1f) * 0.3f
            + smoothNoise(wx * 0.2f, wz * 0.2f) * 0.1f;
    int h = (int)(n * 18) + 5;
    if (h < 1) h = 1;
    if (h >= WORLD_HEIGHT) h = WORLD_HEIGHT - 1;
    return h;
}

void generateChunk(Chunk* c) {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int wx = c->cx * CHUNK_SIZE + x;
            int wz = c->cz * CHUNK_SIZE + z;
            int h = getHeight(wx, wz);
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                uint8_t b = AIR;
                if (y == 0) b = BEDROCK;
                else if (y < h - 4) b = STONE;
                else if (y < h - 1) b = DIRT;
                else if (y == h - 1) b = GRASS;
                if (b == STONE) {
                    int r = rand() % 1000;
                    if (y < h - 12 && r < 2) b = DIAMOND;
                    else if (y < h - 8 && r < 8) b = IRON;
                    else if (y < h - 4 && r < 20) b = COAL;
                }
                c->blocks[x][y][z] = b;
            }
        }
    }
}

// 仅查找已加载区块，不生成
Chunk* findChunk(int cx, int cz) {
    auto it = chunks.find(chunkKey(cx, cz));
    return it != chunks.end() ? it->second : nullptr;
}

Chunk* getChunk(int cx, int cz) {
    long long key = chunkKey(cx, cz);
    auto it = chunks.find(key);
    if (it != chunks.end()) return it->second;
    Chunk* c = new Chunk(cx, cz);
    generateChunk(c);
    chunks[key] = c;
    return c;
}

// 未加载区块返回 AIR，避免隐式生成
uint8_t getBlock(int wx, int wy, int wz) {
    if (wy < 0 || wy >= WORLD_HEIGHT) return AIR;
    int cx = wx >> 4, cz = wz >> 4;
    Chunk* c = findChunk(cx, cz);
    if (!c) return AIR;
    return c->blocks[wx & 15][wy][wz & 15];
}

void setBlock(int wx, int wy, int wz, uint8_t type) {
    if (wy < 0 || wy >= WORLD_HEIGHT) return;
    int cx = wx >> 4, cz = wz >> 4;
    Chunk* c = getChunk(cx, cz);
    c->blocks[wx & 15][wy][wz & 15] = type;
}

bool isSolid(uint8_t b) { return b != AIR; }

// ==================== 颜色 ====================
uint32_t blockColor(uint8_t b, int face) {
    float r, g, bl;
    switch (b) {
        case GRASS:   r = 0.3f; g = 0.7f; bl = 0.2f; break;
        case DIRT:    r = 0.5f; g = 0.3f; bl = 0.15f; break;
        case STONE:   r = 0.5f; g = 0.5f; bl = 0.5f; break;
        case WOOD:    r = 0.4f; g = 0.25f; bl = 0.1f; break;
        case LEAVES:  r = 0.1f; g = 0.5f; bl = 0.1f; break;
        case COAL:    r = 0.15f; g = 0.15f; bl = 0.15f; break;
        case IRON:    r = 0.7f; g = 0.6f; bl = 0.5f; break;
        case DIAMOND: r = 0.2f; g = 0.9f; bl = 0.9f; break;
        case BEDROCK: r = 0.1f; g = 0.1f; bl = 0.1f; break;
        default:      r = 1.0f; g = 0.0f; bl = 1.0f; break;
    }
    if (face != 2) {
        if (face == 3) { r *= 0.5f; g *= 0.5f; bl *= 0.5f; }
        else { r *= 0.75f; g *= 0.75f; bl *= 0.75f; }
    }
    int ri = (int)(r * 255), gi = (int)(g * 255), bi = (int)(bl * 255);
    if (ri > 255) ri = 255; if (gi > 255) gi = 255; if (bi > 255) bi = 255;
    return (ri << 16) | (gi << 8) | bi;
}

// ==================== 光栅化 ====================
void putPixel(int x, int y, float depth, uint32_t color) {
    if (x < 0 || x >= g_width || y < 0 || y >= g_height) return;
    int idx = y * g_width + x;
    if (depth < g_depth[idx]) {
        g_depth[idx] = depth;
        g_pixels[idx] = color;
    }
}

// 透视正确的逐像素深度插值
void rasterizeTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                       const float z0, const float z1, const float z2,
                       uint32_t color) {
    float x0 = v0.x, y0 = v0.y;
    float x1 = v1.x, y1 = v1.y;
    float x2 = v2.x, y2 = v2.y;

    int minX = (int)floorf(std::min(x0, std::min(x1, x2)));
    int maxX = (int)ceilf(std::max(x0, std::max(x1, x2)));
    int minY = (int)floorf(std::min(y0, std::min(y1, y2)));
    int maxY = (int)ceilf(std::max(y0, std::max(y1, y2)));
    minX = std::max(0, minX); maxX = std::min(g_width - 1, maxX);
    minY = std::max(0, minY); maxY = std::min(g_height - 1, maxY);

    float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (fabsf(area) < 0.0001f) return;
    float invArea = 1.0f / area;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) * invArea;
            float w1 = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) * invArea;
            float w2 = ((x0 - px) * (y1 - py) - (x1 - px) * (y0 - py)) * invArea;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                float depth = w0 * z0 + w1 * z1 + w2 * z2;
                putPixel(x, y, depth, color);
            }
        }
    }
}

void clearBuffers() {
    uint32_t sky = 0x87CEEB;
    for (int i = 0; i < g_width * g_height; i++) {
        g_pixels[i] = sky;
        g_depth[i] = 1e30f;
    }
}

// ==================== 渲染 ====================
void renderWorld() {
    clearBuffers();

    Vec3 forward(cosf(radians(yaw)) * cosf(radians(pitch)),
                 sinf(radians(pitch)),
                 sinf(radians(yaw)) * cosf(radians(pitch)));
    forward = forward.normalize();
    Vec3 eye = playerPos + Vec3(0, 1.6f, 0);
    Vec3 center = eye + forward;
    Vec3 up(0, 1, 0);
    Mat4 view = Mat4::lookAt(eye, center, up);
    Mat4 proj = Mat4::perspective(radians(75.0f), (float)g_width / g_height, 0.1f, 500.0f);
    Mat4 mvp = proj * view;

    int pcx = (int)floorf(playerPos.x) >> 4;
    int pcz = (int)floorf(playerPos.z) >> 4;

    static const int faceDirs[6][3] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    static const float faceVerts[6][4][3] = {
        {{1,0,0},{1,0,1},{1,1,1},{1,1,0}},
        {{0,0,1},{0,0,0},{0,1,0},{0,1,1}},
        {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
        {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},
        {{1,0,1},{0,0,1},{0,1,1},{1,1,1}},
        {{0,0,0},{1,0,0},{1,1,0},{0,1,0}}
    };

    float maxDist = RENDER_DISTANCE * CHUNK_SIZE;

    for (auto& pair : chunks) {
        Chunk* c = pair.second;
        int dcx = abs(c->cx - pcx);
        int dcz = abs(c->cz - pcz);
        if (dcx > RENDER_DISTANCE + 1 || dcz > RENDER_DISTANCE + 1) continue;

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t block = c->blocks[x][y][z];
                    if (block == AIR) continue;
                    int wx = c->cx * CHUNK_SIZE + x;
                    int wz = c->cz * CHUNK_SIZE + z;
                    float dx = wx - playerPos.x;
                    float dz = wz - playerPos.z;
                    if (dx * dx + dz * dz > maxDist * maxDist) continue;

                    for (int d = 0; d < 6; d++) {
                        int nx = wx + faceDirs[d][0];
                        int ny = y + faceDirs[d][1];
                        int nz = wz + faceDirs[d][2];
                        if (isSolid(getBlock(nx, ny, nz))) continue;

                        uint32_t color = blockColor(block, d);

                        Vec3 clip[4];
                        float cw[4];
                        for (int v = 0; v < 4; v++) {
                            Vec3 p(wx + faceVerts[d][v][0],
                                   y  + faceVerts[d][v][1],
                                   wz + faceVerts[d][v][2]);
                            clip[v] = Vec3(
                                mvp.m[0][0]*p.x + mvp.m[0][1]*p.y + mvp.m[0][2]*p.z + mvp.m[0][3],
                                mvp.m[1][0]*p.x + mvp.m[1][1]*p.y + mvp.m[1][2]*p.z + mvp.m[1][3],
                                mvp.m[2][0]*p.x + mvp.m[2][1]*p.y + mvp.m[2][2]*p.z + mvp.m[2][3]);
                            cw[v] = mvp.m[3][0]*p.x + mvp.m[3][1]*p.y + mvp.m[3][2]*p.z + mvp.m[3][3];
                        }

                        bool anyInside = false;
                        for (int v = 0; v < 4; v++) if (cw[v] > 0.001f) anyInside = true;
                        if (!anyInside) continue;

                        Vec3 screen[4];
                        float ndcZ[4];
                        bool valid[4] = {false, false, false, false};
                        for (int v = 0; v < 4; v++) {
                            if (cw[v] > 0.001f) {
                                float invW = 1.0f / cw[v];
                                float ndcX = clip[v].x * invW;
                                float ndcY = clip[v].y * invW;
                                screen[v] = Vec3((1.0f-(ndcX * 0.5f + 0.5f)) * g_width,
                                                 ((ndcY * 0.5f + 0.5f))* g_height,
                                                 0.0f);
                                ndcZ[v] = clip[v].z * invW;
                                valid[v] = true;
                            }
                        }
                        if (!(valid[0] && valid[1] && valid[2] && valid[3])) continue;

                        rasterizeTriangle(screen[0], screen[1], screen[2],
                                          ndcZ[0], ndcZ[1], ndcZ[2], color);
                        rasterizeTriangle(screen[0], screen[2], screen[3],
                                          ndcZ[0], ndcZ[2], ndcZ[3], color);
                    }
                }
            }
        }
    }
}

// ==================== 碰撞与移动 ====================
bool checkCollision(Vec3 pos) {
    float minX = pos.x - PLAYER_RADIUS;
    float maxX = pos.x + PLAYER_RADIUS;
    float minY = pos.y;
    float maxY = pos.y + PLAYER_HEIGHT;
    float minZ = pos.z - PLAYER_RADIUS;
    float maxZ = pos.z + PLAYER_RADIUS;

    for (int x = (int)floorf(minX); x <= (int)floorf(maxX); x++)
        for (int y = (int)floorf(minY); y <= (int)floorf(maxY); y++)
            for (int z = (int)floorf(minZ); z <= (int)floorf(maxZ); z++) {
                if (isSolid(getBlock(x, y, z))) {
                    if (maxX > x && minX < x + 1 &&
                        maxY > y && minY < y + 1 &&
                        maxZ > z && minZ < z + 1)
                        return true;
                }
            }
    return false;
}

void moveWithCollision(Vec3 delta) {
    Vec3 np = playerPos + Vec3(delta.x, 0, 0);
    if (!checkCollision(np)) playerPos.x = np.x;

    np = playerPos + Vec3(0, 0, delta.z);
    if (!checkCollision(np)) playerPos.z = np.z;

    np = playerPos + Vec3(0, delta.y, 0);
    if (!checkCollision(np)) {
        playerPos.y = np.y;
        if (delta.y < 0) onGround = false;
    } else {
        if (delta.y < 0) { onGround = true; velocity.y = 0; }
        else if (delta.y > 0) velocity.y = 0;
    }
}

// ==================== Amanatides & Woo 体素射线 ====================
void raycast() {
    hasTarget = false;

    Vec3 dir(cosf(radians(yaw)) * cosf(radians(pitch)),
             sinf(radians(pitch)),
             sinf(radians(yaw)) * cosf(radians(pitch)));
    dir = dir.normalize();
    Vec3 origin = playerPos + Vec3(0, 1.6f, 0);

    int bx = (int)floorf(origin.x);
    int by = (int)floorf(origin.y);
    int bz = (int)floorf(origin.z);

    int stepX = (dir.x > 0) ? 1 : (dir.x < 0 ? -1 : 0);
    int stepY = (dir.y > 0) ? 1 : (dir.y < 0 ? -1 : 0);
    int stepZ = (dir.z > 0) ? 1 : (dir.z < 0 ? -1 : 0);

    float tMaxX = (stepX != 0) ? ((bx + (stepX > 0 ? 1 : 0) - origin.x) / dir.x) : 1e30f;
    float tMaxY = (stepY != 0) ? ((by + (stepY > 0 ? 1 : 0) - origin.y) / dir.y) : 1e30f;
    float tMaxZ = (stepZ != 0) ? ((bz + (stepZ > 0 ? 1 : 0) - origin.z) / dir.z) : 1e30f;

    float tDeltaX = (stepX != 0) ? fabsf(1.0f / dir.x) : 1e30f;
    float tDeltaY = (stepY != 0) ? fabsf(1.0f / dir.y) : 1e30f;
    float tDeltaZ = (stepZ != 0) ? fabsf(1.0f / dir.z) : 1e30f;

    int faceNX = 0, faceNY = 0, faceNZ = 0;

    for (int i = 0; i < 200; i++) {
        if (isSolid(getBlock(bx, by, bz))) {
            hasTarget = true;
            targetBX = bx; targetBY = by; targetBZ = bz;
            normalDX = faceNX; normalDY = faceNY; normalDZ = faceNZ;
            return;
        }
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                if (tMaxX > 6.0f) break;
                bx += stepX; tMaxX += tDeltaX;
                faceNX = -stepX; faceNY = 0; faceNZ = 0;
            } else {
                if (tMaxZ > 6.0f) break;
                bz += stepZ; tMaxZ += tDeltaZ;
                faceNX = 0; faceNY = 0; faceNZ = -stepZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                if (tMaxY > 6.0f) break;
                by += stepY; tMaxY += tDeltaY;
                faceNX = 0; faceNY = -stepY; faceNZ = 0;
            } else {
                if (tMaxZ > 6.0f) break;
                bz += stepZ; tMaxZ += tDeltaZ;
                faceNX = 0; faceNY = 0; faceNZ = -stepZ;
            }
        }
    }
}

// ==================== 窗口与消息 ====================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            if (wParam < 256) keys[wParam] = true;
            break;
        case WM_KEYUP:
            if (wParam < 256) keys[wParam] = false;
            break;
        case WM_LBUTTONDOWN:
            mouseLeftPressed = true;
            break;
        case WM_LBUTTONUP:
            mouseLeftPressed = false;
            break;
        case WM_RBUTTONDOWN:
            mouseRightPressed = true;
            break;
        case WM_RBUTTONUP:
            mouseRightPressed = false;
            break;
        case WM_MOUSEMOVE: {
            // 用光标相对客户区中心计算位移
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            RECT rc;
            GetClientRect(hwnd, &rc);
            POINT center = {(rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2};
            int dx = p.x - center.x;
            int dy = p.y - center.y;
            if (dx || dy) {
                yaw += dx * 0.2f;
                pitch -= dy * 0.2f;
                if (pitch >  89) pitch =  89;
                if (pitch < -89) pitch = -89;
                POINT sc = center;
                ClientToScreen(hwnd, &sc);
                SetCursorPos(sc.x, sc.y);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void presentFrame() {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_width;
    bmi.bmiHeader.biHeight = g_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetDIBitsToDevice(g_memDC, 0, 0, g_width, g_height, 0, 0, 0, g_height,
                      g_pixels.data(), &bmi, DIB_RGB_COLORS);

    HDC hdc = GetDC(g_hwnd);
    BitBlt(hdc, 0, 0, g_width, g_height, g_memDC, 0, 0, SRCCOPY);
    ReleaseDC(g_hwnd, hdc);
}

// ==================== 区块卸载 ====================
void unloadFarChunks(int pcx, int pcz) {
    const int UNLOAD_DIST = RENDER_DISTANCE + 2;
    vector<long long> toRemove;
    for (auto& pair : chunks) {
        int dcx = abs(pair.second->cx - pcx);
        int dcz = abs(pair.second->cz - pcz);
        if (dcx > UNLOAD_DIST || dcz > UNLOAD_DIST)
            toRemove.push_back(pair.first);
    }
    for (long long k : toRemove) {
        delete chunks[k];
        chunks.erase(k);
    }
}

// ==================== main ====================
int main() {
    srand((unsigned)time(0));

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "Minecraft3D";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    RECT rc = {0, 0, g_width, g_height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindow("Minecraft3D", "Beta Minecraft 3D (Win32)",
                          WS_OVERLAPPEDWINDOW, 100, 100,
                          rc.right - rc.left, rc.bottom - rc.top,
                          nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return -1;

    g_pixels.resize(g_width * g_height);
    g_depth.resize(g_width * g_height);

    HDC hdc = GetDC(g_hwnd);
    g_memDC = CreateCompatibleDC(hdc);
    g_memBitmap = CreateCompatibleBitmap(hdc, g_width, g_height);
    g_oldBitmap = SelectObject(g_memDC, g_memBitmap);
    ReleaseDC(g_hwnd, hdc);

    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);

    // 鼠标锁在窗口中心
    {
        RECT r;
        GetClientRect(g_hwnd, &r);
        POINT center = {(r.right - r.left) / 2, (r.bottom - r.top) / 2};
        ClientToScreen(g_hwnd, &center);
        SetCursorPos(center.x, center.y);
    }

    int startH = getHeight(0, 0);
    playerPos = Vec3(0.5f, startH + 0.1f, 0.5f);

    LARGE_INTEGER freq, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);

    int frameCount = 0;
    double fpsTime = 0;
    MSG msg;
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)(now.QuadPart - last.QuadPart) / freq.QuadPart;
        last = now;
        if (dt > 0.1f) dt = 0.1f;

        if (keys[VK_ESCAPE]) break;

        // 移动
        Vec3 forward(cosf(radians(yaw)) * cosf(radians(pitch)),
                     sinf(radians(pitch)),
                     sinf(radians(yaw)) * cosf(radians(pitch)));
        forward = forward.normalize();
        Vec3 right = forward.cross(Vec3(0, 1, 0)).normalize();
        Vec3 flatForward = Vec3(forward.x, 0, forward.z).normalize();
        Vec3 flatRight = Vec3(right.x, 0, right.z).normalize();

        Vec3 move(0, 0, 0);
        if (keys['W']) move += flatForward;
        if (keys['S']) move -= flatForward;
        if (keys['D']) move += flatRight;
        if (keys['A']) move -= flatRight;

        if (keys[VK_SPACE] && onGround) {
            velocity.y = JUMP_SPEED;
            onGround = false;
        }

        if (move.length() > 0) move = move.normalize();
        velocity.y += GRAVITY * dt;
        Vec3 delta = move * MOVE_SPEED * dt;
        delta.y = velocity.y * dt;
        moveWithCollision(delta);

        if (playerPos.y < -20) {
            playerPos = Vec3(0.5f, getHeight(0, 0) + 1, 0.5f);
            velocity = Vec3(0, 0, 0);
        }

        int pcx = (int)floorf(playerPos.x) >> 4;
        int pcz = (int)floorf(playerPos.z) >> 4;
        for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++)
            for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++)
                getChunk(pcx + dx, pcz + dz);

        unloadFarChunks(pcx, pcz);

        raycast();

        if (mouseLeftPressed && hasTarget) {
            setBlock(targetBX, targetBY, targetBZ, AIR);
            mouseLeftPressed = false;
        }
        if (mouseRightPressed && hasTarget) {
            int nx = targetBX + normalDX;
            int ny = targetBY + normalDY;
            int nz = targetBZ + normalDZ;
            if (getBlock(nx, ny, nz) == AIR) setBlock(nx, ny, nz, STONE);
            mouseRightPressed = false;
        }

        renderWorld();
        presentFrame();

        frameCount++;
        fpsTime += dt;
        if (fpsTime >= 1.0) {
            string title = "Beta Minecraft 3D | FPS: " + to_string((int)(frameCount / fpsTime));
            SetWindowText(g_hwnd, title.c_str());
            frameCount = 0;
            fpsTime = 0;
        }

        // 空闲时让出 CPU；有消息则立即唤醒
        if (!PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
            WaitMessage();
    }

    for (auto& pair : chunks) delete pair.second;
    chunks.clear();

    SelectObject(g_memDC, g_oldBitmap);
    DeleteObject(g_memBitmap);
    DeleteDC(g_memDC);

    return 0;
}
