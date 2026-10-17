#include <iostream>
using namespace std;
const int MAXN = 999;
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
int main()
{
    int n;
    cin >> n;
    if(n<1 || n>999) return 0;
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            mat[i][j]=0;
            t[i][j]=0;
        }
    }

    if(n%2 == 1)
        oddMagic(n);
    else if(n%4 == 0)
        doubleEvenMagic(n);
    else
        singleEvenMagic(n);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            if(j>0) cout<<" ";
            cout<<mat[i][j];
        }
        cout<<endl;
    }
    return 0;
}
