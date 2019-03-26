#ifndef _BGET_H
#define _BGET_H

#define SizeQuant   4		      /* Buffer allocation size quantum:
					 all buffers allocated are a
					 multiple of this size.  This
					 MUST be a power of two. */

#define BufDump     1		      /* Define this symbol to enable the
					 bpoold() function which dumps the
					 buffers in a buffer pool. */

#define BufValid    1		      /* Define this symbol to enable the
					 bpoolv() function for validating
					 a buffer pool. */ 

#define DumpData    1		      /* Define this symbol to enable the
					 bufdump() function which allows
					 dumping the contents of an allocated
					 or free buffer. */

#define BufStats    1		      /* Define this symbol to enable the
					 bstats() function which calculates
					 the total free space in the buffer
					 pool, the largest available
					 buffer, and the total space
					 currently allocated. */

#define FreeWipe    1		      /* Wipe free buffers to a guaranteed
					 pattern of garbage to trip up
					 miscreants who attempt to use
					 pointers into released buffers. */

#define BestFit     1		      /* Use a best fit algorithm when
					 searching for space for an
					 allocation request.  This uses
					 memory more efficiently, but
					 allocation will be much slower. */

#define BECtl	    1		      /* Define this symbol to enable the
					 bectl() function for automatic
					 pool space control.  */



typedef long bufsize;

/* Queue links */

struct qlinks {
    struct bfhead *flink;	      /* Forward link */
    struct bfhead *blink;	      /* Backward link */
};


/* Header in allocated and free buffers */

struct bhead {
    bufsize prevfree;		      /* Relative link back to previous
					 free buffer in memory or 0 if
					 previous buffer is allocated.	*/
    bufsize bsize;		      /* Buffer size: positive if free,
					 negative if allocated. */
};

/*  Header in directly allocated buffers (by acqfcn) */

struct bdhead {
    bufsize tsize;		      /* Total size, including overhead */
    struct bhead bh;		      /* Common header */
};

/* Header in free buffers */

struct bfhead {
    struct bhead bh;		      /* Common allocated/free header */
    struct qlinks ql;		      /* Links on free list */
};

typedef struct bctx {
    struct bfhead freelist;               /* List of free buffers */
    void *p;
#ifdef BufStats
    bufsize totalloc;	      /* Total space currently allocated */
    long numget, numrel;   /* Number of bget() and brel() calls */
#ifdef BECtl
    long numpblk;	      /* Number of pool blocks */
    long numpget, numprel; /* Number of block gets and rels */
    long numdget, numdrel; /* Number of direct gets and rels */
#endif /* BECtl */
#endif /* BufStats */

#ifdef BECtl

/* Automatic expansion block management functions */

    int (*compfcn) (struct bctx *ctx, bufsize sizereq, int sequence);
    void *(*acqfcn) (struct bctx *ctx, bufsize size);
    void (*relfcn) (struct bctx *ctx, void *buf);

    bufsize exp_incr;	      /* Expansion block size */
    bufsize pool_len;	      /* 0: no bpool calls have been made
					 -1: not all pool blocks are
					     the same size
					 >0: (common) block size for all
					     bpool calls made so far
				      */
#endif
} bctx;

void	bpool	    (bctx *ctx, void *buffer, bufsize len);
void   *bget	    (bctx *ctx, bufsize size);
void   *bgetz	    (bctx *ctx, bufsize size);
void   *bgetr	    (bctx *ctx, void *buffer, bufsize newsize);
void	brel	    (bctx *ctx, void *buf);
void	bectl	    (bctx *ctx, 
               int (*compact)(bctx *ctx, bufsize sizereq, int sequence),
		       void *(*acquire)(bctx *ctx, bufsize size),
		       void (*release)(bctx *ctx, void *buf), bufsize pool_incr);
void	bstats	    (bctx *ctx, bufsize *curalloc, bufsize *totfree,
               bufsize *maxfree, long *nget, long *nrel);
void	bstatse     (bctx *ctx,  bufsize *pool_incr, long *npool, 
               long *npget, long *nprel, long *ndget, long *ndrel);
void	bufdump     (bctx *ctx, void *buf);
void	bpoold	    (bctx *ctx, void *pool, int dumpalloc, int dumpfree);
int	    bpoolv	    (bctx *ctx, void *pool);
void    binit       (bctx *ctx);

#endif