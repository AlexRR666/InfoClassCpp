#include <bits/stdc++.h>
using namespace std;
 
long long c[25][25];
 
int main() {
    int n;
    cin >> n;                           // 输出前 n 行，n ≤ 20
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            if (j == 0 || j == i) c[i][j] = 1;      // 两端
            else c[i][j] = c[i-1][j-1] + c[i-1][j];
            if (j) cout << " ";
            cout << c[i][j];
        }
        cout << "\n";
    }
    return 0;
}

