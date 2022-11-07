#include "clock.h"

#include <time.h>

const unsigned long long NSEC_PER_SEC = 1000000000;

void get_time(struct timespec *out)
{
	clock_gettime(CLOCK_MONOTONIC, out);
}

void timespec_subtract(const struct timespec *x, const struct timespec *y, struct timespec *result)
{
	long sec = 0;
	if (x->tv_nsec < y->tv_nsec) {
		sec = (y->tv_nsec - x->tv_nsec + NSEC_PER_SEC - 1) / NSEC_PER_SEC;
	}
	result->tv_sec = x->tv_sec - (y->tv_sec + sec);
	result->tv_nsec = x->tv_nsec - (y->tv_nsec - NSEC_PER_SEC * sec);
}

vm_clock_t get_vm_clock(const struct timespec *ref)
{
	struct timespec now;
	get_time(&now);
	struct timespec diff;
	timespec_subtract(&now, ref, &diff);
	return diff.tv_nsec + diff.tv_sec * NSEC_PER_SEC;
}

long vm_clock_as_usec(vm_clock_t clk)
{
	return clk / 1000;
}
