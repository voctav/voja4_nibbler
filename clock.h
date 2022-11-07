#ifndef _CLOCK_H
#define _CLOCK_H

#include <time.h>

/* Measures time elapsed in nanoseconds since a given reference time. */
typedef long long vm_clock_t;

/* Returns time from a monotonic clock. */
void get_time(struct timespec *out);

/* Returns a clock value measured from the given reference. */
vm_clock_t get_vm_clock(const struct timespec *ref);

/* Returns time elapsed as microseconds. */
long vm_clock_as_usec(vm_clock_t clk);


#endif /* _CLOCK_H */
