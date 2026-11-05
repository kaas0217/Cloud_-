#include <assert.h>
#include <stdio.h>
#include "visible_solver.h"
int main(void)
{
    int adj[9*8], i, j, mask, seen, cases = 0, actions = 0;
    for (i=0;i<9;i++) {
        int k=0;
        for(j=0;j<9;j++)
            if(i!=j && abs(i/3-j/3)<=1 && abs(i%3-j%3)<=1) adj[i*8+k++]=j;
        while(k<8) adj[i*8+k++]=-1;
    }
    for(mask=0;mask<512;mask++) for(seen=0;seen<512;seen++) {
        signed char board[9]; int tile=-1, mine=-1, result;
        if(mask & seen) continue;
        for(i=0;i<9;i++) {
            board[i]=-2;
            if(seen & (1<<i)) {
                board[i]=0;
                for(j=0;j<8;j++) if(adj[i*8+j]>=0 && (mask & (1<<adj[i*8+j]))) board[i]++;
            }
        }
        /* Exhaust every hidden layout and safe reveal subset. Repeat
         * inference using only mines that this solver itself proves. */
        do {
            result=vs_next(9,8,adj,board,&tile,&mine);
            assert(result>=0);
            if(result==1) {
                assert(tile>=0 && tile<9 && board[tile]==-2);
                assert(!!(mask & (1<<tile))==mine);
                actions++;
                if(mine) board[tile]=-1;
            }
        } while(result==1 && mine);
        cases++;
    }
    {
        int graph[15]={2,3,-1, 2,3,4, 0,1,-1, 0,1,-1, 1,-1,-1};
        signed char b[5]={1,1,-2,-2,-2}; int t,m;
        assert(vs_next(5,3,graph,b,&t,&m)==1 && t==4 && m==0);
        b[1]=2;
        assert(vs_next(5,3,graph,b,&t,&m)==1 && t==4 && m==1);
        b[0]=3;
        assert(vs_next(5,3,graph,b,&t,&m)==-1);
    }
    {
        int graph[6]={1,2,0,-1,0,-1};
        signed char b[3]={1,-2,-2}; int t,m;
        assert(vs_next(3,2,graph,b,&t,&m)==0);
    }
    printf("PASS: %d visible/layout cases, %d sound deductions; subset, ambiguity, contradiction\n",cases,actions);
    return 0;
}
