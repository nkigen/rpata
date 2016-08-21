#include <stdio.h>
#include <stdlib.h>
#include <rpata.h>
#include <unistd.h>

void new_peer(char *ip)
{
	printf("new peer %s has joined\n", ip);
}

struct rpata_callback cback = {.peer_joined = new_peer};

int main()
{
	struct rpata *ctx = rpata_init();

	if(!ctx){
		printf("ctx is null\n");
		return -1;
	}

	if(rpata_setopt(ctx, WITH_MCAST_ADDR, "225.0.0.37") &&
			rpata_setopt(ctx, WITH_MCAST_PORT, "18000") &&
			rpata_setopt(ctx, WITH_NI, "wlo1")){
		printf("all set\n");
	}
	else{
		printf("all NOT set!!\n");
		return -1;
	}

	rpata_setcallback(ctx, &cback); 
	rpata_start(ctx);

	while(1){
		struct rpata_peer *peers = NULL;
		int num;
		rpata_getpeers(ctx, &peers, &num);
		char ip[20];
		for(int i = 0; i < num; ++i){
			rpata_peer_getipaddr(peers, ip, i);
			printf("ip %s\n", ip);
		}
		free(peers);
		sleep(10);
	}

	return 0;
}