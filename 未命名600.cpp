#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	long long a,b;
	cin>>a;
	b=a*a;
	const int c=a;
    int m[c][c];
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=x+y+1;
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    //cout<<endl;
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=(x+1)*(y+1);
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    //cout<<endl;
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){	
			m[x][y]=10*min(x,y)-max(x,y); 
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    //cout<<endl;
	}
	for(int x=1;x<=a;x++){
		for(int y=1;y<=a;y++){	
			cout<<x<<"*"<<y<<"="<<x*y<<" ";
	    }
	    //cout<<endl;
	}
	for(int x=1;x<=b;x++){
		cout<<sqrt(x)<<" ";
		if(x%a==0){
			//cout<<endl;
		}
	}
	for(int x=1;x<=b;x++){
		cout<<x*x<<" ";
		if(x%a==0) {
			//cout<<endl;
		}
	}
	for(int x=1;x<=b;x++){
		cout<<x*(x-(x*x))<<" ";
		if(x%a==0) {
			//cout<<endl;
		}
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=(x+1)*(y+1)*(max(x,y)-min(x,y));
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    //cout<<endl;
	}
	return 0;
}
