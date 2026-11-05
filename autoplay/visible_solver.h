/* Public-information-only Minesweeper deduction. No game/layout dependency. */
#ifndef VISIBLE_SOLVER_H
#define VISIBLE_SOLVER_H
#include <stdlib.h>
#define VS_DEGREE 16
struct vs_constraint { int cell[VS_DEGREE], size, mines; };
/* board: >=0 revealed clue, -1 PROVEN mine, -2 unknown.
 * Returns 1: action (mine=0 open, mine=1 flag), 0: stalled,
 * -1: inconsistent/invalid input, -2: allocation failure.
 * Manual flags MUST be converted to unknown by the caller initially.
 */
static int vs_next(int n, int degree, const int *adj,
                   const signed char *board, int *tile, int *mine)
{
    struct vs_constraint *c;
    int count = 0, i, j, a, b, result = 0;
    if (n <= 0 || degree <= 0 || degree > VS_DEGREE) return -1;
    c = calloc((size_t)n, sizeof(*c));
    if (!c) return -2;
    for (i = 0; i < n; i++) {
        struct vs_constraint *s;
        if (board[i] < -2 || board[i] > degree) { result = -1; goto out; }
        if (board[i] < 0) continue;
        s = &c[count++];
        s->mines = board[i];
        for (j = 0; j < degree; j++) {
            int v = adj[i*degree+j], k;
            if (v == -1) continue;
            if (v < 0 || v >= n || v == i) { result = -1; goto out; }
            for (k = 0; k < j; k++)
                if (adj[i*degree+k] == v) { result = -1; goto out; }
            if (board[v] == -1) s->mines--;
            else if (board[v] == -2) s->cell[s->size++] = v;
        }
        if (s->mines < 0 || s->mines > s->size) { result = -1; goto out; }
    }
    /* Every constraint is sum(unknown neighbours) = remaining mines. */
    for (a = 0; a < count; a++) {
        if (c[a].size && (c[a].mines == 0 || c[a].mines == c[a].size)) {
            *tile = c[a].cell[0]; *mine = c[a].mines != 0;
            result = 1; goto out;
        }
    }
    /* If A is a subset of B, subtract: sum(B-A) = mines(B)-mines(A). */
    for (a = 0; a < count; a++) for (b = 0; b < count; b++) {
        int subset = 1, difference, remaining, candidate = -1;
        if (a == b || !c[a].size || c[a].size > c[b].size) continue;
        for (i = 0; i < c[a].size; i++) {
            for (j = 0; j < c[b].size; j++)
                if (c[a].cell[i] == c[b].cell[j]) break;
            if (j == c[b].size) { subset = 0; break; }
        }
        if (!subset) continue;
        difference = c[b].size - c[a].size;
        remaining = c[b].mines - c[a].mines;
        if (remaining < 0 || remaining > difference) { result = -1; goto out; }
        if (!difference || (remaining != 0 && remaining != difference)) continue;
        for (j = 0; j < c[b].size; j++) {
            for (i = 0; i < c[a].size; i++)
                if (c[b].cell[j] == c[a].cell[i]) break;
            if (i == c[a].size) { candidate = c[b].cell[j]; break; }
        }
        *tile = candidate; *mine = remaining != 0; result = 1; goto out;
    }
out:
    free(c);
    return result;
}
#endif
