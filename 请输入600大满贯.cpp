#include<bits/stdc++.h> 
using namespace std;
const int MAXN=1000;
int mat[MAXN][MAXN];
int t[MAXN][MAXN];
void oddMagic(int n)
{
    int row = 0, col = n / 2;
    for(int num = 1; num <= n * n; num++)
    {
        mat[row][col] = num;
        int pr = row, pc = col;
        row--; col++;
        if(row < 0) row = n - 1;
        if(col >= n) col = 0;
        if(mat[row][col] != 0)
        {
            row = pr + 1;
            col = pc;
        }
    }
}
void doubleEvenMagic(int n)
{
    int cnt = 1;
    for(int i = 0; i < n; i++)
        for(int j = 0; j < n; j++)
            mat[i][j] = cnt++;

    for(int i = 0; i < n; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if((i%4==0||i%4==3) && (j%4==0||j%4==3))
                mat[i][j] = n*n + 1 - mat[i][j];
            else if((i%4==1||i%4==2) && (j%4==1||j%4==2))
                mat[i][j] = n*n + 1 - mat[i][j];
        }
    }
}
void singleEvenMagic(int n)
{
    int k = (n - 2)/4;
    int m = n/2;
    int r=0,c=m/2;
    for(int num=1;num<=m*m;num++)
    {
        t[r][c]=num;
        int pr=r,pc=c;
        r--;c++;
        if(r<0) r=m-1;
        if(c>=m) c=0;
        if(t[r][c]!=0)
        {
            r=pr+1;
            c=pc;
        }
    }
    for(int i=0;i<m;i++)
    {
        for(int j=0;j<m;j++)
        {
            int val = t[i][j];
            mat[i][j] = val;
            mat[i+m][j+m] = val + m*m;
            if(j >= m - k)
            {
                mat[i][j+m] = val + 2*m*m;
                mat[i+m][j] = val + 3*m*m;
            }
            else if(i == k)
            {
                mat[i][j+m] = val + 3*m*m;
                mat[i+m][j] = val + 2*m*m;
            }
            else
            {
                mat[i][j+m] = val + 2*m*m;
                mat[i+m][j] = val + 3*m*m;
            }
        }
    }
}


int main(int argc,char**argv){
	long long a,b;
	cin>>a;
	b=a*a;
	const int c=a;
    int m[c][c];
    double d,e;
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			m[x][y]=x+y+1;
	    }
	}
	for(int x=0;x<c;x++){
		for(int y=0;y<c;y++){
			cout<<m[x][y]<<" ";
	    }
	    cout<<endl;
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
	    cout<<endl;
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
	    cout<<endl;
	}
	for(int x=1;x<=a;x++){
		for(int y=1;y<=a;y++){	
			cout<<x<<"*"<<y<<"="<<x*y<<" ";
	    }
	    cout<<endl;
	}
	for(int x=1;x<=b;x++){
		cout<<sqrt(x)<<" ";
		if(x%a==0){
			cout<<endl;
		}
	}
	for(int x=1;x<=b;x++){
		cout<<x*x<<" ";
		if(x%a==0) {
			cout<<endl;
		}
	}
	for(int x=1;x<=b;x++){
		cout<<x*(x-(x*x))<<" ";
		if(x%a==0) {
			cout<<endl;
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
	    cout<<endl;
	}
	for(int x=1;x<=b;x++){
		d=x/a;
		e=x%a;
		cout<<max(sqrt(x),max(d,e))-min(sqrt(x),min(d,e))<<" ";
		if(e==0){
			cout<<endl;
		}
	}
	if(a<1 || a>999) return 0;
    for(int i=0;i<a;i++)
    {
        for(int j=0;j<a;j++)
        {
            mat[i][j]=0;
            t[i][j]=0;
        }
    }

    if(a%2 == 1)
        oddMagic(a);
    else if(a%4 == 0)
        doubleEvenMagic(a);
    else
        singleEvenMagic(a);
    for(int i=0;i<a;i++)
    {
        for(int j=0;j<a;j++)
        {
            if(j>0) cout<<" ";
            cout<<mat[i][j];
        }
        cout<<endl;
    }
	return 0;
}
