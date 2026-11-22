#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T;
    cin>>T;
    while(T--){
        int n;
        cin>>n;
        stack<unsigned long long> s;
        string op;
        unsigned long long x;
        for (int i=0;i<n;i++){
            cin>>op;
            if(op=="push"){
                cin>>x;
                s.push(x);
            }else if(op=="pop"){
                if(s.empty()){
                    cout<<"Empty\n";
                }else{
                    s.pop();
                }
            }else if(op=="query"){
                if(s.empty()){
                    cout<<"Anguei!\n";
                }else{
                    cout<<s.top()<<'\n';
                }
            }else if(op=="size"){
                cout<<s.size()<<'\n';
            }
        }
    }
    return 0;
}
