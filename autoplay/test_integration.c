/* Engine types are minimal stubs. open_square below is extracted from the supplied source after first-click generation. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "visible_solver.h"
#define MAX_NEIGHBOURS 16
#define MINES_GRID_SQUARE 0
#define snewn(n,t) ((t *)malloc((n)*sizeof(t)))
#define sfree free
struct grid_info {int ntiles,nsize,type; int *neighbours;};
struct mine_layout {bool *mines;};
typedef struct game_state {int w,h; bool dead,won; signed char *board;
struct grid_info *grid; struct mine_layout *layout;} game_state;
static game_state *dup_game(const game_state *s) {
 game_state *r=malloc(sizeof(*r)); *r=*s;
 r->board=malloc(s->grid->ntiles); memcpy(r->board,s->board,s->grid->ntiles); return r;
}
static void free_game(game_state *s) {free(s->board);free(s);}
static int open_square(game_state *state, int tile) {
 int ii, nmines, ncovered;
    if (state->layout->mines[tile]) {
	/*
	 * The player has landed on a mine. Bad luck. Expose the
	 * mine that killed them, but not the rest (in case they
	 * want to Undo and carry on playing).
	 */
	state->dead = true;
	state->board[tile] = 65;
	return -1;
    }

    /*
     * Otherwise, the player has opened a safe square. Mark it to-do.
     */
    state->board[tile] = -10;	       /* `todo' value internal to this func */

    /*
     * Now go through the board finding all `todo' values and
     * opening them. Every time one of them turns out to have no
     * neighbouring mines, we add all its unopened neighbours to
     * the list as well.
     * 
     * FIXME: We really ought to be able to do this better than
     * using repeated N^2 scans of the board.
     */
    while (1) {
	bool done_something = false;
        for (ii = 0; ii < state->grid->ntiles; ii++)
            if (state->board[ii] == -10) {
                int j, ni, v;
                int nsize = state->grid->nsize;

                assert(!state->layout->mines[ii]);

                v = 0;

                for (j = 0; j < state->grid->nsize; j++) {
                    ni = state->grid->neighbours[ii*nsize + j];
                    if (ni >= 0 && state->layout->mines[ni])
                        v++;
                }

                state->board[ii] = v;

                if (v == 0) {
                    for (j = 0; j < state->grid->nsize; j++) {
                        ni = state->grid->neighbours[ii*nsize + j];
                        if (ni >= 0 &&
                            state->board[ni] == -2)
                            state->board[ni] = -10;
                    }
                }

                done_something = true;
            }

	if (!done_something)
	    break;
    }

    /* If the player has already lost, don't let them win as well. */
    if (state->dead) return 0;
    /*
     * Finally, scan the grid and see if exactly as many squares
     * are still covered as there are mines. If so, set the `won'
     * flag and fill in mine markers on all covered squares.
     */
    nmines = ncovered = 0;
    for (ii = 0; ii < state->grid->ntiles; ii++) {
        if (state->board[ii] < 0)
            ncovered++;
        if (state->layout->mines[ii])
            nmines++;
    }
    assert(ncovered >= nmines);
    if (ncovered == nmines) {
        for (ii = 0; ii < state->grid->ntiles; ii++) {
            if (state->board[ii] < 0)
                state->board[ii] = -1;
	}
	state->won = true;
    }

    return 0;
}
static char *solve_game(const game_state *state, const game_state *currstate,
                        const char *aux, const char **error)
{
    const game_state *from = currstate ? currstate : state;
    game_state *sim;
    char *moves, *p;
    int i, tile = -1, mine = 0, status = 0;
    int n = from->grid->ntiles;
    bool has_clue = false;
    (void)aux;

    if (from->dead || from->won) {
        *error = "The game has already ended";
        return NULL;
    }
    for (i = 0; i < n; i++)
        if (from->board[i] >= 0 && from->board[i] <= MAX_NEIGHBOURS)
            has_clue = true;
    if (!has_clue) {
        *error = "Open the first tile manually; no visible clues yet";
        return NULL;
    }
    sim = dup_game(from);
    /* Human flags/questions are hypotheses, never facts. */
    for (i = 0; i < n; i++)
        if (sim->board[i] < 0) sim->board[i] = -2;

    /* Only the normal player action reads the engine's mine layout.
     * vs_next receives just revealed clues and public topology.
     * Never call the original S command or the generator's perturbation.
     */
    while (!sim->won && !sim->dead &&
           (status = vs_next(n, sim->grid->nsize, sim->grid->neighbours,
                             sim->board, &tile, &mine)) == 1) {
        if (mine) sim->board[tile] = -1;
        else open_square(sim, tile);
    }
    if (status < 0 || sim->dead) {
        free_game(sim);
        *error = "Invalid visible constraints or deduction failure";
        return NULL;
    }
    /* Replay legal moves. Include flood-opened tiles too: original human
     * flags may otherwise prevent the engine's normal flood expansion.
     * 64 bytes per tile covers two signed-int coordinates and delimiters.
     */
    moves = snewn((size_t)n * 64 + 1, char);
    p = moves;
    for (i = 0; i < n; i++) {
        const char *cmd = NULL;
        if (from->board[i] >= 0) continue;
        if (sim->board[i] >= 0) cmd = "O";
        else if (sim->board[i] == -1 && from->board[i] != -1) cmd = "PF";
        if (!cmd) continue;
        if (p != moves) *p++ = ';';
        if (from->grid->type == MINES_GRID_SQUARE)
            p += sprintf(p, "%s%d,%d", cmd, i % from->w, i / from->w);
        else
            p += sprintf(p, "%s%d", cmd, i);
    }
    *p = '\0';
    free_game(sim);
    if (p == moves) {
        sfree(moves);
        *error = "No certain move found; stopped without guessing";
        return NULL;
    }
    return moves;
}

int main(void) {
 int adj[24],i,j,k; bool bombs[3]={false,false,true};
 signed char board[3]={0,-1,-2};
 struct grid_info g={3,8,0,adj}; struct mine_layout layout={bombs};
 game_state s={3,1,false,false,board,&g,&layout};
 const char *err=NULL; char *moves;
 for(i=0;i<3;i++) { k=0;for(j=0;j<3;j++) if(abs(i-j)==1) adj[i*8+k++]=j;
 while(k<8) adj[i*8+k++]=-1; }
 moves=solve_game(&s,&s,NULL,&err);
 assert(moves && strstr(moves,"O1,0") && strstr(moves,"PF2,0"));
 assert(board[1]==-1 && board[2]==-2); /* input remains untouched */
 {
  game_state *replayed=dup_game(&s); char *cursor=moves;
  while(*cursor) {
   int x,y;
   if(*cursor=='O') {assert(sscanf(cursor+1,"%d,%d",&x,&y)==2);open_square(replayed,y*3+x);}
   else {assert(sscanf(cursor+2,"%d,%d",&x,&y)==2);replayed->board[y*3+x]=-1;}
   while(*cursor && *cursor!=';') cursor++;
   if(*cursor) cursor++;
  }
  assert(replayed->won && !replayed->dead);
  assert(replayed->board[0]==0 && replayed->board[1]==1 && replayed->board[2]==-1);
  free_game(replayed);
 }
 free(moves);
 board[0]=-2; moves=solve_game(&s,&s,NULL,&err); assert(!moves);
 board[0]=1; board[1]=-2; bombs[0]=false; bombs[1]=true; bombs[2]=false;
 /* Revealed tile 0 has one neighbour, so flag it without opening. */
 moves=solve_game(&s,&s,NULL,&err);assert(moves && strstr(moves,"PF1,0"));free(moves);
 puts("PASS: integration with original open_square (post-first-click), wrong human flag, replay moves, first-click stop");
 return 0;
}
