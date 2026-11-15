#include<bits/stdc++.h>
using namespace std;
const int MAXN=5000005;
int a[MAXN],fail_[MAXN];
int main(){
    int m=1;a[0]=1;fail_[0]=0;int cnt4=0;
    while(1){
        int bestK=1;
        int p=m-fail_[m-1];
        if(m%p==0)bestK=m/p;
        a[m]=bestK;
        int j=fail_[m-1];
        while(j>0 && a[j]!=a[m]) j=fail_[j-1];
        if(a[j]==a[m]) j++;
        fail_[m]=j;
        m++;
        if(bestK==4)cnt4++;else cnt4=0;
        if(bestK==5){cout<<m<<"\n";return 0;}
        if(m>5000000){cout<<"NOT_FOUND_UPTO "<<m<<" last="<<bestK<<" cnt4="<<cnt4<<"\n";return 0;}
    }
}
