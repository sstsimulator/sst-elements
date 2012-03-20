
typedef int clockid_t;
struct timespec {
};

long clock_gettime (clockid_t which_clock, struct timespec *tp)
{
    abort();
}
