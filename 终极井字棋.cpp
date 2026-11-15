#include <iostream>
#include <array>
#include <vector>
#include <limits>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <random>
#include <cstring>
#include <string>
#include <utility>

using namespace std;
using Clock = chrono::steady_clock;

struct Move { int sb; int idx; };

const int LINES[8][3] = {
    {0,1,2}, {3,4,5}, {6,7,8},
    {0,3,6}, {1,4,7}, {2,5,8},
    {0,4,8}, {2,4,6}
};

const int CELL_W[9] = {3, 2, 3, 2, 4, 2, 3, 2, 3};

const int COMPUTER = 1;
const int HUMAN    = 2;
const int INF      = 1000000000;
const int TIMEOUT_VAL = INF + 1;
const int WIN_SCORE = 10000000;
const int WIN_BIG_BONUS = 10000000;

const int    MAX_DEPTH      = 82;
const int    TT_SIZE = 1 << 18;
const int    TT_MASK = TT_SIZE - 1;
const unsigned char TT_EMPTY = 0;
const unsigned char TT_EXACT = 1;
const unsigned char TT_LOWER = 2;
const unsigned char TT_UPPER = 3;

struct TTEntry {
    unsigned long long key;
    int value;
    int depth;
    unsigned char flag;
    TTEntry() : key(0), value(0), depth(0), flag(TT_EMPTY) {}
};

vector<TTEntry> tt(TT_SIZE);

unsigned long long zCell[9][9][2];
unsigned long long zSbw[9][4];
unsigned long long zForced[9];
unsigned long long g_hash = 0;

void initZobrist() {
    std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);
    for (int b = 0; b < 9; ++b)
        for (int i = 0; i < 9; ++i)
            for (int p = 0; p < 2; ++p)
                zCell[b][i][p] = rng();
    for (int b = 0; b < 9; ++b)
        for (int r = 1; r < 4; ++r)
            zSbw[b][r] = rng();
    for (int f = 0; f < 9; ++f)
        zForced[f] = rng();
}

array<array<char, 9>, 9> cell;
array<int, 9> sbw;
int forcedBoard = -1;
int bigWinner = 0;
bool gameOver = false;

Clock::time_point g_deadline;
bool g_timeout = false;
long long g_nodes = 0;

std::mt19937 rngReal((unsigned)std::random_device{}());

// ========== 难度参数 ==========
struct Difficulty {
    const char* name;
    double timeLimit;
    int rootMoves;
    int qDepth;
    bool useTT;
    bool useQuiescence;
    int randomBlunderPercent; // 随机漏着概率，越大越弱
};

const Difficulty DIFFS[4] = {
    {"Easy",       0.02,  1,  0, false, false, 60},
    {"Medium",     0.10,  3,  2, false, true,  20},
    {"Hard",       0.50,  6,  4, true,  true,  5 },
    {"Impossible", 3.00,  9,  4, true,  true,  0 }
};

int g_diffIndex = 3;
Difficulty g_diff = DIFFS[3];

enum class NextBoardMode {
    Random,
    SearchFree
};

struct State {
    array<array<char, 9>, 9> cell;
    array<int, 9> sbw;
    int forcedBoard;
    int bigWinner;
    bool gameOver;
    unsigned long long hash;
};

State saveState() {
    State s;
    s.cell = cell;
    s.sbw = sbw;
    s.forcedBoard = forcedBoard;
    s.bigWinner = bigWinner;
    s.gameOver = gameOver;
    s.hash = g_hash;
    return s;
}

void restoreState(const State& s) {
    cell = s.cell;
    sbw = s.sbw;
    forcedBoard = s.forcedBoard;
    bigWinner = s.bigWinner;
    gameOver = s.gameOver;
    g_hash = s.hash;
}

int checkSmall(int b) {
    for (int i = 0; i < 8; ++i) {
        char a = cell[b][LINES[i][0]];
        char c = cell[b][LINES[i][1]];
        char d = cell[b][LINES[i][2]];
        if (a != '.' && a == c && c == d) return a == 'X' ? 1 : 2;
    }
    for (int i = 0; i < 9; ++i) if (cell[b][i] == '.') return 0;
    return 3;
}

int checkBig() {
    for (int i = 0; i < 8; ++i) {
        int a = sbw[LINES[i][0]];
        int b = sbw[LINES[i][1]];
        int c = sbw[LINES[i][2]];
        if (a != 0 && a != 3 && a == b && b == c) return a;
    }
    for (int i = 0; i < 9; ++i) if (sbw[i] == 0) return 0;
    return 3;
}

void resetGame() {
    for (int b = 0; b < 9; ++b)
        for (int i = 0; i < 9; ++i) cell[b][i] = '.';
    sbw.fill(0);
    forcedBoard = -1;
    bigWinner = 0;
    gameOver = false;
    g_hash = 0;
}

char showCell(int b, int idx, bool playerTurn) {
    if (sbw[b] == 1) return 'X';
    if (sbw[b] == 2) return 'O';
    if (sbw[b] == 3) return '#';
    char c = cell[b][idx];
    if (c == 'X' || c == 'O') return c;
    if (playerTurn && !gameOver &&
        (b == forcedBoard || forcedBoard == -1))
        return char('1' + idx);
    return '.';
}

void drawBoard(bool playerTurn) {
    cout << "\n小棋盘编号：\n";
    cout << " 1 | 2 | 3 \n";
    cout << "---+---+---\n";
    cout << " 4 | 5 | 6 \n";
    cout << "---+---+---\n";
    cout << " 7 | 8 | 9 \n";
    cout << "小格编号相同：1 2 3 / 4 5 6 / 7 8 9\n\n";

    static const char* LINE_SINGLE =
        "-----+-----+-----++-----+-----+-----++-----+-----+-----";
    static const char* LINE_DOUBLE =
        "=================++=================++=================";

    for (int r = 0; r < 9; ++r) {
        int bigRow = r / 3;
        int smallRow = r % 3;

        for (int bigCol = 0; bigCol < 3; ++bigCol) {
            if (bigCol > 0) cout << "||";
            int b = bigRow * 3 + bigCol;
            for (int smallCol = 0; smallCol < 3; ++smallCol) {
                if (smallCol > 0) cout << "|";
                int idx = smallRow * 3 + smallCol;
                cout << "  " << showCell(b, idx, playerTurn) << "  ";
            }
        }
        cout << "\n";

        if (r == 2 || r == 5)
            cout << LINE_DOUBLE << "\n";
        else if (r != 8)
            cout << LINE_SINGLE << "\n";
    }
    cout << "\n";
}

bool isLegal(Move m, int player) {
    (void)player;
    if (gameOver) return false;
    if (m.sb < 0 || m.sb > 8 || m.idx < 0 || m.idx > 8) return false;
    if (sbw[m.sb] != 0) return false;
    if (cell[m.sb][m.idx] != '.') return false;
    if (forcedBoard != -1 && m.sb != forcedBoard) return false;
    return true;
}

vector<Move> legalMoves(int player) {
    (void)player;
    vector<Move> res;
    if (gameOver) return res;

    if (forcedBoard != -1 && sbw[forcedBoard] == 0) {
        for (int i = 0; i < 9; ++i)
            if (cell[forcedBoard][i] == '.')
                res.push_back({forcedBoard, i});
        if (!res.empty()) return res;
    }
    for (int b = 0; b < 9; ++b) {
        if (sbw[b] != 0) continue;
        for (int i = 0; i < 9; ++i)
            if (cell[b][i] == '.')
                res.push_back({b, i});
    }
    return res;
}

bool applyMove(Move m, int player, NextBoardMode mode = NextBoardMode::Random) {
    if (!isLegal(m, player)) return false;

    int oldFb = forcedBoard;

    g_hash ^= zCell[m.sb][m.idx][player - 1];
    cell[m.sb][m.idx] = (player == 1 ? 'X' : 'O');

    sbw[m.sb] = checkSmall(m.sb);
    if (sbw[m.sb] != 0)
        g_hash ^= zSbw[m.sb][sbw[m.sb]];

    bigWinner = checkBig();

    if (bigWinner != 0) {
        gameOver = true;
        forcedBoard = -1;
        if (oldFb != -1) g_hash ^= zForced[oldFb];
        return true;
    }

    int next = m.idx;
    if (sbw[next] != 0) {
        if (mode == NextBoardMode::SearchFree) {
            forcedBoard = -1;
        } else {
            int freeBoards[9];
            int cnt = 0;
            for (int b = 0; b < 9; ++b)
                if (sbw[b] == 0) freeBoards[cnt++] = b;
            if (cnt == 0) forcedBoard = -1;
            else forcedBoard = freeBoards[rngReal() % cnt];
        }
    } else {
        forcedBoard = next;
    }

    if (oldFb != forcedBoard) {
        if (oldFb != -1) g_hash ^= zForced[oldFb];
        if (forcedBoard != -1) g_hash ^= zForced[forcedBoard];
    }

    return true;
}

struct TempPlace {
    int sb, idx;
    char old;
    TempPlace(int sb_, int idx_, char c) : sb(sb_), idx(idx_), old(cell[sb_][idx_]) {
        cell[sb][idx] = c;
    }
    ~TempPlace() { cell[sb][idx] = old; }
};

struct TempSbw {
    int sb;
    int old;
    TempSbw(int sb_, int val) : sb(sb_), old(sbw[sb_]) { sbw[sb_] = val; }
    ~TempSbw() { sbw[sb] = old; }
};

bool wouldWinSmall(Move m, int player) {
    char me = (player == 1 ? 'X' : 'O');
    TempPlace tp(m.sb, m.idx, me);
    return checkSmall(m.sb) == player;
}

bool wouldBlockSmall(Move m, int player) {
    char op = (player == 1 ? 'O' : 'X');
    TempPlace tp(m.sb, m.idx, op);
    return checkSmall(m.sb) == (3 - player);
}

bool wouldWinBig(Move m, int player) {
    char me = (player == 1 ? 'X' : 'O');
    TempPlace tp(m.sb, m.idx, me);
    TempSbw ts(m.sb, checkSmall(m.sb));
    return checkBig() == player;
}

pair<int,int> evalSmallBoth(int b) {
    int sx = 0, so = 0;
    for (int i = 0; i < 8; ++i) {
        int xc = 0, oc = 0;
        for (int k = 0; k < 3; ++k) {
            char c = cell[b][LINES[i][k]];
            if (c == 'X') xc++;
            else if (c == 'O') oc++;
        }
        if (xc > 0 && oc == 0) sx += xc * xc * 10;
        if (oc > 0 && xc == 0) so += oc * oc * 10;
    }
    for (int i = 0; i < 9; ++i) {
        if (cell[b][i] == 'X') sx += CELL_W[i];
        else if (cell[b][i] == 'O') so += CELL_W[i];
    }
    return make_pair(sx, so);
}

pair<int,int> smallDoubleThreatsBoth(int b) {
    int cx = 0, co = 0;
    for (int i = 0; i < 8; ++i) {
        int xc = 0, oc = 0, empty = 0;
        for (int k = 0; k < 3; ++k) {
            char c = cell[b][LINES[i][k]];
            if (c == 'X') xc++;
            else if (c == 'O') oc++;
            else empty++;
        }
        if (xc == 2 && oc == 0 && empty == 1) cx++;
        if (oc == 2 && xc == 0 && empty == 1) co++;
    }
    return make_pair(cx, co);
}

pair<int,int> evalBigBoth() {
    int sx = 0, so = 0;
    for (int i = 0; i < 8; ++i) {
        int xc = 0, oc = 0;
        for (int k = 0; k < 3; ++k) {
            int w = sbw[LINES[i][k]];
            if (w == 1) xc++;
            else if (w == 2) oc++;
        }
        if (xc > 0 && oc == 0) sx += xc * xc * 100;
        if (oc > 0 && xc == 0) so += oc * oc * 100;
    }
    return make_pair(sx, so);
}

pair<int,int> bigDoubleThreatsBoth() {
    int cx = 0, co = 0;
    for (int i = 0; i < 8; ++i) {
        int xc = 0, oc = 0, empty = 0;
        for (int k = 0; k < 3; ++k) {
            int w = sbw[LINES[i][k]];
            if (w == 1) xc++;
            else if (w == 2) oc++;
            else if (w == 0) empty++;
        }
        if (xc == 2 && oc == 0 && empty == 1) cx++;
        if (oc == 2 && xc == 0 && empty == 1) co++;
    }
    return make_pair(cx, co);
}

int smallDoubleThreats(int b, int player) {
    char me = (player == 1 ? 'X' : 'O');
    int cnt = 0;
    for (int i = 0; i < 8; ++i) {
        int mc = 0, empty = 0;
        bool blocked = false;
        for (int k = 0; k < 3; ++k) {
            char c = cell[b][LINES[i][k]];
            if (c == me) mc++;
            else if (c == '.') empty++;
            else { blocked = true; break; }
        }
        if (!blocked && mc == 2 && empty == 1) cnt++;
    }
    return cnt;
}

int bigDoubleThreats(int player) {
    int cnt = 0;
    for (int i = 0; i < 8; ++i) {
        int mc = 0, empty = 0;
        bool blocked = false;
        for (int k = 0; k < 3; ++k) {
            int w = sbw[LINES[i][k]];
            if (w == player) mc++;
            else if (w == 0) empty++;
            else { blocked = true; break; }
        }
        if (!blocked && mc == 2 && empty == 1) cnt++;
    }
    return cnt;
}

int evalSmall(int b, int player) {
    int score = 0;
    char me = (player == 1 ? 'X' : 'O');
    char op = (player == 1 ? 'O' : 'X');
    for (int i = 0; i < 8; ++i) {
        int mc = 0, oc = 0;
        for (int k = 0; k < 3; ++k) {
            char c = cell[b][LINES[i][k]];
            if (c == me) mc++;
            else if (c == op) oc++;
        }
        if (mc > 0 && oc == 0) score += mc * mc * 10;
        if (oc > 0 && mc == 0) score -= oc * oc * 10;
    }
    for (int i = 0; i < 9; ++i) {
        if (cell[b][i] == me) score += CELL_W[i];
        else if (cell[b][i] == op) score -= CELL_W[i];
    }
    return score;
}

int evaluate() {
    if (bigWinner == COMPUTER) return WIN_SCORE;
    if (bigWinner == HUMAN)    return -WIN_SCORE;
    if (bigWinner == 3)        return 0;

    int s = 0;

    pair<int,int> bb = evalBigBoth();
    s += bb.first - bb.second;

    for (int b = 0; b < 9; ++b) {
        if (sbw[b] != 0) continue;
        pair<int,int> es = evalSmallBoth(b);
        s += es.first - es.second;
        pair<int,int> st = smallDoubleThreatsBoth(b);
        s += 300 * (st.first - st.second);
    }

    pair<int,int> dt = bigDoubleThreatsBoth();
    s += 8000 * (dt.first - dt.second);

    return s;
}

int bigBoardPosWeight(int sb) {
    if (sb == 4) return 120;
    if (sb == 0 || sb == 2 || sb == 6 || sb == 8) return 110;
    return 100;
}

int smallCellPosWeight(int idx) {
    if (idx == 4) return 40;
    if (idx == 0 || idx == 2 || idx == 6 || idx == 8) return 30;
    return 20;
}

int moveOrderScore(Move m, int player) {
    if (wouldWinBig(m, player)) return WIN_BIG_BONUS;

    int score = 0;

    if (wouldWinSmall(m, player))
        score += bigBoardPosWeight(m.sb);

    if (wouldBlockSmall(m, player))
        score += 60;

    score += smallCellPosWeight(m.idx);

    int nextBoard = m.idx;
    if (sbw[nextBoard] == 0) {
        int aiAdv = evalSmall(nextBoard, COMPUTER);
        int huAdv = evalSmall(nextBoard, HUMAN);
        int huThreats = smallDoubleThreats(nextBoard, HUMAN);
        int aiThreats = smallDoubleThreats(nextBoard, COMPUTER);

        if (aiAdv > 0 || aiThreats > 0) score -= 15;

        if (huThreats > 0 || huAdv > 20) {
            if (nextBoard == 4) score -= 40;
            else if (nextBoard == 0 || nextBoard == 2 ||
                     nextBoard == 6 || nextBoard == 8) score -= 30;
            else score -= 20;
        }

        if (aiThreats > 0 && huAdv > 0) score -= 20;
    }

    return score;
}

void orderMoves(vector<Move>& moves, int player) {
    sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){
        int sa = moveOrderScore(a, player);
        int sb = moveOrderScore(b, player);
        if (sa != sb) return sa > sb;
        return a.sb * 9 + a.idx < b.sb * 9 + b.idx;
    });
}

bool outOfTime() {
    if (((++g_nodes) & 1023) == 0) {
        if (Clock::now() >= g_deadline) g_timeout = true;
    }
    return g_timeout;
}

bool isForcingMove(Move m, int player) {
    return wouldWinSmall(m, player) || wouldBlockSmall(m, player);
}

int quiescence(int alpha, int beta, int player, int qdepth) {
    if (gameOver || qdepth == 0) return evaluate();
    if (outOfTime()) return TIMEOUT_VAL;

    vector<Move> moves = legalMoves(player);
    vector<Move> tactical;

    bool hasBigThreat = (bigDoubleThreats(player) > 0);

    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (wouldWinSmall(m, player) || wouldBlockSmall(m, player)
            || (hasBigThreat && wouldWinBig(m, player)))
            tactical.push_back(m);
    }

    if (tactical.empty()) return evaluate();

    if (player == COMPUTER) {
        int best = -INF;
        for (size_t i = 0; i < tactical.size(); ++i) {
            Move m = tactical[i];
            State st = saveState();
            applyMove(m, COMPUTER, NextBoardMode::SearchFree);
            int v = quiescence(alpha, beta, HUMAN, qdepth - 1);
            restoreState(st);
            if (g_timeout) return best;
            if (v > best) best = v;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
        return best;
    } else {
        int best = INF;
        for (size_t i = 0; i < tactical.size(); ++i) {
            Move m = tactical[i];
            State st = saveState();
            applyMove(m, HUMAN, NextBoardMode::SearchFree);
            int v = quiescence(alpha, beta, COMPUTER, qdepth - 1);
            restoreState(st);
            if (g_timeout) return best;
            if (v < best) best = v;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
        return best;
    }
}

int alphaBeta(int depth, int alpha, int beta, int player) {
    if (gameOver) return evaluate();
    if (outOfTime()) return TIMEOUT_VAL;
    if (depth == 0) return quiescence(alpha, beta, player, g_diff.qDepth);

    unsigned long long key = g_hash;
    TTEntry& entry = tt[key & TT_MASK];
    if (g_diff.useTT && entry.key == key && entry.flag != TT_EMPTY && entry.depth >= depth) {
        if (entry.flag == TT_EXACT) return entry.value;
        if (entry.flag == TT_LOWER && entry.value >= beta) return entry.value;
        if (entry.flag == TT_UPPER && entry.value <= alpha) return entry.value;
    }

    int originalAlpha = alpha;
    int originalBeta = beta;

    vector<Move> moves = legalMoves(player);
    if (moves.empty()) return evaluate();

    orderMoves(moves, player);

    int best;
    if (player == COMPUTER) {
        best = -INF;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            State st = saveState();
            applyMove(m, COMPUTER, NextBoardMode::SearchFree);
            int v = alphaBeta(depth - 1, alpha, beta, HUMAN);
            restoreState(st);
            if (g_timeout) return best;
            if (v > best) best = v;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
    } else {
        best = INF;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            State st = saveState();
            applyMove(m, HUMAN, NextBoardMode::SearchFree);
            int v = alphaBeta(depth - 1, alpha, beta, COMPUTER);
            restoreState(st);
            if (g_timeout) return best;
            if (v < best) best = v;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
    }

    if (!g_timeout && g_diff.useTT) {
        entry.key = key;
        entry.value = best;
        entry.depth = depth;
        if (best <= originalAlpha) entry.flag = TT_UPPER;
        else if (best >= originalBeta) entry.flag = TT_LOWER;
        else entry.flag = TT_EXACT;
    }

    return best;
}

// 根据难度随机漏着
Move randomBlunderMove(vector<Move>& moves) {
    int idx = rngReal() % moves.size();
    return moves[idx];
}

Move findBestMove() {
    auto start = Clock::now();
    g_deadline = start + chrono::duration_cast<Clock::duration>(
                              chrono::duration<double>(g_diff.timeLimit));
    g_timeout = false;
    g_nodes = 0;

    if (g_diff.useTT)
        for (int i = 0; i < TT_SIZE; ++i) tt[i] = TTEntry();

    vector<Move> moves = legalMoves(COMPUTER);
    if (moves.empty()) {
        Move none = {-1, -1};
        return none;
    }

    orderMoves(moves, COMPUTER);

    // 低难度随机漏着
    if (g_diff.randomBlunderPercent > 0) {
        int r = rngReal() % 100;
        if (r < g_diff.randomBlunderPercent) {
            return randomBlunderMove(moves);
        }
    }

    if ((int)moves.size() > g_diff.rootMoves) moves.resize(g_diff.rootMoves);

    Move best = moves[0];
    int bestScore = -INF;

    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        State st = saveState();
        applyMove(m, COMPUTER, NextBoardMode::SearchFree);
        int v = evaluate();
        restoreState(st);
        if (v > bestScore) { bestScore = v; best = m; }
    }

    bestScore = -INF;

    vector<int> scores(moves.size(), -INF);

    for (int d = 1; d <= MAX_DEPTH; ++d) {
        g_timeout = false;
        int localBestScore = -INF;
        Move localBest = {-1, -1};
        bool complete = true;
        for (size_t i = 0; i < scores.size(); ++i) scores[i] = -INF;

        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            State st = saveState();
            applyMove(m, COMPUTER, NextBoardMode::SearchFree);
            int v;
            if (g_diff.useQuiescence)
                v = alphaBeta(d - 1, -INF, INF, HUMAN);
            else
                v = (d - 1 == 0) ? evaluate() : alphaBeta(d - 1, -INF, INF, HUMAN);
            restoreState(st);

            if (g_timeout) { complete = false; break; }

            scores[i] = v;
            if (v > localBestScore) { localBestScore = v; localBest = m; }
        }

        if (complete && localBest.sb != -1) {
            best = localBest;
            bestScore = localBestScore;
        }

        if (g_timeout) break;

        if (bestScore >= WIN_SCORE - 1 || bestScore <= -WIN_SCORE + 1) break;

        vector<pair<int, Move> > scored;
        for (size_t i = 0; i < moves.size(); ++i)
            scored.push_back(make_pair(scores[i], moves[i]));
        stable_sort(scored.begin(), scored.end(),
                    [](const pair<int, Move>& a, const pair<int, Move>& b){
                        return a.first > b.first;
                    });
        for (size_t i = 0; i < moves.size(); ++i) moves[i] = scored[i].second;
    }

    return best;
}

void showRules() {
    cout << R"RULES(

========================================
        终极井字棋 规则介绍
========================================

一、它和普通井字棋的关系
------------------------
普通井字棋你肯定玩过：3x3 九格，谁先三连谁赢。
终极井字棋是"井字棋套井字棋"：9 个井字棋小棋盘，拼成一个大棋盘。
你既要在一个个小棋盘里三连，也要在大棋盘上三连。
核心思想就一句话：

    你下在哪一格，决定对手下一步必须去哪个小棋盘。

二、棋盘长什么样
----------------
大棋盘是 3x3，每个大格子里又是一个完整的 3x3 小棋盘。
总共 9 个小棋盘 x 9 格 = 81 格。

给小棋盘编号：

 1 | 2 | 3
---+---+---
 4 | 5 | 6
---+---+---
 7 | 8 | 9

每个小棋盘内部的 9 个小格，编号规则一模一样。
所以一个落子位置可以说成"小棋盘 #几 的小格 #几"。

三、基本落子规则
----------------
1. X 先手，O 后手，轮流落子。
2. 你只能在一个还没结束的小棋盘里落子。
3. 你下在小棋盘的哪一格，对手下一步就必须去编号相同的小棋盘。
   例：你在小棋盘 #5 的小格 #3 落子，对手下一步必须去小棋盘 #3。
4. 如果对手被指定去的小棋盘已经结束了（被赢掉或下满），
   对手就获得自由，可以任意选择一个还没结束的小棋盘。
   本程序为了增加变化，此时会"随机"改送一个空闲小棋盘。

四、小棋盘怎么"结束"
--------------------
  X 赢   -> 整块显示 X
  O 赢   -> 整块显示 O
  平局   -> 整块显示 #
小棋盘一旦结束，就锁死，谁都不能再往里下。

五、大棋盘怎么赢
----------------
你赢了某个小棋盘，就等于在大棋盘上占领了那个格子。
大棋盘上三个你占领的小棋盘连成一线（横、竖、斜），你就赢了整局。

六、为什么这游戏比普通井字棋难
------------------------------
你每下一手，都在决定对手下一步能去哪。
核心不是"我能不能三连"，而是：
  我这一手，会把对手送到哪里？那个地方对它有利还是对我不利？

七、这个程序里的特殊规则
------------------------
1. 机器先手，执 X。
2. 机器第一步固定下在"小棋盘 #5 的小格 #5"，也就是正中心。
3. 你被送到的小棋盘如果已经结束，程序会随机改送一个空闲小棋盘。
4. 轮到你了，当前可下小棋盘的 9 个空格会显示 1~9，直接输入数字即可。
   自由选择时，所有未结束小棋盘都会显示 1~9，请按提示先选小棋盘。
5. 机器有 Easy / Medium / Hard / Impossible 四个难度。

八、新手最容易犯的三个错
------------------------
1. 只顾小棋盘三连，不管大棋盘。
2. 随手把对手送进它优势的小棋盘。
3. 以为赢了小棋盘就是好事。赢小棋盘会让对手获得自由选择权。

九、一句话总结
--------------
在小棋盘里三连，占大棋盘的格子；
你下哪一格，决定对手下一步去哪；
大棋盘三连，赢整局。

========================================

)RULES";

    cout << "按回车开始游戏...";
    string dummy;
    getline(cin, dummy);
}

int chooseDifficulty() {
    cout << "\n请选择 AI 难度：\n";
    cout << "  1. Easy       新手能轻松打过\n";
    cout << "  2. Medium     比新手强\n";
    cout << "  3. Hard       较难\n";
    cout << "  4. Impossible 原本的最强 AI\n";
    cout << "请输入 1-4: ";

    int choice;
    while (true) {
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "请输入数字 1-4: ";
            continue;
        }
        if (choice >= 1 && choice <= 4) break;
        cout << "请输入 1-4: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    g_diffIndex = choice - 1;
    g_diff = DIFFS[g_diffIndex];

    cout << "已选择难度：" << g_diff.name << "\n";
    return g_diffIndex;
}

int main() {
    initZobrist();

    showRules();
    chooseDifficulty();

    cout << "\n=== 终极井字棋：机器先手，难度 "
         << g_diff.name << " ===\n";
    cout << "机器执 X（先手），你执 O（后手）。\n";
    cout << "机器首手固定：中心小棋盘(#5)的中间格(#5)。\n";

    while (true) {
        resetGame();

        {
            Move m = {4, 4};
            if (!applyMove(m, COMPUTER, NextBoardMode::Random)) {
                cerr << "内部错误：首手非法" << endl;
                return 1;
            }
            cout << "\n机器（X）首手：小棋盘 #5 的小格 #5。\n";
        }

        int turn = HUMAN;

        while (!gameOver) {
            drawBoard(turn == HUMAN);

            if (forcedBoard == -1)
                cout << "下一步：可任意选择未结束的小棋盘。\n";
            else
                cout << "下一步：必须下在小棋盘 #" << forcedBoard + 1 << "。\n";

            if (turn == HUMAN) {
                Move m;
                bool ok = false;
                while (!ok) {
                    if (forcedBoard == -1) {
                        int b, c;
                        cout << "输入 小棋盘(1-9) 小格(1-9): ";
                        if (!(cin >> b >> c)) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "请输入数字！\n";
                            continue;
                        }
                        m.sb = b - 1;
                        m.idx = c - 1;
                    } else {
                        int c;
                        cout << "输入 小格(1-9): ";
                        if (!(cin >> c)) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "请输入数字！\n";
                            continue;
                        }
                        m.sb = forcedBoard;
                        m.idx = c - 1;
                    }

                    if (isLegal(m, HUMAN)) {
                        applyMove(m, HUMAN, NextBoardMode::Random);
                        ok = true;
                    } else {
                        cout << "非法位置，请重新输入。\n";
                    }
                }
            } else {
                cout << "机器思考中（难度 " << g_diff.name << "）...\n";
                Move m = findBestMove();
                if (m.sb == -1) break;

                int oldIdx = m.idx;
                applyMove(m, COMPUTER, NextBoardMode::Random);

                cout << "机器（X）下在小棋盘 #"
                     << m.sb + 1 << " 的小格 #" << m.idx + 1 << "。\n";

                if (forcedBoard != -1 && forcedBoard != oldIdx)
                    cout << "（原定小棋盘 #" << oldIdx + 1
                         << " 已结束，随机改送到 #" << forcedBoard + 1 << "）\n";
            }

            if (gameOver) break;
            turn = 3 - turn;
        }

        drawBoard(turn == HUMAN);

        if (bigWinner == COMPUTER) cout << "机器（X）赢了。\n";
        else if (bigWinner == HUMAN) cout << "你赢了。\n";
        else cout << "平局。\n";

        char again;
        cout << "再来一局？(y/n): ";
        cin >> again;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (again != 'y' && again != 'Y') {
            cout << "还要换难度吗？(y/n): ";
            char change;
            cin >> change;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (change == 'y' || change == 'Y') {
                chooseDifficulty();
                continue;
            }
            cout << "游戏结束。\n";
            break;
        }
    }
    return 0;
}
