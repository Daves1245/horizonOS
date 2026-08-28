#ifndef FENWICK_H
#define FENWICH_H

/* Insert a value into the fenwick tree */
void insert(int *arr, int i);
/* Update the value at i in a fenwick tree */
void update(int *arr, int i, int value);
/* Range Sum Query the fenwick tree */
long rsq(int *arr, int i, int j);

#endif
