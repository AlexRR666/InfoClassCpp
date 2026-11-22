// kata_driver.cpp — C++14, GTP driver for KataGo
// 编译: g++ -std=c++14 -O2 kata_driver.cpp -o kata_driver.exe

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#endif

// ============ 修改这三处路径 ============
static const char* KATAGO_BIN   = "C:\\katago\\katago.exe";
static const char* WEIGHT_PATH  = "C:\\katago\\model.bin.gz";
static const char* CONFIG_PATH  = "C:\\katago\\gtp.cfg";
// =======================================

class GtpEngine {
public:
    explicit GtpEngine(const std::string& cmd) {
        pipe_ = _popen(cmd.c_str(), "r+");
        if (!pipe_) throw std::runtime_error("Failed to start KataGo. Check paths.");
        setvbuf(pipe_, nullptr, _IONBF, 0);
    }
    ~GtpEngine() { if (pipe_) _pclose(pipe_); }

    std::string send(const std::string& cmd) {
        fprintf(pipe_, "%s\n", cmd.c_str());
        fflush(pipe_);
        std::string response, line;
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe_)) {
            line = buf;
            if (line == "\n" || line == "\r\n") break;
            response += line;
        }
        return response;
    }

    // 从 "= D4\n\n" 中提取 "D4"
    static std::string parseMove(const std::string& gtpResp) {
        auto eq = gtpResp.find('=');
        if (eq == std::string::npos) return "";
        std::string move = gtpResp.substr(eq + 1);
        size_t start = move.find_first_not_of(" \t\r\n");
        size_t end   = move.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return move.substr(start, end - start + 1);
    }

private:
    FILE* pipe_;
};

// ============ 棋盘 ============
struct Board {
    // board[r][c]: '.' 空, 'B' 黑, 'W' 白
    std::vector<std::string> cells;
    Board() : cells(19, std::string(19, '.')) {}

    void clear() {
        for (auto& row : cells) std::fill(row.begin(), row.end(), '.');
    }

    // GTP 坐标 -> (row, col)，失败返回 false
    static bool gtpToRC(const std::string& m, int& r, int& c) {
        if (m.size() < 2) return false;
        char colChar = std::toupper(m[0]);
        if (colChar == 'I') return false; // GTP 不使用 I
        const char cols[] = "ABCDEFGHJKLMNOPQRST";
        const char* p = std::strchr(cols, colChar);
        if (!p) return false;
        c = static_cast<int>(p - cols);
        int num = std::stoi(m.substr(1));
        if (num < 1 || num > 19) return false;
        r = 19 - num; // GTP 第1行在底部
        return true;
    }

    void place(const std::string& move, char color) {
        int r, c;
        if (gtpToRC(move, r, c)) cells[r][c] = color;
    }

    void print() const {
        const char cols[] = "ABCDEFGHJKLMNOPQRST";
        std::cout << "   ";
        for (int c = 0; c < 19; ++c) std::cout << cols[c] << ' ';
        std::cout << "\n";

        for (int r = 0; r < 19; ++r) {
            int label = 19 - r;
            if (label < 10) std::cout << ' ';
            std::cout << label << ' ';

            for (int c = 0; c < 19; ++c) {
                char p = cells[r][c];
                if (p == 'B')      std::cout << "○ ";
                else if (p == 'W') std::cout << "● ";
                else               std::cout << "· ";
            }
            std::cout << "\n";
        }
    }
};

int main() {
#if defined(_WIN32)
    // 让控制台支持 UTF-8，显示 ○ ● ·
    system("chcp 65001 > nul");
#endif

    std::string cmd = std::string(KATAGO_BIN) + " gtp -model \"" +
                      WEIGHT_PATH + "\" -config \"" + CONFIG_PATH + "\"";

    try {
        GtpEngine engine(cmd);
        std::cout << "Engine: " << GtpEngine::parseMove(engine.send("name")) << "\n";

        engine.send("boardsize 19");
        engine.send("komi 7.5");
        engine.send("clear_board");

        Board board;

        // AI 执黑，人类执白。想换边就交换下面两行。
        std::string aiColor    = "B";
        std::string humanColor = "W";
        std::string turn       = "B";

        std::cout << "\n=== 对局开始 ===\n";
        std::cout << "AI: " << aiColor << " (○)  人类: " << humanColor << " (●)\n";
        std::cout << "输入坐标如 Q16，输入 pass 停一手，输入 quit 退出。\n\n";

        board.print();

        for (int moveNum = 1; moveNum <= 500; ++moveNum) {
            if (turn == aiColor) {
                std::string resp = engine.send("genmove " + turn);
                std::string m = GtpEngine::parseMove(resp);

                std::cout << "\n" << moveNum << ". AI(" << turn << "): " << m << "\n";

                if (m == "pass" || m == "resign" || m.empty()) {
                    std::cout << "AI " << (m.empty() ? "error" : m) << "，对局结束。\n";
                    std::cout << "final_score: " << engine.send("final_score") << "\n";
                    break;
                }

                board.place(m, turn[0]);
                board.print();
            } else {
                std::cout << "\n" << moveNum << ". 你的回合(" << turn << ")，输入坐标: ";
                std::string input;
                if (!(std::cin >> input)) break;

                // 转大写，方便输入 q16
                std::transform(input.begin(), input.end(), input.begin(),
                               [](unsigned char ch) { return std::toupper(ch); });

                if (input == "QUIT") {
                    std::cout << "退出。\n";
                    break;
                }

                if (input == "PASS") {
                    engine.send("play " + turn + " pass");
                    std::cout << "你 pass。\n";
                } else {
                    engine.send("play " + turn + " " + input);
                    std::cout << "你下: " << input << "\n";
                    board.place(input, turn[0]);
                    board.print();
                }
            }

            turn = (turn == "B") ? "W" : "B";
        }

        engine.send("quit");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
