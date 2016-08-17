#include <stdio.h>
#include <rpata.h>



int main()
{
	struct rpata *ctx = rpata_init();

	if(!ctx){
		printf("ctx is null\n");
		return -1;
	}
}
