#ifndef KRUSKALS_H
#define KRUSKALS_H

#include "vector.h"

/* We represent an edge as a 3-tuple (u, v, w)
 * where u and v are vertices connected by the edge
 * and w is the weight
 */
struct edge {
	int u, v, w;
}

/* Returns the cost of a MST of the graph depicted by edgeList */
int kruskals(int num_edges, vector *edgeList);

#endif
