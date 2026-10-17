#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

const int W = 20;
const int H = 14;

struct P { int x, y, w; };

int px, py, jump, camY, score, topY;
vector<P> ps;

void reset() {
    px = W / 2; py = H - 2; jump = 0;
    camY = 0; score = 0; topY = H - 1;
    ps.clear();
    P p0; p0.x = W/2 - 3; p0.y = H - 1; p0.w = 7;
    ps.push_back(p0);
}

int main() {
    srand((unsigned)time(0));
    reset();
    while (true) {
        while (topY > camY - 8) {
            topY -= 3 + rand() % 3;
            int w = 3 + rand() % 5;
            int x = rand() % (W - w);
            P np; np.x = x; np.y = topY; np.w = w;
            ps.push_back(np);
        }
        vector<P> tmp;
        for (int i = 0; i < (int)ps.size(); i++)
            if (ps[i].y < camY + H + 5) tmp.push_back(ps[i]);
        ps = tmp;
        bool ground = false;
        for (int i = 0; i < (int)ps.size(); i++)
            if (py + 1 == ps[i].y && px >= ps[i].x && px < ps[i].x + ps[i].w)
                { ground = true; break; }
        cout << "\n=====================\n";
        cout << "Score: " << score << "  l/r/j/./q\n";
        cout << "=====================\n";
        for (int y = camY; y < camY + H; y++) {
            cout << "|";
            for (int x = 0; x < W; x++) {
                if (px == x && py == y) { cout << "@"; continue; }
                bool d = false;
                for (int i = 0; i < (int)ps.size(); i++)
                    if (ps[i].y == y && x >= ps[i].x && x < ps[i].x + ps[i].w)
                        { cout << "="; d = true; break; }
                if (!d) cout << " ";
            }
            cout << "|\n";
        }
        cout << "=====================\n> ";
        string cmd;
        cin >> cmd;
        if (cmd == "q") break;
        if (cmd == "l" || cmd == "L") px = (px - 1 + W) % W;
        else if (cmd == "r" || cmd == "R") px = (px + 1) % W;
        else if (cmd == "j" || cmd == "J") { if (ground || jump > 0) jump = 3; }
        if (jump > 0) { py--; jump--; }
        else if (!ground) py++;
        if (py < camY + 4) camY = py - 4;
        int s = (H - 2) - py;
        if (s > score) score = s;
        if (py > camY + H) {
            cout << "\nGAME OVER! Score: " << score << "\n";
            cout << "Press Enter to restart (or q to quit): ";
            string t;
            cin.ignore();
            getline(cin, t);
            if (t == "q") break;
            reset();
        }
    }
    return 0;
}
