#ifndef RPATA_H
#define RPATA_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Foward declaration
 */
struct rpata;
struct rpata_peer;

/*
 * RPATA configuration options used with rpata_setopt(...)
 */
enum{
	WITH_NI,
	WITH_MCAST_ADDR,
	WITH_MCAST_PORT,
	WITH_PERIOD,
	WITH_TIMEOUT,
};

/*
 * user defined callback functions 
 */
struct rpata_callback{
	void(*peer_joined)(char*);	/**< New Peer callback function */
	void(*peer_left)(char*);	/**< AWOL Peer callback function */
};

struct rpata *rpata_init();
bool rpata_setopt(struct rpata *ctx, int opt, char *val);
bool rpata_start(struct rpata *ctx);
void rpata_setcallback(struct rpata *ctx, struct rpata_callback *cback);
int rpata_getsize(struct rpata *ctx);
void rpata_getpeers(struct rpata *ctx, struct rpata_peer **peers, int *num);
void rpata_peer_getipaddr(struct rpata_peer *peer, char *ip, int pos);
void rpata_destroy(struct rpata *ctx);

#ifdef __cplusplus
}
#endif

#endif /* RPATA_H */
