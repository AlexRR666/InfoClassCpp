#include<bits/stdc++.h>
using namespace std;

int main(){
    ios::sync_with_stdio(false);cout.tie(nullptr);cin.tie(nullptr);
    int n;cin>>n;
    vector<int>a;a.reserve(1000000);
    a.push_back(1);
    cout<<"pos=1 val=1\n";
    if(n==1){cout<<"FOUND pos=1\n";return 0;}

    // 缓存：key = 块内容的哈希，value = 该块作为末尾重复单元时的最大重复次数
    // 但块内容会变，简单起见用 unordered_map<string,int> 存块序列
    // 实际增量更新复杂，这里用"对每个 L 只查末尾"的缓存：key=(L, 末尾L项的哈希)
    unordered_map<long long,int> cache; // key 由 L 和滚动哈希组成

    while(1){
        int m=a.size();
        int bestK=1;
        for(int L=1;L<=m;L++){
            // 先查缓存：如果这个 L 的末尾块之前算过，直接用
            // 但序列在增长，末尾块内容会变，缓存命中率低，这里仅作示意
            int k=0,pos=m;
            while(pos-L>=0){
                bool ok=1;
                for(int i=0;i<L;i++)if(a[pos-L+i]!=a[m-L+i]){ok=0;break;}
                if(!ok)break;
                k++;pos-=L;
            }
            if(k>bestK)bestK=k;
        }
        a.push_back(bestK);
        cout<<"pos="<<a.size()<<" val="<<bestK<<" len="<<a.size()<<"\n";
        if(bestK==n){cout<<"FOUND pos="<<a.size()<<"\n";return 0;}
        if(a.size()>5000000){cout<<"LIMIT len="<<a.size()<<" last="<<bestK<<"\n";return 0;}
    }
}
