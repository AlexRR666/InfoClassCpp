#include<bits/stdc++.h> 
using namespace std;
freopen("out.txt","w",stdout);
int main(){
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    long long a,b;
    cin>>a;
    b=a*a;
    int c = (int)a;
    
    vector<vector<long long> > m(c, vector<long long>(c));

	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=x+y+1;
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<'\n';
	}
    cout<<"\n";

	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=(long long)(x+1)*(y+1);
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<'\n';
	}
    cout<<"\n";

	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){	
			m[x][y]=10LL*min(x,y)-max(x,y); 
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<'\n';
	}
    cout<<"\n";

	for(int x=1;x<=a;x++){
		for(int y=1;y<=a;y++){	
			cout<<x<<"*"<<y<<"="<<x*y<<" ";
	    }
	    cout<<'\n';
	}
    cout<<"\n";

	for(long long x=1;x<=b;x++){
		cout<<sqrt((double)x)<<" ";
		if(x%a==0){
			cout<<'\n';
		}
	}
    cout<<"\n";

	for(long long x=1;x<=b;x++){
		cout<<x*x<<" ";
		if(x%a==0) {
			cout<<'\n';
		}
	}
    cout<<"\n";

	for(long long x=1;x<=b;x++){
		cout<<x*(x-(x*x))<<" ";
		if(x%a==0) {
			cout<<'\n';
		}
	}
    cout<<"\n";

	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=(long long)(x+1)*(y+1)*(max(x,y)-min(x,y));
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<'\n';
	}
    cout<<"\n";

	for(long long x=1;x<=b;x++){
		double d = (double)(x-1)/a;
		double e = (x-1)%a;
		double sx = sqrt((double)x);
		double mx = max(max(sx, d), e);
		double mn = min(min(sx, d), e);
		cout<<mx - mn<<" ";
		if(e==a-1){
			cout<<'\n';
		}
	}

	return 0;
}

