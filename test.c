
        /***********************\
	*			*
	* Built-in test program *
	*			*
        \***********************/

#ifdef TestProg

#define Repeatable  1		      /* Repeatable pseudorandom sequence */
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
extern long time();
#endif

extern char *malloc();
extern int free _((char *));

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

static void stats(when)
  char *when;
{
    bufsize cural, totfree, maxfree;
    long nget, nfree;
#ifdef BECtl
    bufsize pincr;
    long totblocks, npget, nprel, ndget, ndrel;
#endif

    bstats(&cural, &totfree, &maxfree, &nget, &nfree);
    V printf(
        "%s: %ld gets, %ld releases.  %ld in use, %ld free, largest = %ld\n",
	when, nget, nfree, (long) cural, (long) totfree, (long) maxfree);
#ifdef BECtl
    bstatse(&pincr, &totblocks, &npget, &nprel, &ndget, &ndrel);
    V printf(
         "  Blocks: size = %ld, %ld (%ld bytes) in use, %ld gets, %ld frees\n",
	 (long)pincr, totblocks, pincr * totblocks, npget, nprel);
    V printf("  %ld direct gets, %ld direct frees\n", ndget, ndrel);
#endif /* BECtl */
}

#ifdef BECtl
static int protect = 0; 	      /* Disable compaction during bgetr() */

/*  BCOMPACT  --  Compaction call-back function.  */

static int bcompact(bsize, seq)
  bufsize bsize;
  int seq;
{
#ifdef CompactTries
    char *bc = bchain;
    int i = rand() & 0x3;

#ifdef COMPACTRACE
    V printf("Compaction requested.  %ld bytes needed, sequence %d.\n",
	(long) bsize, seq);
#endif

    if (protect || (seq > CompactTries)) {
#ifdef COMPACTRACE
        V printf("Compaction gave up.\n");
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
	    brel((void *) fb);
	    return 1;
	}
    }

#ifdef COMPACTRACE
    V printf("Compaction bailed out.\n");
#endif
#endif /* CompactTries */
    return 0;
}

/*  BEXPAND  --  Expand pool call-back function.  */

static void *bexpand(size)
  bufsize size;
{
    void *np = NULL;
    bufsize cural, totfree, maxfree;
    long nget, nfree;

    /* Don't expand beyond the total allocated size given by PoolSize. */

    bstats(&cural, &totfree, &maxfree, &nget, &nfree);

    if (cural < PoolSize) {
	np = (void *) malloc((unsigned) size);
    }
#ifdef EXPTRACE
    V printf("Expand pool by %ld -- %s.\n", (long) size,
        np == NULL ? "failed" : "succeeded");
#endif
    return np;
}

/*  BSHRINK  --  Shrink buffer pool call-back function.  */

static void bshrink(buf)
  void *buf;
{
    if (((char *) buf) == bp) {
#ifdef EXPTRACE
        V printf("Initial pool released.\n");
#endif
	bp = NULL;
    }
#ifdef EXPTRACE
    V printf("Shrink pool.\n");
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
    int i;
    double x;

    /* Seed the random number generator.  If Repeatable is defined, we
       always use the same seed.  Otherwise, we seed from the clock to
       shake things up from run to run. */

#ifdef Repeatable
    V srand(1234);
#else
    V srand((int) time((long *) NULL));
#endif

    /*	Compute x such that pow(x, p) ranges between 1 and 4*ExpIncr as
	p ranges from 0 to ExpIncr-1, with a concentration in the lower
	numbers.  */

    x = 4.0 * ExpIncr;
    x = log(x);
    x = exp(log(4.0 * ExpIncr) / (ExpIncr - 1.0));

#ifdef BECtl
    bectl(bcompact, bexpand, bshrink, (bufsize) ExpIncr);
    bp = malloc(ExpIncr);
    assert(bp != NULL);
    bpool((void *) bp, (bufsize) ExpIncr);
#else
    bp = malloc(PoolSize);
    assert(bp != NULL);
    bpool((void *) bp, (bufsize) PoolSize);
#endif

    stats("Create pool");
    V bpoolv((void *) bp);
    bpoold((void *) bp, dumpAlloc, dumpFree);

    for (i = 0; i < TestProg; i++) {
	char *cb;
	bufsize bs = pow(x, (double) (rand() & (ExpIncr - 1)));

	assert(bs <= (((bufsize) 4) * ExpIncr));
	bs = blimit(bs);
	if (rand() & 0x400) {
	    cb = (char *) bgetz(bs);
	} else {
	    cb = (char *) bget(bs);
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
		    brel((void *) fb);
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
		    brel((void *) fb);
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
		    newb = (char *) bgetr((void *) fb, bs);
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
    stats("\nAfter allocation");
    if (bp != NULL) {
	V bpoolv((void *) bp);
	bpoold((void *) bp, dumpAlloc, dumpFree);
    }

    while (bchain != NULL) {
	char *buf = bchain;

	bchain = *((char **) buf);
	brel((void *) buf);
    }
    stats("\nAfter release");
#ifndef BECtl
    if (bp != NULL) {
	V bpoolv((void *) bp);
	bpoold((void *) bp, dumpAlloc, dumpFree);
    }
#endif

    return 0;
}
#endif
