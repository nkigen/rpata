#include "utils.h"

void rpata_timespec_add_ms(struct timespec *t, uint16_t ms)
{
	t->tv_sec += ms / 1000;
	t->tv_nsec += (ms % 1000) * 1000000;
	if (t->tv_nsec > 1000000000) {
		t->tv_nsec -= 1000000000;
		t->tv_sec += 1;
	}
}

/*
 * t1 -t2
 */
double rpata_time_diffms(struct timespec t1, struct timespec t2)
{
	int64_t sec = 0;
	int64_t nsec = 0;
	if ((t1.tv_nsec - t2.tv_nsec) < 0) {
		sec = t1.tv_sec - t2.tv_sec - 1;
		nsec = 1000000000 + t1.tv_nsec - t2.tv_nsec;
	} else {
		sec = t1.tv_sec - t2.tv_sec;
		nsec = t1.tv_nsec - t2.tv_nsec;
	}
	
	return sec * 1000 + (nsec/1000000);
}
