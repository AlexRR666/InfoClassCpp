#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	int a,b;
	cin>>a>>b;
	const int c=a;
	const int d=b;
	double e=0;
    int m[c][d];
	for(int x=0;x<c;x++){
		for(int y=0;y<d;y++){
			cin>>m[x][y];
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<d;y++){
			e+=m[x][y];
	    }
	}
	cout<<e/c/d;
	return 0;
}
