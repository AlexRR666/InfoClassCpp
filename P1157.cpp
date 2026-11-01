#include<bits/stdc++.h>
using namespace std;
int ans[22], cnt;
void f(int a,int b,int c) {
	if(c==0){                                   
		for(int i=0;i<cnt;i++)cout<<setw(3)<<ans[i];
		cout<<endl;
		return;
	}
	for(int x=a+1;x<=b;x++){
		ans[cnt++]=x;                       
		f(x,b,c-1);     
		cnt--;
	}
}
int main(){
	int n,r;
	cin>>n>>r;
	f(0,n,r);
	return 0;
}
