#include "kruskals.h"
#include "ufds.h"
#include "vector.h"

int edge_comp(const void *a, const void *b) {
	return ((struct edge *)a)->w - ((struct edge *)b)->w;
}

/* Return the cost of a MST of the given graph */
int kruskals(int E, struct edge *elist) {
	struct ufds *unionfind = UnionFind(E);
	int cost = 0;
	qsort(elist, E, sizeof(struct edge), edge_comp);
	for (int i = 0; i < E; i++) {
		struct elem *e = vector_get(edgeList, i);
		if (!in_same_set(e->pair.u, e->pair.v, unionfind)) {
			cost += e->w;
			/*
             * Because this operation is amortized constant time,
             * kruskals runs at O(ElgV) and the main runtime cost
             * is in sorting the edgeList.
             */
			unionSet(e->pair.u, e->pair.v, unionfind);
		}
		if (unionfind->num_sets == 1) {
			/* Stop if we've collected every vertex - this forms an MST */
			return cost;
		}
	}
	return cost;
}
