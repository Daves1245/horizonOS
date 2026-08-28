#include <stdio.h>

#include "kruskals.h"
#include "vector.h"

int main() {
	int E; // number of edges
	vector *edgeList;

	/*
     * performance - we'll have to allocate a large block, store each
     * edgeList entry within it, and sort pointers to elements instead.
     * TODO this all needs to be much more readable and elegant
     */
	edgeList = vector_new();
	scanf("%d", &E);
	for (int i = 0; i < E; i++) {
		/* DUMMY CODE - THIS IS WRONG ON PURPOSE */
		struct elem e; // one method to make an edgelist
		// since we don't have vector<int, pair<int, int>>, we
		// make a vector of {int, {int, int}} structs. TODO test performance (later)
		scanf("%d %d %d", &e.pair.u, &e.pair.v, &e.w);
		vector_add(e, edgeList);
	}

	printf("MST cost: %d\n", kruskals(E, edgeList));

	return 0;
}
