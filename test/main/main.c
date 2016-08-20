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

	if(!rpata_setopt(ctx, USE_IFACE, "lo"))
		printf("not found\n");
	else
		printf("found\n");

	rpata_start(ctx);
	sleep(3);

	return 0;
}
