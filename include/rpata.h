#ifndef RPATA_H
#define RPATA_H
#include <stdbool.h>

enum{
	USE_IFACE=0,
	MCAST_ADDR=1,
};

struct rpata;

struct rpata *rpata_init();
bool rpata_setopt(struct rpata *ctx, int opt, char *val);
bool rpata_start(struct rpata *ctx);
#endif /* RPATA_H */
