
        /***********************\
	*			*
	* Built-in test program *
	*			*
        \***********************/

#include "bget.h"

#define TestProg 20000

#define Repeatable  0		      /* Repeatable pseudorandom sequence */
				      /* If Repeatable is not defined, a
					 time-seeded pseudorandom sequence
					 is generated, exercising BGET with
					 a different pattern of calls on each
					 run. */
#define OUR_RAND		      /* Use our own built-in version of
					 rand() to guarantee the test is
					 100% repeatable. */

#ifdef BECtl
#define PoolSize    300000	      /* Test buffer pool size */
#else
#define PoolSize    50000	      /* Test buffer pool size */
#endif
#define ExpIncr     32768	      /* Test expansion block size */
#define CompactTries 10 	      /* Maximum tries at compacting */

#define dumpAlloc   0		      /* Dump allocated buffers ? */
#define dumpFree    0		      /* Dump free buffers ? */

#ifndef Repeatable
#include <stdlib.h>
#endif

#include <malloc.h>
#include <stdio.h>
#include <assert.h>

static char *bchain = NULL;	      /* Our private buffer chain */
static char *bp = NULL; 	      /* Our initial buffer pool */

#include <math.h>

#ifdef OUR_RAND

static unsigned long int next = 1;

/* Return next random integer */

int rand()
{
	next = next * 1103515245L + 12345;
	return (unsigned int) (next / 65536L) % 32768L;
}

/* Set seed for random generator */

void srand(seed)
  unsigned int seed;
{
	next = seed;
}
#endif

/*  STATS  --  Edit statistics returned by bstats() or bstatse().  */

static void stats(bctx *ctx, char *when)
{
    bufsize cural, totfree, maxfree;
    long nget, nfree;
#ifdef BECtl
    bufsize pincr;
    long totblocks, npget, nprel, ndget, ndrel;
#endif

    bstats(ctx, &cural, &totfree, &maxfree, &nget, &nfree);
    printf(
        "%s: %ld gets, %ld releases.  %ld in use, %ld free, largest = %ld\n",
	when, nget, nfree, (long) cural, (long) totfree, (long) maxfree);
#ifdef BECtl
    bstatse(ctx, &pincr, &totblocks, &npget, &nprel, &ndget, &ndrel);
    printf(
         "  Blocks: size = %ld, %ld (%ld bytes) in use, %ld gets, %ld frees\n",
	 (long)pincr, totblocks, pincr * totblocks, npget, nprel);
    printf("  %ld direct gets, %ld direct frees\n", ndget, ndrel);
#endif /* BECtl */
}

#ifdef BECtl
static int protect = 0; 	      /* Disable compaction during bgetr() */

/*  BCOMPACT  --  Compaction call-back function.  */

static int bcompact(bctx *ctx, bufsize bsize, int seq)
{
#ifdef CompactTries
    char *bc = bchain;
    int i = rand() & 0x3;

#ifdef COMPACTRACE
    printf("Compaction requested.  %ld bytes needed, sequence %d.\n",
	(long) bsize, seq);
#endif

    if (protect || (seq > CompactTries)) {
#ifdef COMPACTRACE
        printf("Compaction gave up.\n");
#endif
	return 0;
    }

    /* Based on a random cast, release a random buffer in the list
       of allocated buffers. */

    while (i > 0 && bc != NULL) {
	bc = *((char **) bc);
	i--;
    }
    if (bc != NULL) {
	char *fb;

	fb = *((char **) bc);
	if (fb != NULL) {
	    *((char **) bc) = *((char **) fb);
	    brel(ctx, (void *) fb);
	    return 1;
	}
    }

#ifdef COMPACTRACE
    printf("Compaction bailed out.\n");
#endif
#endif /* CompactTries */
    return 0;
}

/*  BEXPAND  --  Expand pool call-back function.  */

static void *bexpand(bctx *ctx, bufsize size)
{
    void *np = NULL;
    bufsize cural, totfree, maxfree;
    long nget, nfree;

    /* Don't expand beyond the total allocated size given by PoolSize. */

    bstats(ctx, &cural, &totfree, &maxfree, &nget, &nfree);

    if (cural < PoolSize) {
	np = (void *) malloc((unsigned) size);
    }
#ifdef EXPTRACE
    printf("Expand pool by %ld -- %s.\n", (long) size,
        np == NULL ? "failed" : "succeeded");
#endif
    return np;
}

/*  BSHRINK  --  Shrink buffer pool call-back function.  */

static void bshrink(bctx *ctx, void *buf)
{
    if (((char *) buf) == bp) {
#ifdef EXPTRACE
        printf("Initial pool released.\n");
#endif
	bp = NULL;
    }
#ifdef EXPTRACE
    printf("Shrink pool.\n");
#endif
    free((char *) buf);
}

#endif /* BECtl */

/*  Restrict buffer requests to those large enough to contain our pointer and
    small enough for the CPU architecture.  */

static bufsize blimit(bs)
  bufsize bs;
{
    if (bs < sizeof(char *)) {
	bs = sizeof(char *);
    }

    /* This is written out in this ugly fashion because the
       cool expression in sizeof(int) that auto-configured
       to any length int befuddled some compilers. */

    if (sizeof(int) == 2) {
	if (bs > 32767) {
	    bs = 32767;
	}
    } else {
	if (bs > 200000) {
	    bs = 200000;
	}
    }
    return bs;
}

int main()
{
    bctx context;
    binit(&context);
    bctx *ctx = &context;
    int i;
    double x;
    /* Seed the random number generator.  If Repeatable is defined, we
       always use the same seed.  Otherwise, we seed from the clock to
       shake things up from run to run. */

#ifdef Repeatable
    srand(1234);
#else
    srand((int) time((long *) NULL));
#endif

    /*	Compute x such that pow(x, p) ranges between 1 and 4*ExpIncr as
	p ranges from 0 to ExpIncr-1, with a concentration in the lower
	numbers.  */

    x = 4.0 * ExpIncr;
    x = log(x);
    x = exp(log(4.0 * ExpIncr) / (ExpIncr - 1.0));

#ifdef BECtl
    bectl(ctx, bcompact, bexpand, bshrink, (bufsize) ExpIncr);
    bp = malloc(ExpIncr);
    assert(bp != NULL);
    bpool(ctx, (void *) bp, (bufsize) ExpIncr);
#else
    bp = malloc(PoolSize);
    assert(bp != NULL);
    bpool(ctx, (void *) bp, (bufsize) PoolSize);
#endif

    stats(ctx, "Create pool");
    bpoolv(ctx, (void *) bp);
    bpoold(ctx, (void *) bp, dumpAlloc, dumpFree);

    for (i = 0; i < TestProg; i++) {
	char *cb;
	bufsize bs = pow(x, (double) (rand() & (ExpIncr - 1)));

	assert(bs <= (((bufsize) 4) * ExpIncr));
	bs = blimit(bs);
	if (rand() & 0x400) {
	    cb = (char *) bgetz(ctx, bs);
	} else {
	    cb = (char *) bget(ctx, bs);
	}
	if (cb == NULL) {
#ifdef EasyOut
	    break;
#else
	    char *bc = bchain;

	    if (bc != NULL) {
		char *fb;

		fb = *((char **) bc);
		if (fb != NULL) {
		    *((char **) bc) = *((char **) fb);
		    brel(ctx, (void *) fb);
		}
		continue;
	    }
#endif
	}
	*((char **) cb) = (char *) bchain;
	bchain = cb;

	/* Based on a random cast, release a random buffer in the list
	   of allocated buffers. */

	if ((rand() & 0x10) == 0) {
	    char *bc = bchain;
	    int i = rand() & 0x3;

	    while (i > 0 && bc != NULL) {
		bc = *((char **) bc);
		i--;
	    }
	    if (bc != NULL) {
		char *fb;

		fb = *((char **) bc);
		if (fb != NULL) {
		    *((char **) bc) = *((char **) fb);
		    brel(ctx, (void *) fb);
		}
	    }
	}

	/* Based on a random cast, reallocate a random buffer in the list
	   to a random size */

	if ((rand() & 0x20) == 0) {
	    char *bc = bchain;
	    int i = rand() & 0x3;

	    while (i > 0 && bc != NULL) {
		bc = *((char **) bc);
		i--;
	    }
	    if (bc != NULL) {
		char *fb;

		fb = *((char **) bc);
		if (fb != NULL) {
		    char *newb;

		    bs = pow(x, (double) (rand() & (ExpIncr - 1)));
		    bs = blimit(bs);
#ifdef BECtl
		    protect = 1;      /* Protect against compaction */
#endif
		    newb = (char *) bgetr(ctx, (void *) fb, bs);
#ifdef BECtl
		    protect = 0;
#endif
		    if (newb != NULL) {
			*((char **) bc) = newb;
		    }
		}
	    }
	}
    }
    stats(ctx, "\nAfter allocation");
    if (bp != NULL) {
	bpoolv(ctx, (void *) bp);
	bpoold(ctx, (void *) bp, dumpAlloc, dumpFree);
    }

    while (bchain != NULL) {
	char *buf = bchain;

	bchain = *((char **) buf);
	brel(ctx, (void *) buf);
    }
    stats(ctx, "\nAfter release");
#ifndef BECtl
    if (bp != NULL) {
	bpoolv(ctx, (void *) bp);
	bpoold(ctx, (void *) bp, dumpAlloc, dumpFree);
    }
#endif

    return 0;
}
