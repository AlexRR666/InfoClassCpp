#include<bits/stdc++.h> 
using namespace std;
int main(int argc,char**argv){
	int a;
	bool b=0;
	cin>>a;
	if(a<0) a*=-1;b=1;
	string c=to_string(a);
	reverse(c.begin(), c.end()); 
	a=stoi(c);
	if(b=1)a*=-1;
	cout<<a;
	return 0;
}
