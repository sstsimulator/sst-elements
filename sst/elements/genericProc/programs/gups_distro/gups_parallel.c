#include "gups_sysheaders.h"
#include "gups_types.h"

#define ln2 0.69314718055994530942

unsigned long *indices;

#ifdef GETTIMEOFDAY_IS_BROKEN
# define gettimeofday fake_gettimeofday

int fake_gettimeofday(struct timeval *tp, void *tzp)
{
	tp->tv_sec = time(NULL);
	tp->tv_usec = 0;
}
#endif /* GETTIMEOFDAY_IS_BROKEN */

#ifdef RANDOM_IS_LFSR
# define random lfsr

/* Maximal length lfsr feedback terms for bit length > 4. */
/* From http://www.cs.cmu.edu/~koopman/lfsr */
long feedbackterms[] = {
  0x0, 0x0, 0x0, 0x0, 0x9, 0x12, 0x21, 0x41, 0x8e, 0x108, 0x204, 0x402,
  0x829, 0x100d, 0x2015, 0x4001, 0x8016, 0x10004, 0x20013, 0x40013, 0x80004,
  0x100002, 0x200001, 0x400010, 0x80000d, 0x1000004, 0x2000023, 0x4000013,
  0x8000004, 0x10000002, 0x20000029, 0x40000004, 0x80000057	/*, 0x100000029,
  0x200000073, 0x400000002, 0x80000003B, 0x100000001F, 0x2000000031,
  0x4000000008 */
};

unsigned int lfsr_bits = 0;
unsigned int lfsr_state = 0;
unsigned int
lfsr (void)
{
  if (lfsr_state & 0x1)
    {
      lfsr_state = (lfsr_state >> 1) ^ feedbackterms[lfsr_bits];
    }
  else
    {
      lfsr_state = (lfsr_state >> 1);
    }
  return lfsr_state;
}

#endif /* RANDOM_IS_LFSR */

#ifdef DO_RANDOMS_BEFOREHAND
# define GET_NEXT_INDEX(i,size) indices[i]
#else
# define GET_NEXT_INDEX(i,size) random () % size
#endif /* DO_RANDOMS_BEFOREHAND */

/* GUPS for 8-bit wide data, serial */
void
gups8 (uint8 *field, unsigned long iters, unsigned long size)
{
  uint8 data;
  unsigned long i;
  unsigned long long index;

#ifdef _CRAYC
# pragma _CRI concurrent
#endif
  for (i = 0; i < iters; i++)
    {
      field[indices[i]] += 1;
    }
}

/* GUPS for 16-bit wide data, serial */
void
gups16 (uint16 *field, unsigned long iters, unsigned long size)
{
  uint16 data;
  unsigned long i;
  unsigned long long index;

#ifdef _CRAYC
# pragma _CRI concurrent
#endif
  for (i = 0; i < iters; i++)
    {
      field[indices[i]] += 1;
    }
}

/* GUPS for 32-bit wide data, serial */
void
gups32 (uint32 *field, unsigned long iters, unsigned long size)
{
  uint32 data;
  unsigned long i;
  unsigned long long index;

#ifdef _CRAYC
# pragma _CRI concurrent
#endif
  for (i = 0; i < iters; i++)
    {
      field[indices[i]] += 1;
    }
}

/* GUPS for 64-bit wide data, serial */
void
gups64 (uint64 *field, unsigned long iters, unsigned long size)
{
  uint64 data;
  unsigned long i;
  unsigned long long index;

#ifdef _CRAYC
# pragma _CRI concurrent
#endif
  for (i = 0; i < iters; i++)
    {
      field[indices[i]] += 1;
    }
}

void
timetest (void *field, unsigned long iters, unsigned long size)
{
  uint8 data;
  unsigned long i;
  unsigned long long index;

  for (i = 0; i < iters; i++)
    {
      index = random (); /* % size; */
      /* data = field[index]; */
      /* data = data + ((uint8) iters); */
      /* field[index] = data; */
    }
}

/* Returns the number of seconds encoded in T, a "struct timeval". */
#define tv_to_double(t) (t.tv_sec + (t.tv_usec / 1000000.0))

/* Useful for doing arithmetic on struct timevals. */
void
timeDiff (struct timeval *d, struct timeval *a, struct timeval *b)
{
  d->tv_sec = a->tv_sec - b->tv_sec;
  d->tv_usec = a->tv_usec - b->tv_usec;
  if (d->tv_usec < 0)
    {
      d->tv_sec -= 1;
      d->tv_usec += 1000000;
    }
}

/* Return the no. of elapsed seconds between Starttime and Endtime. */
double
elapsed (struct timeval *starttime, struct timeval *endtime)
{
  struct timeval diff;

  timeDiff (&diff, endtime, starttime);
  return tv_to_double (diff);
}

void
calc_indices (uint64 updates, uint64 nelems)
{
  unsigned int i;

  indices = (unsigned long *) malloc (updates * sizeof (unsigned long));
  if (!indices)
    {
      printf ("Error: couldn't malloc space for array of indices\n");
      assert (indices != NULL);	/* abort here */
    }
  for (i = 0; i < updates; i++)
    {
      indices[i] = random () % nelems;
    }
}

int
main (int argc, char **argv)
{
  uint64 updates;
  float expt;
  uint64 width;
  uint64 size, elt_size, nelems;
  void *field;
  uint8 *field8;
  uint16 *field16;
  uint32 *field32;
  uint64 *field64;
  struct timeval starttime, stoptime;
  double secs, gups, randtime = 0;

  if (sizeof (uint8) != 1)
    {
      printf ("Warning: uint8 is %i bits. Check typedefs and recompile.\n",
	      8 * sizeof (uint8));
    }
  if (sizeof (uint16) != 2)
    {
      printf ("Warning: uint16 is %i bits. Check typedefs and recompile.\n",
	      8 * sizeof (uint16));
    }
  if (sizeof (uint32) != 4)
    {
      printf ("Warning: uint32 is %i bits. Check typedefs and recompile.\n",
	      8 * sizeof (uint32));
    }
  if (sizeof (uint64) != 8)
    {
      printf ("Warning: uint64 is %i bits. Check typedefs and recompile.\n",
	      8 * sizeof (uint64));
    }

  scanf ("%llu %f %llu", &updates, &expt, &width);
  assert (width == 8 || width == 16 || width == 32 || width == 64);
  assert (expt > 8.0);
  assert (updates > 0 && (updates % 256 == 0));
  size = (uint64) exp (expt * ln2);
  size -= (size % 256);
  assert (size > 0 && (size % 256 == 0));

  printf ("%llu updates, ", updates);
  printf ("%llu-bit-wide data, ", width);
  printf ("field of 2^%.2f (%llu) bytes.\n", expt, size);

  field = malloc (size);
  if (field == NULL)
    {
      printf ("Error: Failed to malloc %llu bytes.\n", size);
      assert (field != NULL);	/* abort here */
    }

#ifdef RANDOM_IS_LFSR
  lfsr_bits = ((int) ceil (expt));
  printf ("Selecting lfsr width %d.\n", lfsr_bits);
#endif /* RANDOM_IS_LFSR */

  switch (width)
    {
    case 8:
      field8 = (uint8 *) field;
      assert (sizeof (uint8) == 1);
      elt_size = sizeof (uint8);
      break;
    case 16:
      field16 = (uint16 *) field;
      assert (sizeof (uint16) == 2);
      elt_size = sizeof (uint16);
      break;
    case 32:
      field32 = (uint32 *) field;
      assert (sizeof (uint32) == 4);
      elt_size = sizeof (uint32);
      break;
    case 64:
      field64 = (uint64 *) field;
      assert (sizeof (uint64) == 8);
      elt_size = sizeof (uint64);
      break;
    }

  printf ("Element size is %llu bytes.\n", elt_size);
  nelems = size / elt_size;
  printf ("Field is %llu data elements starting at 0x%08lx.\n", nelems,
	  (unsigned long) field);

#ifdef DO_RANDOMS_BEFOREHAND
  printf ("Calculating indices.\n");
  calc_indices (updates, nelems);
#else
  printf ("Calibrating.\n");
  gettimeofday (&starttime, NULL);
  timetest (field, updates, nelems);
  gettimeofday (&stoptime, NULL);
  randtime = elapsed (&starttime, &stoptime);
#endif

  printf ("Timing.\n");
  switch (width)
    {
    case 8:
      gettimeofday (&starttime, NULL);
      gups8 (field8, updates, nelems);
      gettimeofday (&stoptime, NULL);
      break;
    case 16:
      gettimeofday (&starttime, NULL);
      gups16 (field16, updates, nelems);
      gettimeofday (&stoptime, NULL);
      break;
    case 32:
      gettimeofday (&starttime, NULL);
      gups32 (field32, updates, nelems);
      gettimeofday (&stoptime, NULL);
      break;
    case 64:
      gettimeofday (&starttime, NULL);
      gups64 (field64, updates, nelems);
      gettimeofday (&stoptime, NULL);
      break;
    }

  secs = elapsed (&starttime, &stoptime);
  secs -= randtime;
  printf ("Elapsed time: %.4f seconds.\n", secs);
  gups = ((double) updates) / (secs * 1.0e9);
  printf ("GUPS = %.10f\n", gups);

  return 0;
}
