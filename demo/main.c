#include <stdio.h>
#include <stdlib.h>
#include <rpata.h>
#include <unistd.h>
#include <string.h>

#define MCAST_IP "225.0.0.37"
#define MCAST_PORT "18000" 
#define MCAST_NI "wlo1"

void new_peer(char *ip)
{
	printf("new peer %s has joined\n", ip);
}

void peer_left(char *ip)
{
	printf("peer %s has gone AWOL\n", ip);
}

struct rpata_callback cback = {.peer_joined = new_peer, .peer_left = peer_left};

int main(int argc, char *argv[])
{
	struct rpata *ctx = rpata_init();

	if(!ctx){
		printf("ctx is null\n");
		return -1;
	}

	if(rpata_setopt(ctx, WITH_MCAST_ADDR, MCAST_IP) &&
			rpata_setopt(ctx, WITH_MCAST_PORT, MCAST_PORT) &&
			rpata_setopt(ctx, WITH_NI, MCAST_NI)){
		printf("all set\n");
	}
	else{
		printf("all NOT set!!\n");
		return -1;
	}

	rpata_setcallback(ctx, &cback); 
	rpata_start(ctx);

	int num = 0;
	char **peers = NULL;
	while(1){
		peers = rpata_getpeers(ctx, &num);
		printf("******peer list start******\nnum %d\n", num);
		for(int i = 0; i < num; ++i){
			printf("ip %s\n", peers[i]);
		}
		printf("******peer list end******\n");
		rpata_freepeers(ctx, peers, num);
		sleep(5);
	}

	return 0;
}
