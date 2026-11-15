#include<bits/stdc++.h>
using namespace std;
void aaa(char from,char to,char middle,int p){
	if(p==0)return;
	aaa(from,middle,to,p-1);
	cout<<from<<"->"<<to<<endl;
	aaa(middle,to,from,p-1);
	return;
}
int main(){
	int n;
	cin>>n;
	aaa('A','C','B',n);
	return 0;
}
