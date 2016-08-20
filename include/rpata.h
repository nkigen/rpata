#ifndef RPATA_H
#define RPATA_H
#include <stdbool.h>

enum{
	USE_IFACE=0,
	USE_MCAST_ADDR=1,
	USE_MCAST_PORT=2,
};

struct rpata;
struct rpata_callback{
	void(*peer_joined)(char*);
};


struct rpata *rpata_init();
bool rpata_setopt(struct rpata *ctx, int opt, char *val);
bool rpata_start(struct rpata *ctx);
void rpata_setcallback(struct rpata *ctx, struct rpata_callback *cback);
void rpata_destroy(struct rpata *ctx);
#endif /* RPATA_H */
