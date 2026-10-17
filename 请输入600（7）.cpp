#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	long long a,b;
	cin>>a;
	b=a*a;
	for(int x=1;x<=b;x++){
		cout<<x*(x-(x*x))<<" ";
		if(x%a==0) {
			cout<<endl;
		}
	}
	return 0;
}
