#include <stdio.h>
#include <malloc.h>

#include "bget.h"

int main() {
    bctx context;
    binit(&context);
    void *p = malloc(4096);
    
    bpool(&context, p, 4096);
    
    void *x0 = bget(&context, 1000);
    printf("%016x\n", x0);
    brel(&context, x0);
    
    void *x1 = bget(&context, 100);
    printf("%016x\n", x1);
    brel(&context, x1);
    
    void *x2 = bget(&context, 2000);
    printf("%016x\n", x2);
    brel(&context, x2);
    
    void *x3 = bget(&context, 3000);
    printf("%016x\n", x3);
    brel(&context, x3);
    
    void *x4 = bget(&context, 5000);
    printf("%016x\n", x4);
    
    return 0;
}