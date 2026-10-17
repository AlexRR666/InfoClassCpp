#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	int a;
	cin>>a;
	for(int x=1;x<=a;x++){
		for(int y=1;y<=a;y++){	
			cout<<x<<"*"<<y<<"="<<x*y<<" ";
	    }
	    cout<<endl;
	}
	return 0;
}
