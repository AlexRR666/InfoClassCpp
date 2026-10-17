#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	int a;
	cin>>a;
	const int c=a;
    int m[c][c];
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=(x+1)*(y+1);
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<endl;
	}
	return 0;
}
