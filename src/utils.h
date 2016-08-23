#ifndef RPATA_UTILS_H
#define RPATA_UTILS_H
#include <time.h>
#include <inttypes.h>

void rpata_timespec_add_ms(struct timespec *t, uint16_t ms);
double rpata_time_diffms(struct timespec t1, struct timespec t2);

#endif /* RPATA_UTILS_H */

