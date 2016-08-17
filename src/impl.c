#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <rpata.h>
#include "impl.h"

static bool ipaddr_init(struct ipaddr *ip)
{
	FILE *fp = popen("ls -A /sys/class/net", "r");
	if(!fp)
		return -1;

	char out[10];
	ip->nlen = 0;
	while(fgets(out, 9, fp))
		++ip->nlen;

	if(!ip->nlen)
		return false;

	ip->ips = calloc(ip->nlen, sizeof *ip->ips);
	if(!ip->ips)
		return false;

	return true;
}

static bool iface_init(struct rpata *ctx)
{
	ctx->ips = malloc(sizeof *ctx->ips);
	if(!ctx->ips)
		return false;

	if(!ipaddr_init(ctx->ips))
		return false;

	struct ifaddrs *ifaddr, *ifa;
	char host[NI_MAXHOST];
	int s, i;

	if(-1 == getifaddrs(&ifaddr))
		return false;

	for(ifa = ifaddr, i = 0; ifa != NULL; ifa = ifa->ifa_next, ++i) {
		if(NULL == ifa->ifa_addr)
			continue;

		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (0 != s) {
				goto err_getnameinfo;
			}

			printf("name %s ipaddr %s\n", ifa->ifa_name, host);

			strcpy(ctx->ips->ips[i].name, ifa->ifa_name);
			ctx->ips->ips[i].addr = strdup(host);
		}
	}

	freeifaddrs(ifaddr);

	return true; 

err_getnameinfo:
err_ifaddrs:
	return false;
}	

/*
static void guid_init(char *guid)
{
 	//cat /proc/sys/kernel/random/uuid

}

static void ipaddr_init(struct ipaddr *ips)
{

}

static void *periodic(void *data)
{

}

*/
struct rpata *rpata_init()
{
	struct rpata *ctx = malloc(sizeof *ctx);
	if(!ctx)
		return NULL;

	iface_init(ctx);
/*
	guid_init(ctx->guid);	

	pthread_mutex_init(&ctx->mutex, NULL);

	if(pthread_create(&ctx->thread, NULL, periodic, ctx)){
		free(ctx);
		return NULL
	}

	*/
	return ctx;
}
