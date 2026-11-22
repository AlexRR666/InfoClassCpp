#include <bits/stdc++.h>
using namespace std;
 
int main() {
    string s;
    cin >> s;                          // 表达式以 @ 结束，无空格
    int cnt = 0;                       // 未匹配的左括号个数
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] == '(') cnt++;        // 左括号：进栈
        if (s[i] == ')') {             // 右括号：找一个左括号配对
            if (cnt == 0) {            // 没得配：直接失败
                cout << "NO" << "\n";
                return 0;
            }
            cnt--;
        }
    }
    if (cnt == 0) cout << "YES" << "\n";
    else cout << "NO" << "\n";
    return 0;
}

