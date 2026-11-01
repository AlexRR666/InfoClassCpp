#include<bits/stdc++.h>
using namespace std;
int main(){
	int n,k;cin>>n>>k;
	long long ans=0;
	if(k==2){
		for(int a=1;a<=n/2;a++){int b=n-a;if(b>=a)ans++;}
	}
	else if(k==3){
		for(int a=1;a<=n/3;a++)
			for(int b=a;b<=(n-a)/2;b++){int c=n-a-b;if(c>=b)ans++;}
	}
	else if(k==4){
		for(int a=1;a<=n/4;a++)
			for(int b=a;b<=(n-a)/3;b++)
				for(int c=b;c<=(n-a-b)/2;c++){int d=n-a-b-c;if(d>=c)ans++;}
	}
	else if(k==5){
		for(int a=1;a<=n/5;a++)
			for(int b=a;b<=(n-a)/4;b++)
				for(int c=b;c<=(n-a-b)/3;c++)
					for(int d=c;d<=(n-a-b-c)/2;d++){int e=n-a-b-c-d;if(e>=d)ans++;}
	}
	else if(k==6){
		for(int a=1;a<=n/6;a++)
			for(int b=a;b<=(n-a)/5;b++)
				for(int c=b;c<=(n-a-b)/4;c++)
					for(int d=c;d<=(n-a-b-c)/3;d++)
						for(int e=d;e<=(n-a-b-c-d)/2;e++){int f=n-a-b-c-d-e;if(f>=e)ans++;}
	}
	cout<<ans;
	return 0;
}
