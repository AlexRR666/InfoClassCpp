#include<bits/stdc++.h>
using namespace std;
int n,m,t;
int sx,sy,fx,fy;
int g[10][10];
bool vis[10][10];
int ans=0;
int dx[4]={-1,1,0,0};
int dy[4]={0,0,-1,1};
void dfs(int x,int y){
	if(x==fx&&y==fy){ans++;return;}
	for(int d=0;d<4;d++){
		int nx=x+dx[d];
		int ny=y+dy[d];
		if(nx<1||nx>n||ny<1||ny>m)continue;
		if(g[nx][ny]==1)continue;
		if(vis[nx][ny])continue;
		vis[nx][ny]=true;
		dfs(nx,ny);
		vis[nx][ny]=false;
	}
}
int main(){
	cin>>n>>m>>t;
	cin>>sx>>sy>>fx>>fy;
	for(int i=0;i<t;i++){
		int x,y;
		cin>>x>>y;
		g[x][y]=1;
	}
	vis[sx][sy]=true;
	dfs(sx,sy);
	cout<<ans<<endl;
	return 0;
}
