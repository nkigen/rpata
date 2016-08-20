#ifndef RPATA_H
#define RPATA_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum{
	USE_IFACE=0,
	USE_MCAST_ADDR=1,
	USE_MCAST_PORT=2,
};

struct rpata;
struct rpata_peer;
struct rpata_callback{
	void(*peer_joined)(char*);
};


struct rpata *rpata_init();
bool rpata_setopt(struct rpata *ctx, int opt, char *val);
bool rpata_start(struct rpata *ctx);
void rpata_setcallback(struct rpata *ctx, struct rpata_callback *cback);
void rpata_getpeers(struct rpata *ctx, struct rpata_peer **peers, int *num);
void rpata_peer_getipaddr(struct rpata_peer *peer, char *ip, int pos);
void rpata_destroy(struct rpata *ctx);

#ifdef __cplusplus
}
#endif
#endif /* RPATA_H */
