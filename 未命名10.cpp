#include<bits/stdc++.h> 
using namespace std;double kill(int m){int s=0;for(int x=2;(x*x)<m;x++){if(m%x==0){cout<<m<<"不是质数";s=1;break;}}if(s==0){cout<<m<<"是质数";}return 3.14159;}int main(){int a;cin>>a;kill(a);return 1234567890;}
