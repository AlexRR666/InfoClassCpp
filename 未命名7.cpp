#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	int a,b;
	cin>>a>>b;
	const int c=a;
	const int d=b;
    int m[c][d];
	for(int x=0;x<c;x++){
		for(int y=0;y<d;y++){
			m[x][y]=x+y+1;
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<d;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<""<<endl;
	}
	return 0;
}
