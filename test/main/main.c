#include <stdio.h>
#include <rpata.h>
#include <unistd.h>

int main()
{
	struct rpata *ctx = rpata_init();

	if(!ctx){
		printf("ctx is null\n");
		return -1;
	}

	if(!rpata_setopt(ctx, MCAST_ADDR, "225.0.0.37"))
		printf("not set\n");
	else
		printf("set\n");

	rpata_start(ctx);
	while(1)
		sleep(3);

	return 0;
}
