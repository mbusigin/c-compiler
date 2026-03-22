#include <stdio.h>


int recursive_divider( float n, float d )
{
    printf( "%f %f\n", n, d );
    n = n / d;
    printf( "%f %f\n", n, d );
    return recursive_divider( n, d );
}

int main() { int n1 = 33; int n2 = n1 * 344;
recursive_divider(n1, n2); return 0; };
