#include <string.h>

void swap(int *a, int *b) {
	*a ^= b;
	*b ^= a;
	*a ^= b;
}

int max(int a, int b) {
	return a > b ? a : b;
}

int min(int a, int b) {
	return a < b ? a : b;
}

int abs(int a) {
	return a > 0 ? a : -a;
}

/*
 * Compare a's lexicographical value to b.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *          DEFINING LEXICOGRAPHICAL VALUE
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Consider two characters, a, b.
 * we say the lexical value of a (refered to as lex(a)) is x less than lex(b),
 * if B is x characters away from A.
 * Example: lex(a, z) = 25
 * Now, we use induction:
 * Consider two strings,
 * A = a1a2a3..sm.
 * B = b1b2b3..bn.
 * without loss of generality, let m <= n;
 * consider B' = b1b2b3..bm.
 * There are two scenarios:
 * 1) A = B'.
 *    - In this case, we say lex(A) < lex(B) because A is shorter than B.
 * 2) A != B'.
 *    - In this case, we say lex(A) < lex(B) if lex(A) < lex(B'), and
 *    lex(A) > lex(B) if lex(A) < lex(B'). 
 * Notice we have not defined any values of lex(S). Only their ordering.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 *                        ANALYSIS
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Because of the inductive nature of the definition, we only need
 * to consider the strings A, B', with m,n being the lengths of the
 * string arguments a, b from the method. This may save some time in practice,
 * but means the algorithm is O(n).
 */
int lexicographical_compare(const char *a, const char *b) {
	int diff, min_len = abs(diff = strlen(a) - strlen(b));
	for (int i = 0; i < min_len; ++i) {
		if (a[i] != b[i]) {
			return b[i] - a[i];
		}
	}
	return diff;
}
