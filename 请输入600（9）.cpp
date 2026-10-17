#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	long long a,b;
	double d,e;
	cin>>a;
	b=a*a;
	for(int x=1;x<=b;x++){
		d=x/a;
		e=x%a;
		cout<<max(sqrt(x),max(d,e))-min(sqrt(x),min(d,e))<<" ";
		if(e==0){
			cout<<endl;
		}
	}
	return 0;
}
