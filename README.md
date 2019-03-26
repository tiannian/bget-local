# bget-local

[bget](http://www.fourmilab.ch/bget/) is a comprehensive memory allocator. It can use in a variety of environments. You can't use multiple bget instances in the same program, because it uses the global variable sharing state.

We use a `struct` instead of the global state, so that `bget` can have multiple instances in the same program.

## Usage

Create a `bctx` and initialize it.

```c
bctx ctx;
binit(&bctx);
```

Create a buffer and pool it into `bget`.

```c
uint8_t buffer[4096];
bpool(&ctx, buffer, 4096);
```

Then you can use `bget` and `brel` to allocate or release memory.

```c
void *p = bget(&ctx, 1000);
brel(&ctx, p);
```

Other usage and configuration reference to http://www.fourmilab.ch/bget .

