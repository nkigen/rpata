#include <stdio.h>
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

	if(rpata_setopt(ctx, USE_MCAST_ADDR, "225.0.0.37") &&
			rpata_setopt(ctx, USE_MCAST_PORT, "18000"))
		printf("all set\n");
	else
		printf("all NOT set!!\n");

	rpata_setcallback(ctx, &cback); 
	rpata_start(ctx);

	while(1)
		sleep(3);

	return 0;
}
