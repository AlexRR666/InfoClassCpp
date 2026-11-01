#include <bits/stdc++.h>
using namespace std;
void solve2(int x){
    bool first=true;
    for (int i=14;i>=0;i--){
        if (x&(int)pow(2,i)){
            if(!first)cout << "+";
            first=false;
            if(i==0){
                cout<<"2(0)";
            }else if(i==1){
                cout<<"2";
            }else{
                cout<<"2(";
                solve2(i);
                cout<<")";
            }
        }
    }
}
int main(){
    int n;
    cin>>n;
    solve2(n);
    return 0;
}

