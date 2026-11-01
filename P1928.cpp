#include<iostream>
#include<vector>
#include<string>
using namespace std;
vector<char> k;   
bool z(){
    int l=-1;
    for(int i=0;i<(int)k.size();i++)
        if(k[i]=='[') l=i;          
    if(l==-1) return false;         
    int r=-1;
    for(int i=l+1;i<(int)k.size();i++)
        if(k[i]==']'){ r=i; break; } 
    int num=0, i=l+1;
    while(i<r && k[i]>='0' && k[i]<='9'){
        num = num*10 + (k[i]-'0');  
        i++;
    }
    string t;
    for(int j=i;j<r;j++) t += k[j]; 
    	string rep;
    for(int q=0;q<num;q++) rep += t;
    	vector<char> nv;
    for(int j=0;j<l;j++) nv.push_back(k[j]);
    for(char c:rep) nv.push_back(c);
    for(int j=r+1;j<(int)k.size();j++) nv.push_back(k[j]);
    k = nv;                           
    return true;                      
}
int main(){
	char c;
	while(cin>>c) k.push_back(c);
	while(z()){}
	for(int i=0; i<(int)k.size(); i++){
   		char c = k[i];
  		cout << c;
	}
	return 0;
}

